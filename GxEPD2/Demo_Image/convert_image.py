#!/usr/bin/env python3
"""
convert_image.py — 6-color e-paper image converter for reTerminal E1002

Converts any JPEG/PNG to a GxEPD2 PROGMEM header suitable for the
Seeed Studio reTerminal E1002's 7.3" 6-color panel (GDEP073E01, 800×480).

6-color ACeP palette (no orange!):
  Index 0x0  Black   (  0,   0,   0)
  Index 0x1  White   (255, 255, 255)
  Index 0x2  Green   (  0, 255,   0)
  Index 0x3  Blue    (  0,   0, 255)
  Index 0x4  Red     (255,   0,   0)
  Index 0x5  Yellow  (255, 255,   0)

Pixel packing (matches GxEPD2_7C.h):
  Each byte holds TWO pixels as two 4-bit nibbles, MSB = first pixel:
    byte = (pixel_left << 4) | pixel_right

Output: a C header file (mocha800x480.h by default) containing a single
  const uint8_t image800x480[] PROGMEM = { ... };
  declaration ready to be #include-d in the Arduino sketch.

Usage:
  python3 convert_image.py [input_image] [output_header] [var_name]

  Defaults:
    input_image   = mocha800x480.jpg
    output_header = mocha800x480.h
    var_name      = image800x480

Examples:
  python3 convert_image.py
  python3 convert_image.py myphoto.jpg myimage800x480.h myimage800x480
"""

from PIL import Image
import numpy as np
import sys
import os

# ---------------------------------------------------------------------------
# 6-color ACeP palette for GDEP073E01 (RGB tuples, lab-optimised order)
# ---------------------------------------------------------------------------
EINK_COLORS_RGB = np.array([
    [  0,   0,   0],   # 0x0  Black
    [255, 255, 255],   # 0x1  White
    [  0, 255,   0],   # 0x2  Green
    [  0,   0, 255],   # 0x3  Blue
    [255,   0,   0],   # 0x4  Red
    [255, 255,   0],   # 0x5  Yellow
], dtype=np.float32)

NUM_COLORS   = len(EINK_COLORS_RGB)   # 6
TARGET_WIDTH  = 800
TARGET_HEIGHT = 480


# ---------------------------------------------------------------------------
# Colour-distance helper — squared Euclidean in RGB space.
# For a small palette (6 colours) this is fast enough; no need for Lab.
# ---------------------------------------------------------------------------
def nearest_color(pixel_rgb: np.ndarray) -> int:
    """Return the palette index closest to *pixel_rgb* (shape (3,), float32)."""
    diffs = EINK_COLORS_RGB - pixel_rgb          # broadcast (6,3)
    sq    = np.einsum('ij,ij->i', diffs, diffs)  # squared distances
    return int(np.argmin(sq))


# ---------------------------------------------------------------------------
# Floyd-Steinberg dithering + quantisation
# ---------------------------------------------------------------------------
def quantise_to_6color(img: Image.Image) -> np.ndarray:
    """
    Return a 2-D array of palette indices (dtype uint8, shape HxW).
    Applies Floyd-Steinberg dithering.
    """
    arr   = np.array(img, dtype=np.float32)   # H x W x 3
    H, W  = arr.shape[:2]
    result = np.zeros((H, W), dtype=np.uint8)

    for y in range(H):
        for x in range(W):
            old_px = np.clip(arr[y, x], 0.0, 255.0)
            idx    = nearest_color(old_px)
            result[y, x] = idx
            new_px = EINK_COLORS_RGB[idx]
            err    = old_px - new_px

            # Floyd-Steinberg error diffusion
            if x + 1 < W:
                arr[y,     x + 1] += err * (7.0 / 16.0)
            if y + 1 < H:
                if x > 0:
                    arr[y + 1, x - 1] += err * (3.0 / 16.0)
                arr[y + 1, x    ] += err * (5.0 / 16.0)
                if x + 1 < W:
                    arr[y + 1, x + 1] += err * (1.0 / 16.0)

        # Progress indicator (every 10 % of rows)
        if (y + 1) % max(1, H // 10) == 0:
            pct = int(round((y + 1) / H * 100))
            print(f"  Dithering ... {pct:3d}%", end="\r", flush=True)

    print()   # newline after progress
    return result


# ---------------------------------------------------------------------------
# Pack nibbles: 2 pixels per byte, MSB = left pixel
# ---------------------------------------------------------------------------
def pack_nibbles(quantised: np.ndarray) -> list[int]:
    """
    Convert a 2-D array of 4-bit colour indices (H x W) into a list of
    bytes, two pixels packed per byte (MSB = earlier pixel).

    Width must be even; if not, the last pixel of each row is padded with
    White (0x1) so the buffer length stays a predictable W/2 * H bytes.
    """
    H, W = quantised.shape
    if W % 2 != 0:
        # pad each row with a white pixel on the right
        pad = np.full((H, 1), 0x1, dtype=np.uint8)
        quantised = np.concatenate([quantised, pad], axis=1)
        W += 1

    # Reshape to (H, W//2, 2) then combine nibbles
    pairs = quantised.reshape(H, W // 2, 2)
    packed = ((pairs[:, :, 0].astype(np.uint16) << 4) |
               pairs[:, :, 1].astype(np.uint16)).astype(np.uint8)
    return packed.flatten().tolist()


# ---------------------------------------------------------------------------
# Preview image (palette colours, no dithering artefacts)
# ---------------------------------------------------------------------------
def save_preview(quantised: np.ndarray, path: str) -> None:
    palette_u8 = EINK_COLORS_RGB.astype(np.uint8)
    rgb = palette_u8[quantised]                      # H x W x 3
    Image.fromarray(rgb, 'RGB').save(path)
    print(f"Preview saved → {path}")


# ---------------------------------------------------------------------------
# C header generation
# ---------------------------------------------------------------------------
def generate_header(
    byte_list: list[int],
    width: int,
    height: int,
    var_name: str,
    src_filename: str,
) -> str:
    guard = f"_{var_name.upper()}_H_"
    total = len(byte_list)
    lines = [
        f"// Generated from {src_filename}",
        f"// Target display: Seeed Studio reTerminal E1002  7.3\" GDEP073E01 (6-color ACeP)",
        f"// Image size    : {width} x {height} pixels",
        f"// Pixel format  : 4-bit nibbles, 2 pixels per byte (MSB = left pixel)",
        f"//   0x0=Black  0x1=White  0x2=Green  0x3=Blue  0x4=Red  0x5=Yellow",
        f"// Total bytes   : {total}  ({width}*{height}/2)",
        "",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#include <Arduino.h>",
        "",
        f"const uint8_t {var_name}[{total}] PROGMEM = {{",
    ]

    # 16 bytes per line
    for i in range(0, total, 16):
        chunk = byte_list[i : i + 16]
        hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
        comma   = "," if (i + 16) < total else ""
        lines.append(f"  {hex_str}{comma}")

    lines += [
        "};",
        "",
        f"#endif  // {guard}",
        "",
    ]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> None:
    # --- argument defaults ---
    input_file  = sys.argv[1] if len(sys.argv) > 1 else "mocha800x480.jpg"
    output_file = sys.argv[2] if len(sys.argv) > 2 else "mocha800x480.h"
    var_name    = sys.argv[3] if len(sys.argv) > 3 else "image800x480"

    print(f"=== reTerminal E1002 6-color image converter ===")
    print(f"  Input  : {input_file}")
    print(f"  Output : {output_file}")
    print(f"  Array  : {var_name}[{TARGET_WIDTH}*{TARGET_HEIGHT}//2]")
    print()

    # 1. Load
    if not os.path.exists(input_file):
        print(f"ERROR: input file '{input_file}' not found.")
        sys.exit(1)

    print(f"Loading {input_file} ...")
    img = Image.open(input_file)
    print(f"  Original size : {img.size}  mode={img.mode}")

    # 2. Convert to RGB
    if img.mode != "RGB":
        img = img.convert("RGB")

    # 3. Rotate if landscape source matches landscape display (or vice-versa)
    #    The E1002 panel is landscape (800 wide × 480 tall).  If the source
    #    image is portrait, rotate it 90° CW so it fills the screen naturally.
    if img.width < img.height:
        print("  Rotating 90° CW (portrait → landscape) ...")
        img = img.rotate(-90, expand=True)

    # 4. Resize to panel resolution with high-quality Lanczos filter
    print(f"  Resizing to {TARGET_WIDTH}×{TARGET_HEIGHT} ...")
    img = img.resize((TARGET_WIDTH, TARGET_HEIGHT), Image.Resampling.LANCZOS)

    # 5. Quantise with Floyd-Steinberg dithering
    print("  Quantising to 6-color palette with Floyd-Steinberg dithering ...")
    quantised = quantise_to_6color(img)

    # 6. Preview
    preview_path = output_file.rsplit(".", 1)[0] + "_preview.png"
    save_preview(quantised, preview_path)

    # 7. Pack nibbles
    print("  Packing nibbles (2 pixels/byte) ...")
    byte_list = pack_nibbles(quantised)
    expected  = (TARGET_WIDTH * TARGET_HEIGHT) // 2
    print(f"  Packed {len(byte_list)} bytes  (expected {expected})")

    # 8. Generate header
    print(f"  Writing {output_file} ...")
    header = generate_header(byte_list, TARGET_WIDTH, TARGET_HEIGHT, var_name, input_file)
    with open(output_file, "w") as f:
        f.write(header)

    print()
    print(f"Done!")
    print(f"  Header : {output_file}  ({len(byte_list)} bytes)")
    print(f"  Preview: {preview_path}")
    print()
    print("Next steps:")
    print(f"  1. Copy {output_file} into the GxEPD2_reTerminal_E1002_Image/ sketch folder.")
    print(f"  2. #include \"{output_file}\" in GxEPD2_reTerminal_E1002_Image.ino")
    print(f"     (already done if you kept the default filename).")
    print(f"  3. Upload to your XIAO ESP32-S3 — the panel refreshes in ~25-30 s.")


if __name__ == "__main__":
    main()
