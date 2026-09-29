# Good Display Vendor Driver Examples — Seeed Studio reTerminal E1002

Official Good Display vendor drivers ported directly to the **Seeed Studio reTerminal E1002** (XIAO ESP32-S3 carrier board).

- **Panel:** Good Display GDEP073E01 (7.3″ 6-Color ACeP / Spectra 6)
- **Controller IC:** ED2208
- **Resolution:** 800 × 480 px (physical landscape orientation)
- **Supported Colors:** Black, White, Yellow, Red, Blue, Green (no orange)
- **Host MCU:** Seeed Studio XIAO ESP32-S3

---

## 🔗 Official Documentation & Resources

- **Product Page:** [Good Display GDEP073E01](https://www.good-display.com/product/533.html)
- **Store Link:** [Buy-LCD GDEP073E01](https://buy-lcd.com/products/gdep073e01)
- **Panel Datasheet:**
  - Local PDF: [`GDEP073E01-1.0.pdf`](GDEP073E01-1.0.pdf) *(downloaded directly from Good Display)*
  - Online Link: [GDEP073E01 Specification v1.0](https://v4.cecdn.yun300.cn/100001_1909185148/GDEP073E01-1.0.pdf)
- **Bitmap Production Guide:** [GDEP073E01 Picture Production and Bitmap Conversion 7.30](https://v4.cecdn.yun300.cn/100001_1909185148/GDEP073E01%20Picture%20Production%20and%20Bitmap%20Conversion%207.30.pdf)
- **Official Source Packages (Good Display CDN):**
  - Full-Refresh ESP32 Sample: [`ESP32-GDEP073E01.zip`](https://v4.cecdn.yun300.cn/100001_1909185148/ESP32-GDEP073E01.zip) (ID 1435)
  - Specified-Area Partial Refresh ESP32 Sample: [`ESP32-GDEP073E01 - 局刷.zip`](https://v4.cecdn.yun300.cn/100001_1909185148/ESP32-GDEP073E01%20-%20%E5%B1%80%E5%88%B7.zip) (ID 1908 / 2037)

---

## 📌 Hardware Pinout & Wiring

All pin definitions are pre-configured in `Display_EPD_W21_spi.h`:

| Signal | reTerminal E1002 FPC | XIAO ESP32-S3 GPIO | Function |
|---|---|---|---|
| **SCK / SCLK** | Pin 12 | **GPIO 7** | Hardware SPI Clock (HSPI, 2 MHz) |
| **MOSI / SDA** | Pin 14 | **GPIO 9** | Hardware SPI Data Out |
| **CS** | Pin 16 | **GPIO 10** | Chip Select (Active LOW) |
| **DC** | Pin 17 | **GPIO 11** | Data (HIGH) / Command (LOW) |
| **RST / RES** | Pin 18 | **GPIO 12** | Hardware Reset (Active LOW) |
| **BUSY** | Pin 19 | **GPIO 13** | Hardware Busy Status (**LOW = Busy**, HIGH = Idle) |
| **UART TX** | Carrier USB-C Bridge | **GPIO 43** | Serial1 Transmit (`115200` baud) |
| **UART RX** | Carrier USB-C Bridge | **GPIO 44** | Serial1 Receive (`115200` baud) |

> **Serial Note:** On the reTerminal E1002, the onboard USB-C port is wired to a CP2102/CH340 USB-UART bridge connected to **GPIO 43 (TX) and GPIO 44 (RX)**. Sketches output log messages simultaneously to `Serial1` (the carrier USB port) and native USB CDC `Serial`.

---

## 📂 Sketches

### 1. [`GDEP073E01/`](GDEP073E01/) — Full-Panel Refresh Demo
* **Source:** Official Good Display ESP32 sample package (`ESP32-GDEP073E01.zip`).
* **Sequence:**
  1. Displays full-screen 800×480 bitmap (`gImage_1`), formatted in native upright landscape.
  2. Displays 6 solid color fields (White, Black, Yellow, Blue, Green, Red).
  3. Clears the display to white and enters deep sleep (`EPD_sleep()`).
* **Refresh Characteristics:** Uses full electrophoretic agitation (~15 s per frame) across the entire screen, yielding full color saturation and contrast.

### 2. [`GDEP073E01_Partial/`](GDEP073E01_Partial/) — Specified-Area Partial Window Refresh Demo
* **Source:** Latest official Good Display partial package (`ESP32-GDEP073E01 - 局刷.zip`).
* **Sequence:**
  1. Displays full-screen test bitmap (`gImage_1`).
  2. Renders an 8×8 black-and-white chessboard pattern across the 800×480 screen.
  3. Uses ED2208 `CMD_PARTIAL_WINDOW` (`0x83`) to update individual 100×60 pixel blocks with colors (**Red**, **Yellow**, **Blue**, **Green**) without redrawing the rest of the display.
  4. Clears screen to white and powers down to deep sleep.

---

## ⚡ Partial Refresh Behavior on Spectra 6 / ACeP Panels

The GDEP073E01 is an **ACeP (Advanced Color e-Paper)** display containing 6 distinct physical pigment particles inside each microcup.

### Why Colors Appear Faded During Partial Updates
1. **Shared VCOM Plane:** The top ITO conductive layer covers the entire physical glass panel. A full-screen refresh vigorously shakes all particles for 15 seconds to achieve maximum separation and vivid saturation.
2. **Localized Driving (`0x83`):** During a partial window update, the driver restricts electric fields to the targeted window to avoid disturbing the static content outside. Because this localized driving cannot apply full agitation without corrupting surrounding pixels, particle separation is less complete, resulting in a lighter/pastel/faded tone.
3. **Best Practice:** Use partial window updates for quick incremental status changes, and periodically trigger a full-screen refresh to clean residual ghosting and restore maximum color depth.

---

## 🖼️ Image Orientation Details

* The reTerminal E1002 display is physically mounted in **landscape (800 horizontal × 480 vertical)**.
* Good Display's original demo bitmap was authored in vertical portrait orientation (intended for badges).
* In this repository, `image.h` provides:
  * `gImage_1`: The official Good Display child portrait, rotated $90^\circ$ clockwise and centered with clean borders to display upright on the landscape screen.
  * `gImage_7`: Good Display's official native 800×480 horizontal conference nameplate image.
