# Waveshare Vendor Driver Examples — Seeed Studio reTerminal E1002

Official Waveshare e-Paper driver and graphics library ported to the **Seeed Studio reTerminal E1002** (XIAO ESP32-S3 carrier board).

- **Panel:** Waveshare 7.3inch e-Paper (E) / Good Display GDEP073E01
- **Controller IC:** ED2208 (6-color ACeP / Spectra 6)
- **Resolution:** 800 × 480 px (physical landscape orientation)
- **Supported Colors:** Black, White, Yellow, Red, Blue, Green (no orange)
- **Host MCU:** Seeed Studio XIAO ESP32-S3

---

## 🔗 Official Documentation & Resources

### Product & Documentation Pages
- **Panel Product Page:** [Waveshare 7.3inch e-Paper (E) Panel](https://www.waveshare.com/7.3inch-e-paper-e.htm)
- **Wiki Documentation & Manual:** [Waveshare 7.3inch e-Paper (E) Manual](https://www.waveshare.com/wiki/7.3inch_e-Paper_HAT_(E)_Manual)

### Official Datasheet & Manual
- **User Manual & Panel Specification:**
  - Local PDF: [`7.3inch-e-Paper-(E)-user-manual.pdf`](7.3inch-e-Paper-(E)-user-manual.pdf) *(downloaded locally beside this README)*
  - Online Link: [7.3inch-e-Paper-(E)-user-manual.pdf](https://files.waveshare.com/wiki/7.3inch-e-Paper-HAT-(E)/7.3inch-e-Paper-(E)-user-manual.pdf)

### Official Source Packages & Repositories
- **Upstream GitHub Repository:** [waveshareteam/e-Paper](https://github.com/waveshareteam/e-Paper)
  - Upstream Arduino R4 Driver: [`Arduino_R4/src/e-Paper/EPD_7in3e.cpp`](https://github.com/waveshareteam/e-Paper/blob/master/Arduino_R4/src/e-Paper/EPD_7in3e.cpp) / [`.h`](https://github.com/waveshareteam/e-Paper/blob/master/Arduino_R4/src/e-Paper/EPD_7in3e.h)
  - Upstream Test Routine: [`Arduino_R4/src/Examples/EPD_7in3e_test.cpp`](https://github.com/waveshareteam/e-Paper/blob/master/Arduino_R4/src/Examples/EPD_7in3e_test.cpp)
  - Upstream Raspberry Pi / Jetson C Driver: [`RaspberryPi_JetsonNano/c/lib/e-Paper/EPD_7in3e.c`](https://github.com/waveshareteam/e-Paper/blob/master/RaspberryPi_JetsonNano/c/lib/e-Paper/EPD_7in3e.c)
  - Upstream Raspberry Pi Python Driver: [`RaspberryPi_JetsonNano/python/lib/waveshare_epd/epd7in3e.py`](https://github.com/waveshareteam/e-Paper/blob/master/RaspberryPi_JetsonNano/python/lib/waveshare_epd/epd7in3e.py)
- **Official Demo Code Archive:** [`E-Paper_code.zip`](https://files.waveshare.com/wiki/common/E-Paper_code.zip) *(33.5 MB, includes Arduino, STM32, Raspberry Pi, Jetson Nano examples for all Waveshare e-Paper models)*

### Development Tools & Tutorials
- **Bitmap Image Conversion Tool:** [`Image2Lcd.7z`](https://files.waveshare.com/upload/3/36/Image2Lcd.7z)
- **Image2Lcd Conversion Guide:** [Wiki Image2Lcd Tutorial](https://www.waveshare.com/wiki/Image2Lcd_Image_Bitmap_Conversion)
- **Font Generation Tool:** [`Zimo221.7z`](https://files.waveshare.com/upload/c/c6/Zimo221.7z)
- **Font Library Tutorial:** [Wiki Font Tutorial](https://www.waveshare.com/wiki/Ink_Screen_Font_Library_Tutorial)
- **Floyd-Steinberg Color Dithering:** [Wiki Dithering Guide](https://www.waveshare.com/wiki/E-Paper_Floyd-Steinberg)
- **Waveshare e-Paper API Architecture:** [Wiki API Analysis](https://www.waveshare.com/wiki/E-Paper_API_Analysis)

---

## 📌 Hardware Pinout & Wiring

All pin definitions are configured in [`DEV_Config.h`](DEV_Config.h):

| Signal | reTerminal E1002 FPC | XIAO ESP32-S3 GPIO | Function |
|---|---|---|---|
| **SCK** | Pin 12 | **GPIO 7** | Hardware SPI Clock (HSPI, 2 MHz) |
| **MOSI** | Pin 14 | **GPIO 9** | Hardware SPI Data Out |
| **CS** | Pin 16 | **GPIO 10** | Chip Select (Active LOW) |
| **DC** | Pin 17 | **GPIO 11** | Data (HIGH) / Command (LOW) |
| **RST** | Pin 18 | **GPIO 12** | Hardware Reset (Active LOW) |
| **BUSY** | Pin 19 | **GPIO 13** | Hardware Busy Status (**LOW = Busy**, HIGH = Idle) |
| **UART TX** | Carrier USB-C Bridge | **GPIO 43** | Serial1 Transmit (`115200` baud) |
| **UART RX** | Carrier USB-C Bridge | **GPIO 44** | Serial1 Receive (`115200` baud) |

> **Serial Note:** On the reTerminal E1002, the onboard USB-C port is wired to a CP2102/CH340 USB-UART bridge connected to **GPIO 43 (TX) and GPIO 44 (RX)**. All driver debugging and status messages are piped simultaneously to `Serial1` (the carrier USB port) and native USB CDC `Serial`.

---

## 📂 Source Code Architecture & Porting Notes

Waveshare's Arduino library separates hardware access, panel driving, and GUI rendering into modular layers:

```
Waveshare_7in3e/
├── DEV_Config.cpp / .h   # Hardware Abstraction Layer (SPI, GPIO, dual serial logging)
├── EPD_7in3e.cpp / .h    # Low-level ED2208 panel driver & register control
├── GUI_Paint.cpp / .h    # Waveshare 2D vector drawing engine (lines, circles, text)
├── ImageData.cpp / .h    # 800×480 landscape sample photo bitmap (192,000 bytes)
├── fonts.h / font*.cpp   # ASCII (8..24 pt) & Chinese GB2312 (12, 24 pt) bitmap fonts
└── Waveshare_7in3e.ino   # Main application demo sketch
```

### Porting Modifications for reTerminal E1002
1. **SPI Frequency & Pin Assignment:** Configured to use hardware HSPI at 2 MHz (`SPISettings(2000000, MSBFIRST, SPI_MODE0)`) on GPIO 7 (SCK) and GPIO 9 (MOSI) to maintain clean signal integrity across the reTerminal FPC ribbon cable.
2. **Dual Serial Logging:** Standard `printf` in `DEV_Config.cpp` has been bridged to output to both `Serial1` (GPIO 43 carrier bridge) and native USB `Serial`.
3. **BUSY Wait Logic:** BUSY pin on the ED2208 controller is active LOW (`0 = Busy`, `1 = Idle`). Polling includes a 20 ms safety window after sending `0x12` (`DRF`) before polling to allow the internal charge pump and sequencer to assert BUSY.
4. **Buffer Allocation:** The 800×480 6-color image buffer requires 192 KB of RAM ($800 \times 480 / 2$). The XIAO ESP32-S3 allocates this cleanly in internal SRAM / PSRAM.

---

## 🎨 Color Palette & Data Format

The ED2208 controller for the 7.3″ Spectra 6 panel packs **2 pixels per byte** (4 bits per pixel):

$$\text{Byte} = (\text{Pixel}_0 \ll 4) \mid \text{Pixel}_1$$

| Color | Hex Value | Binary Nibble |
|---|---|---|
| **Black** | `0x0` | `0000` |
| **White** | `0x1` | `0001` |
| **Yellow** | `0x2` | `0010` |
| **Red** | `0x3` | `0011` |
| **Blue** | `0x5` | `0101` |
| **Green** | `0x6` | `0110` |

*(Note: Value `0x4` is reserved / orange on 7-color panels; the 7.3″ 6-color panel does not support orange).*

---

## 🚀 Sketch Execution Sequence (`Waveshare_7in3e.ino`)

1. **Hardware Init:** Configures GPIOs, initialises HSPI, resets the panel (`0x00`), and sends controller initialization parameters.
2. **White Background Clear:** Writes clean white (`0x11`) across all 192,000 bytes and triggers full electrophoretic refresh.
3. **Landscape Image (`gImage_7in3e`):** Displays the official Waveshare 800×480 landscape sample photo stored in flash.
4. **Color Swatches (`EPD_7IN3E_Show7Block`):** Renders vertical solid color stripes (Black, White, Yellow, Red, Blue, Green) directly using hardware register commands.
5. **In-Memory GUI Primitives:**
   - Allocates 192 KB framebuffer (`Paint_NewImage`).
   - Draws geometric primitives: lines, empty rectangles, filled rectangles, and circles.
   - Renders multi-size text fonts (`Font8`, `Font12`, `Font16`, `Font20`, `Font24`).
   - Renders Chinese characters with `Font12CN` and `Font24CN`.
   - Flushes buffer to display and refreshes.
6. **Deep Sleep:** Sends `0x02` (`POF`) followed by `0x07` (`DSLP`) to put the ED2208 into microamp hibernation mode to prevent DC bias degradation.
