#!/usr/bin/env python3
"""
convert_image.py — 6-color e-paper image converter for reTerminal E1002 (SD-card variant)

Converts any JPEG/PNG to a raw binary (.bin) file ready to copy to an SD card
and load at runtime by GxEPD2_reTerminal_E1002_SDcard.ino.

The output format is identical to the PROGMEM .h variant (mocha800x480.h) but
without the C wrapping — just raw packed bytes, suitable for SD.open() + read().

6-color ACeP palette  (GDEP073E01, no orange):
  Nibble 0x0  Black   (  0,   0,   0)
  Nibble 0x1  White   (255, 255, 255)
  Nibble 0x2  Green   (  0, 255,   0)
  Nibble 0x3  Blue    (  0,   0, 255)
  Nibble 0x4  Red     (255,   0,   0)
  Nibble 0x5  Yellow  (255, 255,   0)

Pixel packing (matches GxEPD2_7C.h color7() + drawNative):
  Each byte holds TWO pixels as two 4-bit nibbles, MSB = left pixel:
    byte = (pixel_left << 4) | pixel_right
  800 × 480 pixels → 192 000 bytes per file.

Dithering options (--dither flag):
  fs        Floyd-Steinberg   (default, best quality)
  bayer     8×8 ordered Bayer (no error buffer, fast)
  none      Nearest-color, no dithering (fastest, blocky)

Usage:
  python3 convert_image.py [options] input_image output.bin

  Positional:
    input_image   Source JPEG or PNG (any size, any orientation)
    output.bin    Destination raw binary (default: <input stem>.bin)

  Options:
    --dither fs|bayer|none   Dithering algorithm  (default: fs)
    --gamma FLOAT            Brightness gamma: >1 darkens, <1 brightens (default: 1.0)
    --no-preview             Skip saving the preview PNG
    --width  INT             Target width  in pixels (default: 800)
    --height INT             Target height in pixels (default: 480)

Examples:
  python3 convert_image.py photo.jpg mocha.bin
  python3 convert_image.py photo.jpg mocha.bin --dither bayer --gamma 0.9
  python3 convert_image.py photo.jpg mocha.bin --no-preview
"""

from __future__ import annotations
import argparse
import os
import sys
from pathlib import Path

import numpy as np
from PIL import Image

# ---------------------------------------------------------------------------
# 6-color ACeP palette — GxEPD2 nibble order
# ---------------------------------------------------------------------------
PALETTE_RGB = np.array([
    [  0,   0,   0],   # 0x0  Black
    [255, 255, 255],   # 0x1  White
    [  0, 255,   0],   # 0x2  Green
    [  0,   0, 255],   # 0x3  Blue
    [255,   0,   0],   # 0x4  Red
    [255, 255,   0],   # 0x5  Yellow
], dtype=np.float32)

NUM_COLORS = len(PALETTE_RGB)

# ---------------------------------------------------------------------------
# Colour distance (squared Euclidean RGB — fast enough for 6 colours)
# ---------------------------------------------------------------------------
def nearest_idx(pixel: np.ndarray) -> int:
    diffs = PALETTE_RGB - pixel
    return int(np.argmin(np.einsum("ij,ij->i", diffs, diffs)))

# ---------------------------------------------------------------------------
# Gamma correction
# ---------------------------------------------------------------------------
def apply_gamma(arr: np.ndarray, gamma: float) -> np.ndarray:
    if gamma == 1.0:
        return arr
    return np.clip(255.0 * (arr / 255.0) ** gamma, 0.0, 255.0).astype(np.float32)

# ---------------------------------------------------------------------------
# Dithering algorithms
# ---------------------------------------------------------------------------
def dither_none(arr: np.ndarray) -> np.ndarray:
    H, W = arr.shape[:2]
    out  = np.zeros((H, W), dtype=np.uint8)
    for y in range(H):
        for x in range(W):
            out[y, x] = nearest_idx(np.clip(arr[y, x], 0.0, 255.0))
        if (y + 1) % max(1, H // 10) == 0:
            print(f"  {int((y+1)/H*100):3d}%", end="\r", flush=True)
    print()
    return out


def dither_floyd_steinberg(arr: np.ndarray) -> np.ndarray:
    arr = arr.copy()
    H, W = arr.shape[:2]
    out  = np.zeros((H, W), dtype=np.uint8)
    for y in range(H):
        for x in range(W):
            old = np.clip(arr[y, x], 0.0, 255.0)
            idx = nearest_idx(old)
            out[y, x] = idx
            err = old - PALETTE_RGB[idx]
            if x + 1 < W:
                arr[y,     x + 1] += err * (7.0 / 16.0)
            if y + 1 < H:
                if x > 0:
                    arr[y + 1, x - 1] += err * (3.0 / 16.0)
                arr[y + 1, x    ] += err * (5.0 / 16.0)
                if x + 1 < W:
                    arr[y + 1, x + 1] += err * (1.0 / 16.0)
        if (y + 1) % max(1, H // 10) == 0:
            print(f"  {int((y+1)/H*100):3d}%", end="\r", flush=True)
    print()
    return out


# 8×8 Bayer matrix (normalised to [-0.5, 0.5] × 128)
_BAYER8 = (np.array([
    [ 0, 32,  8, 40,  2, 34, 10, 42],
    [48, 16, 56, 24, 50, 18, 58, 26],
    [12, 44,  4, 36, 14, 46,  6, 38],
    [60, 28, 52, 20, 62, 30, 54, 22],
    [ 3, 35, 11, 43,  1, 33,  9, 41],
    [51, 19, 59, 27, 49, 17, 57, 25],
    [15, 47,  7, 39, 13, 45,  5, 37],
    [63, 31, 55, 23, 61, 29, 53, 21],
], dtype=np.float32) / 64.0 - 0.5) * 128.0   # range ≈ ±64


def dither_bayer(arr: np.ndarray) -> np.ndarray:
    H, W = arr.shape[:2]
    out  = np.zeros((H, W), dtype=np.uint8)
    for y in range(H):
        for x in range(W):
            threshold = _BAYER8[y % 8, x % 8]
            pixel = np.clip(arr[y, x] + threshold, 0.0, 255.0)
            out[y, x] = nearest_idx(pixel)
        if (y + 1) % max(1, H // 10) == 0:
            print(f"  {int((y+1)/H*100):3d}%", end="\r", flush=True)
    print()
    return out

# ---------------------------------------------------------------------------
# Nibble packing: 2 pixels per byte, MSB = left pixel
# ---------------------------------------------------------------------------
def pack_nibbles(quantised: np.ndarray) -> bytes:
    H, W = quantised.shape
    assert W % 2 == 0, "Width must be even for nibble packing"
    pairs  = quantised.reshape(H, W // 2, 2)
    packed = ((pairs[:, :, 0].astype(np.uint8) << 4) |
               pairs[:, :, 1].astype(np.uint8))
    return packed.tobytes()

# ---------------------------------------------------------------------------
# Preview
# ---------------------------------------------------------------------------
def save_preview(quantised: np.ndarray, path: str) -> None:
    rgb = PALETTE_RGB.astype(np.uint8)[quantised]
    Image.fromarray(rgb, "RGB").save(path)
    print(f"  Preview  → {path}")

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Convert image → 6-color raw binary for reTerminal E1002 SD-card sketch"
    )
    p.add_argument("input",  help="Source image (JPEG/PNG)")
    p.add_argument("output", nargs="?", help="Output .bin (default: <input>.bin)")
    p.add_argument("--dither",     choices=["fs", "bayer", "none"], default="fs")
    p.add_argument("--gamma",      type=float, default=1.0,
                   help="Brightness gamma (>1 darkens, <1 brightens, default 1.0)")
    p.add_argument("--no-preview", action="store_true")
    p.add_argument("--width",      type=int, default=800)
    p.add_argument("--height",     type=int, default=480)
    return p.parse_args()


def main() -> None:
    args = parse_args()

    input_path  = Path(args.input)
    output_path = Path(args.output) if args.output else input_path.with_suffix(".bin")
    W, H        = args.width, args.height

    print("=== reTerminal E1002 6-color SD-card image converter ===")
    print(f"  Input    : {input_path}")
    print(f"  Output   : {output_path}")
    print(f"  Size     : {W} × {H}  ({W*H//2} bytes)")
    print(f"  Dither   : {args.dither}")
    print(f"  Gamma    : {args.gamma}")
    print()

    if not input_path.exists():
        sys.exit(f"ERROR: '{input_path}' not found.")

    # 1. Load & normalise
    print(f"Loading {input_path} ...")
    img = Image.open(input_path)
    print(f"  Original : {img.size}  mode={img.mode}")
    if img.mode != "RGB":
        img = img.convert("RGB")

    # 2. Rotate portrait → landscape to match 800×480 panel
    if img.width < img.height:
        print("  Rotating 90° CW (portrait → landscape) ...")
        img = img.rotate(-90, expand=True)

    # 3. Resize
    print(f"  Resizing to {W}×{H} ...")
    img = img.resize((W, H), Image.Resampling.LANCZOS)

    # 4. Gamma
    arr = np.array(img, dtype=np.float32)
    if args.gamma != 1.0:
        print(f"  Applying gamma {args.gamma} ...")
        arr = apply_gamma(arr, args.gamma)

    # 5. Dither / quantise
    print(f"  Quantising ({args.dither}) ...")
    if args.dither == "fs":
        quantised = dither_floyd_steinberg(arr)
    elif args.dither == "bayer":
        quantised = dither_bayer(arr)
    else:
        quantised = dither_none(arr)

    # 6. Preview
    if not args.no_preview:
        preview_path = output_path.with_name(output_path.stem + "_preview.png")
        save_preview(quantised, str(preview_path))

    # 7. Pack and write raw binary
    print("  Packing nibbles (2 pixels/byte) ...")
    raw = pack_nibbles(quantised)
    assert len(raw) == W * H // 2, f"Unexpected size: {len(raw)}"

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(raw)
    print(f"  Written  → {output_path}  ({len(raw)} bytes)")

    print()
    print("Done!")
    print()
    print("Next steps:")
    print(f"  1. Copy '{output_path.name}' to /images/ on your SD card.")
    print(f"  2. Set IMAGE_PATH = \"/images/{output_path.name}\" in the sketch.")
    print(f"  3. Upload GxEPD2_reTerminal_E1002_SDcard.ino once.")
    print(f"     Thereafter swap images by replacing the .bin on the SD card —")
    print(f"     no reflashing needed.")


if __name__ == "__main__":
    main()
