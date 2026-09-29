/*****************************************************************************
* | File        :   Waveshare_7in3e.ino
* | Function    :   7.3inch e-Paper (E) demo - 800x480, 6-color ACeP
* | Info        :
*----------------
* Adapted from Waveshare's official 7.3inch e-Paper (E) demo to run on Seeed
* Studio reTerminal E1002 (XIAO ESP32-S3).
*
* Board       :   XIAO_ESP32S3 (esp32:esp32:XIAO_ESP32S3)
* Panel       :   Waveshare 7.3inch e-Paper (E) / Good Display GDEP073E01
*                 800x480, ED2208 controller, 6 native colors:
*                 Black, White, Yellow, Red, Blue, Green (no orange)
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
#include "EPD_7in3e.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "ImageData.h"

void setup()
{
    DEV_Module_Init();
    LOG_PRINTLN();
    LOG_PRINTLN(F("=================================================="));
    LOG_PRINTLN(F(" Waveshare 7.3inch e-Paper (E) Demo (reTerminal)  "));
    LOG_PRINTLN(F(" 800x480 6-Color ACeP (ED2208 Controller)        "));
    LOG_PRINTLN(F("=================================================="));

    // 1. Display built-in 800x480 bitmap
    LOG_PRINTLN(F("[Waveshare 7.3E] 1. Displaying built-in 800x480 bitmap (gImage_7in3e)..."));
    EPD_7IN3E_Init();
    EPD_7IN3E_Display(gImage_7in3e);
    LOG_PRINTLN(F("[Waveshare 7.3E] Holding bitmap for 10 seconds..."));
    DEV_Delay_ms(10000);

    // 2. Display 6 native color stripes
    LOG_PRINTLN(F("[Waveshare 7.3E] 2. Displaying 6 native color stripes..."));
    EPD_7IN3E_Init();
    EPD_7IN3E_Show7Block();
    LOG_PRINTLN(F("[Waveshare 7.3E] Holding stripes for 10 seconds..."));
    DEV_Delay_ms(10000);

    // 3. In-memory rendering using GUI_Paint
    UBYTE *BlackImage = NULL;
    UDOUBLE Imagesize = ((EPD_7IN3E_WIDTH % 2 == 0) ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1)) * EPD_7IN3E_HEIGHT;

#if defined(BOARD_HAS_PSRAM)
    BlackImage = (UBYTE *)ps_malloc(Imagesize);
#endif
    if (BlackImage == NULL) {
        BlackImage = (UBYTE *)malloc(Imagesize);
    }

    if (BlackImage == NULL) {
        LOG_PRINTLN(F("Failed to allocate image memory for GUI_Paint!"));
        while (1);
    }

    LOG_PRINTLN(F("[Waveshare 7.3E] 3. Rendering 800x480 graphics with GUI_Paint..."));
    Paint_NewImage(BlackImage, EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT, 0, EPD_7IN3E_WHITE);
    Paint_SetScale(6);
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3E_WHITE);

    // Top title banner
    Paint_DrawRectangle(0, 0, 800, 48, EPD_7IN3E_BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(20, 12, "Seeed Studio reTerminal E1002 - Waveshare 7.3\" (E)", &Font20, EPD_7IN3E_WHITE, EPD_7IN3E_BLUE);

    // Top color strip (all 6 colors)
    int sw = 800 / 6;
    const UBYTE colors6[6] = {EPD_7IN3E_BLACK, EPD_7IN3E_WHITE, EPD_7IN3E_YELLOW, EPD_7IN3E_RED, EPD_7IN3E_BLUE, EPD_7IN3E_GREEN};
    for (int i = 0; i < 6; i++) {
        Paint_DrawRectangle(i * sw, 48, (i + 1) * sw, 56, colors6[i], DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    // Typography section
    Paint_DrawString_EN(30, 75, "7.3-inch Full Color ACeP (ED2208) - 800 x 480", &Font20, EPD_7IN3E_BLACK, EPD_7IN3E_WHITE);
    Paint_DrawString_EN(30, 105, "Red Text Sample - Alerts & Highlights", &Font16, EPD_7IN3E_RED, EPD_7IN3E_WHITE);
    Paint_DrawString_EN(30, 130, "Green Text Sample - Status & Success", &Font16, EPD_7IN3E_GREEN, EPD_7IN3E_WHITE);
    Paint_DrawString_EN(30, 155, "Blue Text Sample - Navigation & Accents", &Font16, EPD_7IN3E_BLUE, EPD_7IN3E_WHITE);
    Paint_DrawString_EN(30, 180, "Yellow Text on Black Background", &Font16, EPD_7IN3E_YELLOW, EPD_7IN3E_BLACK);

    // Inverted color boxes
    Paint_DrawRectangle(30, 210, 160, 250, EPD_7IN3E_RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(42, 222, "RED BOX", &Font16, EPD_7IN3E_WHITE, EPD_7IN3E_RED);

    Paint_DrawRectangle(175, 210, 305, 250, EPD_7IN3E_GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(182, 222, "GREEN BOX", &Font16, EPD_7IN3E_WHITE, EPD_7IN3E_GREEN);

    Paint_DrawRectangle(320, 210, 450, 250, EPD_7IN3E_BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(332, 222, "BLUE BOX", &Font16, EPD_7IN3E_WHITE, EPD_7IN3E_BLUE);

    Paint_DrawRectangle(465, 210, 595, 250, EPD_7IN3E_YELLOW, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(472, 222, "YELLOW BOX", &Font16, EPD_7IN3E_BLACK, EPD_7IN3E_YELLOW);

    Paint_DrawRectangle(610, 210, 740, 250, EPD_7IN3E_BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(620, 222, "BLACK BOX", &Font16, EPD_7IN3E_WHITE, EPD_7IN3E_BLACK);

    // Geometric primitives
    Paint_DrawLine(30, 275, 770, 275, EPD_7IN3E_RED, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    // Shapes: Rectangles and Circles
    Paint_DrawRectangle(50, 300, 150, 380, EPD_7IN3E_BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(65, 315, 135, 365, EPD_7IN3E_RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawCircle(230, 340, 40, EPD_7IN3E_BLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(230, 340, 25, EPD_7IN3E_YELLOW, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Concentric circles (all colors)
    for (int r = 0; r < 5; r++) {
        Paint_DrawCircle(360, 340, 45 - r * 8, colors6[r], DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }

    // Chinese typography & numerals
    Paint_DrawString_CN(460, 305, "微雪电子", &Font24CN, EPD_7IN3E_RED, EPD_7IN3E_WHITE);
    Paint_DrawString_CN(460, 345, "你好世界", &Font24CN, EPD_7IN3E_BLUE, EPD_7IN3E_WHITE);
    Paint_DrawNum(620, 315, 800480, &Font20, EPD_7IN3E_BLACK, EPD_7IN3E_WHITE);
    Paint_DrawNum(620, 345, 2026, &Font20, EPD_7IN3E_GREEN, EPD_7IN3E_WHITE);

    // Footer
    Paint_DrawRectangle(0, 440, 800, 480, EPD_7IN3E_BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(200, 452, "Waveshare Driver Port - reTerminal E1002", &Font16, EPD_7IN3E_WHITE, EPD_7IN3E_BLACK);

    // Send rendered buffer to screen
    LOG_PRINTLN(F("[Waveshare 7.3E] Sending rendered image to display..."));
    EPD_7IN3E_Init();
    EPD_7IN3E_Display(BlackImage);
    free(BlackImage);
    BlackImage = NULL;
    LOG_PRINTLN(F("[Waveshare 7.3E] Holding GUI_Paint render for 10 seconds..."));
    DEV_Delay_ms(10000);

    // 4. Clear to White and deep sleep
    LOG_PRINTLN(F("[Waveshare 7.3E] 4. Clearing screen to White and entering sleep..."));
    EPD_7IN3E_Init();
    EPD_7IN3E_Clear(EPD_7IN3E_WHITE);
    EPD_7IN3E_Sleep();

    DEV_Module_Exit();
    LOG_PRINTLN(F("[Waveshare 7.3E] Demo finished successfully. Display in deep sleep."));
}

void loop()
{
}
