/*****************************************************************************
* | File        :   GDEP073E01.ino
* | Function    :   7.3inch 6-color ACeP e-Paper demo (800x480)
* | Info        :
*----------------
* Good Display's official GDEP073E01 sample (ED2208 controller), adapted to
* run on Seeed Studio reTerminal E1002 (XIAO ESP32-S3).
*
* Board       :   XIAO_ESP32S3 (esp32:esp32:XIAO_ESP32S3)
* Panel       :   Good Display GDEP073E01 / Waveshare 7.3inch e-Paper (E)
*                 800x480, 6 native colors (Black, White, Yellow, Red, Blue, Green)
*
* Pinout (reTerminal E1002 onboard wiring):
*   SCK  -> GPIO 7
*   MOSI -> GPIO 9
*   CS   -> GPIO 10
*   DC   -> GPIO 11
*   RST  -> GPIO 12
*   BUSY -> GPIO 13 (Active LOW on ED2208: 0 = busy, 1 = idle)
*
* Logging:
*   Simultaneous output to Serial1 (carrier board USB-UART on GPIO 43/44)
*   and Serial (native USB CDC).
******************************************************************************/
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"
#include "image.h"

void setup()
{
  EPD_SPI_Init();

  LOG_PRINTLN();
  LOG_PRINTLN(F("=================================================="));
  LOG_PRINTLN(F(" Good Display GDEP073E01 (reTerminal E1002) Demo  "));
  LOG_PRINTLN(F(" 7.3\" 800x480 6-Color ACeP (ED2208 controller)    "));
  LOG_PRINTLN(F("=================================================="));

  // 1. Full display of official sample bitmap
  LOG_PRINTLN(F("[GDEP073E01] 1. Displaying sample image..."));
  EPD_init();
  PIC_display(gImage_1);
  LOG_PRINTLN(F("[GDEP073E01] Holding image for 10 seconds..."));
  delay(10000);

  // 2. Demonstration of full color fields
  LOG_PRINTLN(F("[GDEP073E01] 2. Solid color swatches..."));
  EPD_init();

  LOG_PRINTLN(F("  - White"));
  EPD_Display_White();
  delay(5000);

  LOG_PRINTLN(F("  - Black"));
  EPD_Display_Black();
  delay(5000);

  LOG_PRINTLN(F("  - Yellow"));
  EPD_Display_Yellow();
  delay(5000);

  LOG_PRINTLN(F("  - Blue"));
  EPD_Display_blue();
  delay(5000);

  LOG_PRINTLN(F("  - Green"));
  EPD_Display_Green();
  delay(5000);

  LOG_PRINTLN(F("  - Red"));
  EPD_Display_red();
  delay(5000);

  // 3. Clear display and enter deep sleep
  LOG_PRINTLN(F("[GDEP073E01] 3. Clearing screen to White..."));
  PIC_display_Clear();
  EPD_sleep();

  LOG_PRINTLN(F("[GDEP073E01] Demo finished. Display is in deep sleep."));
}

void loop()
{
}
