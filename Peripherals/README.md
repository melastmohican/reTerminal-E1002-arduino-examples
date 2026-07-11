# Peripherals Examples — Seeed Studio reTerminal E1002

Arduino sketches for the onboard peripherals of the **reTerminal E1002** carrier board.

- **MCU:** XIAO ESP32-S3
- **Reference:** [Seeed Wiki — Arduino Cookbook: Onboard Peripherals](https://wiki.seeedstudio.com/reterminal_e10xx_with_arduino_peripherals/)

---

## 🚨 Required Arduino IDE Setting (All Sketches)

> **The board will not flash correctly without this setting.**

| Setting | Path in Arduino IDE | Required Value |
|---|---|---|
| **Upload Speed** | Tools → Upload Speed | **115200** |

The default Arduino IDE upload speed is too high for this board and causes
flashing failures.

> **Note:** PSRAM does **not** need to be enabled for these peripheral examples —
> none of them use the ePaper display or allocate large buffers. PSRAM is only
> required for the sketches in the `GxEPD2/` folder.

---

## Examples

### `Battery/` — Battery voltage monitor

Reads the Li-Po battery voltage through the onboard resistor divider.
The enable pin (`GPIO21`) must be driven HIGH before sampling and LOW
afterward to avoid wasting current through the divider.

**Pin mapping:**
| Signal | GPIO |
|---|---|
| Battery ADC | 1 |
| Battery monitor enable | 21 (active-high) |

---

### `Button/` — Three user buttons

Reads the three user buttons (KEY0, KEY1, KEY2) with time-based debouncing.
The carrier board provides hardware pull-ups; the GPIO reads LOW when pressed.

**Pin mapping:**
| Button | GPIO |
|---|---|
| KEY0 | 3 |
| KEY1 | 4 |
| KEY2 | 5 |

---

### `Buzzer/` — Passive buzzer

Drives the onboard passive buzzer using Arduino's `tone()` / `noTone()` API.
Demonstrates a startup chime, a double-beep, and a frequency sweep alert.

**Pin mapping:**
| Signal | GPIO |
|---|---|
| Buzzer | 45 |

---

### `BuzzerWithTone/` — Musical melody on the buzzer

Plays the **Imperial March** (Darth Vader theme from Star Wars) using a
score player that supports regular and dotted notes.  Melody adapted from
[robsoncouto/arduino-songs](https://github.com/robsoncouto/arduino-songs),
found in the RadioLib AFSK Imperial March example in the local Arduino library.

Score format: `{freq_Hz, divisor}` pairs — divisor 4 = quarter note,
negative divisor = dotted note (duration × 1.5), frequency 0 = rest.

**Pin mapping:** same as `Buzzer/` (GPIO 45).

---

### `LED/` — User LED blink & fade

Blinks and fades the onboard user LED using `digitalWrite()` and
`analogWrite()` (LEDC).

> **GPIO6, active-LOW:** the LED turns ON when the pin is driven LOW and
> OFF when driven HIGH. The fade direction is inverted accordingly
> (`analogWrite(0)` = full brightness, `analogWrite(255)` = off).

**Pin mapping:**
| Signal | GPIO | Notes |
|---|---|---|
| User LED | 6 | active-LOW (inverted logic) |

---

### `RTC/` — PCF8563 real-time clock

Reads and sets the onboard PCF8563 I2C RTC.  Demonstrates setting the
initial time at startup and reading back the current time every second.

**Required library:** `PCF8563 RTC` by Seeed Studio (Arduino Library Manager)

**Pin mapping (I2C):**
| Signal | GPIO |
|---|---|
| SDA | 19 |
| SCL | 20 |

> Set `kSetTime = false` after the first flash to avoid resetting the clock
> on every reboot.

---

### `SD/` — microSD card read / write

Mounts the microSD card and demonstrates:
- Listing directory contents
- Writing a new file
- Reading it back
- Appending to an existing file

Handles the carrier board's power-enable and card-detect pins correctly.

**Pin mapping (HSPI):**
| Signal | GPIO |
|---|---|
| SCK | 7 |
| MISO | 8 |
| MOSI | 9 |
| CS | 14 |
| Power enable | 16 (active-high) |
| Card detect | 15 (active-low) |

---

### `SHT4x/` — Temperature & humidity sensor

Reads the onboard Sensirion SHT40 / SHT41 / SHT45 sensor over I2C and
prints temperature (°C) and relative humidity (%RH) every 2 seconds.

**Required library:** `Adafruit SHT4x Library` by Adafruit
(also requires `Adafruit BusIO`)

**Pin mapping (I2C — same bus as RTC):**
| Signal | GPIO |
|---|---|
| SDA | 19 |
| SCL | 20 |

---

## Serial Logging

All sketches (except `LED/`) log to **both** serial ports simultaneously:

- `Serial1` (UART1) on GPIO 43 TX / 44 RX — always available via the E1002
  carrier board's USB-to-UART bridge
- `Serial` (USB CDC) — available when **USB CDC On Boot** is enabled

Connect at **115200 baud**.
