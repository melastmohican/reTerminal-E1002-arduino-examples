# reTerminal E1002 Arduino Examples

Arduino sketches for the **Seeed Studio reTerminal E1002** — a 7.3″ 6-color ACeP e-paper panel driven by an XIAO ESP32-S3 carrier board.

## 🚨 Required Arduino IDE Settings

> These settings apply to **every sketch** in this repository.
> Without them the board will not flash or behave correctly.

| Setting | Path in Arduino IDE | Required Value |
|---|---|---|
| **Board** | Tools → Board | **XIAO_ESP32S3** |
| **Upload Speed** | Tools → Upload Speed | **115200** |
| **PSRAM** | Tools → PSRAM | **OPI PSRAM** *(GxEPD2 sketches only)* |

> PSRAM is only required for the `GxEPD2/` sketches that load images into a PSRAM buffer.
> The `Peripherals/` sketches do not need PSRAM enabled.

---

## Examples

### [`GxEPD2/`](GxEPD2/) — ePaper display

Sketches for the 7.3″ 6-color ACeP panel (GDEP073E01 · ED2208 controller).

| Sketch | Description |
|---|---|
| [`Demo/`](GxEPD2/Demo/) | Drawing primitives — shapes, text, color swatches |
| [`Demo_Image/`](GxEPD2/Demo_Image/) | Flash-embedded image (no SD card needed) |
| [`Demo_SDcard/`](GxEPD2/Demo_SDcard/) | Raw nibble-packed `.bin` image from SD card via PSRAM |
| [`Demo_BMP/`](GxEPD2/Demo_BMP/) | Standard 24-bit BMP from SD card via PSRAM |

> **Library note:** these examples use [ZinggJM/GxEPD2](https://github.com/ZinggJM/GxEPD2) (install via Library Manager, search "GxEPD2" by Jean-Marc Zingg), **not** the Seeed-provided `Seeed_GxEPD2` fork. The two libraries conflict — do not install both.

See [`GxEPD2/README.md`](GxEPD2/README.md) for full details.

---

### [`Peripherals/`](Peripherals/) — Onboard hardware

Sketches for every onboard peripheral of the E1002 carrier board.

| Sketch | Peripheral | GPIO |
|---|---|---|
| [`Battery/`](Peripherals/Battery/) | Li-Po voltage monitor (ADC + enable) | 1, 21 |
| [`Button/`](Peripherals/Button/) | Three user buttons with debounce | 3, 4, 5 |
| [`Buzzer/`](Peripherals/Buzzer/) | Passive buzzer — chime & sweep | 45 |
| [`BuzzerWithTone/`](Peripherals/BuzzerWithTone/) | Imperial March melody | 45 |
| [`LED/`](Peripherals/LED/) | User LED blink & fade (active-LOW) | 6 |
| [`RTC/`](Peripherals/RTC/) | PCF8563 real-time clock set & read | 19 SDA, 20 SCL |
| [`SD/`](Peripherals/SD/) | microSD card read / write / list | 7-9, 14-16 |
| [`SHT4x/`](Peripherals/SHT4x/) | SHT4x temperature & humidity | 19 SDA, 20 SCL |

See [`Peripherals/README.md`](Peripherals/README.md) for full details and required libraries.

---

## Hardware

- **Board:** [Seeed Studio reTerminal E1002](https://www.seeedstudio.com/reTerminal-E1002.html)
- **MCU:** XIAO ESP32-S3
- **Display:** 7.3″ 6-color ACeP e-paper — Black, White, Red, Green, Blue, Yellow (**no orange**)
- **Reference:** [Seeed Wiki — Arduino Cookbook](https://wiki.seeedstudio.com/reterminal_e10xx_with_arduino/)

## License

MIT
