========================================================================
Good Display GDEP073E01 (7.3" 6-Color ACeP) - Partial Refresh Example
Hardware: Seeed Studio reTerminal E1002 (XIAO ESP32-S3)
========================================================================

Pinout Mapping:
------------------------------------------------------------------------
Pin Function    reTerminal E1002 Pin    XIAO ESP32-S3 GPIO
------------------------------------------------------------------------
SCK / SCLK      FPC Pin 12              GPIO 7 (HSPI SCK, 2 MHz)
MOSI / SDA      FPC Pin 14              GPIO 9 (HSPI MOSI)
CS              FPC Pin 16              GPIO 10
DC              FPC Pin 17              GPIO 11
RST / RES       FPC Pin 18              GPIO 12
BUSY            FPC Pin 19              GPIO 13 (LOW = Busy, HIGH = Idle)
Carrier UART TX USB-C Bridge            GPIO 43 (Serial1 TX, 115200)
Carrier UART RX USB-C Bridge            GPIO 44 (Serial1 RX, 115200)

Description:
------------------------------------------------------------------------
This sketch demonstrates the official Good Display ED2208 Partial / Specified 
Area Refresh capability (Command 0x83 / CMD_PARTIAL_WINDOW) on the 800x480
Spectra 6 (ACeP) color panel:
1. Displays test bitmap gImage_1 (upright landscape).
2. Renders an 8x8 chessboard pattern of alternating white and black blocks.
3. Selectively refreshes individual 100x60 pixel blocks to RED, YELLOW,
   BLUE, and GREEN without redrawing the entire screen buffer.
4. Clears screen to white and powers down cleanly to deep sleep.

Partial Refresh Behavior on Spectra 6 / ACeP:
------------------------------------------------------------------------
- Why colors appear lighter/faded: ACeP panels contain 6 distinct pigment 
  particles suspended in each microcup. A full-screen refresh vigorously 
  agitates all particles for ~15 seconds to achieve maximum separation and 
  saturation. Partial updates restrict the electric field to the targeted 
  window to avoid disturbing static content outside; the gentler waveform 
  results in lighter/pastel tones.
- Best practice: Periodically run a full refresh to clear residual ghosting 
  and restore full saturation.
