#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"

void EPD_W21_Init(void)
{
  EPD_W21_RST_1;
  delay(50);
  EPD_W21_RST_0;    // Module reset
  delay(20);        // At least 20ms delay
  EPD_W21_RST_1;
  delay(20);
  // Do not wait for busy on raw reset: controller is uninitialized
}

// Busy check with 40s timeout (ED2208: 0 = busy, 1 = ready/idle)
void lcd_chkstatus(const char* label)
{
  unsigned long start = millis();
  while (!isEPD_W21_BUSY) {
    if (millis() - start > 40000) {
      LOG_PRINTF("[GDEP073E01_Partial] BUSY timeout during: %s! (pin=%d)\n", label, (int)isEPD_W21_BUSY);
      break;
    }
    delay(5);
  }
}

// Standard ED2208 initialization
void EPD_init(void)
{
  EPD_W21_Init();

  EPD_W21_WriteCMD(0xAA);    // CMDH
  EPD_W21_WriteDATA(0x49);
  EPD_W21_WriteDATA(0x55);
  EPD_W21_WriteDATA(0x20);
  EPD_W21_WriteDATA(0x08);
  EPD_W21_WriteDATA(0x09);
  EPD_W21_WriteDATA(0x18);

  EPD_W21_WriteCMD(PWRR);
  EPD_W21_WriteDATA(0x3F);

  EPD_W21_WriteCMD(PSR);
  EPD_W21_WriteDATA(0x5F);
  EPD_W21_WriteDATA(0x69);

  EPD_W21_WriteCMD(POFS);
  EPD_W21_WriteDATA(0x00);
  EPD_W21_WriteDATA(0x54);
  EPD_W21_WriteDATA(0x00);
  EPD_W21_WriteDATA(0x44);

  EPD_W21_WriteCMD(BTST1);
  EPD_W21_WriteDATA(0x40);
  EPD_W21_WriteDATA(0x1F);
  EPD_W21_WriteDATA(0x1F);
  EPD_W21_WriteDATA(0x2C);

  EPD_W21_WriteCMD(BTST2);
  EPD_W21_WriteDATA(0x6F);
  EPD_W21_WriteDATA(0x1F);
  EPD_W21_WriteDATA(0x17);
  EPD_W21_WriteDATA(0x49);

  EPD_W21_WriteCMD(BTST3);
  EPD_W21_WriteDATA(0x6F);
  EPD_W21_WriteDATA(0x1F);
  EPD_W21_WriteDATA(0x1F);
  EPD_W21_WriteDATA(0x22);

  EPD_W21_WriteCMD(PLL);
  EPD_W21_WriteDATA(0x08);

  EPD_W21_WriteCMD(CDI);
  EPD_W21_WriteDATA(0x3F);

  EPD_W21_WriteCMD(TCON);
  EPD_W21_WriteDATA(0x02);
  EPD_W21_WriteDATA(0x00);

  EPD_W21_WriteCMD(TRES);
  EPD_W21_WriteDATA(0x03);
  EPD_W21_WriteDATA(0x20);
  EPD_W21_WriteDATA(0x01);
  EPD_W21_WriteDATA(0xE0);

  EPD_W21_WriteCMD(T_VDCS);
  EPD_W21_WriteDATA(0x01);

  EPD_W21_WriteCMD(PWS);
  EPD_W21_WriteDATA(0x2F);

  EPD_W21_WriteCMD(0x04);     // PWR on
  delay(20);
  lcd_chkstatus("EPD_init PWR_ON");
}

void EPD_sleep(void)
{
  EPD_W21_WriteCMD(POF);
  EPD_W21_WriteDATA(0x00);
  lcd_chkstatus("EPD_sleep POF");
}

static unsigned char Color_get(unsigned char color)
{
  switch (color) {
    case 0x00: return COLOR_BLACK;
    case 0xff: return COLOR_WHITE;
    case 0xfc: return COLOR_YELLOW;
    case 0xe0: return COLOR_RED;
    case 0x03: return COLOR_BLUE;
    case 0x1c: return COLOR_GREEN;
    default:   return COLOR_BLACK;
  }
}

// Display 800x480 full screen 6-color image from flash
void PIC_display(const unsigned char* picData)
{
  EPD_W21_WriteCMD(DTM); // 0x10

  EPD_W21_CS_0;
  EPD_W21_DC_1;
  for (unsigned int i = 0; i < 480; i++) {
    unsigned int k = 0;
    for (unsigned int j = 0; j < 800 / 2; j++) {
      unsigned char temp1 = picData[i * 800 + k++];
      unsigned char temp2 = picData[i * 800 + k++];
      unsigned char data = (Color_get(temp1) << 4) | Color_get(temp2);
      SPI_Write(data);
    }
    if (i % 60 == 0) yield();
  }
  EPD_W21_CS_1;

  EPD_refresh();
}

// Clear screen to white
void PIC_display_Clear(void)
{
  EPD_W21_WriteCMD(DTM);

  EPD_W21_CS_0;
  EPD_W21_DC_1;
  for (unsigned int i = 0; i < 480; i++) {
    for (unsigned int j = 0; j < 800 / 2; j++) {
      SPI_Write(White);
    }
  }
  EPD_W21_CS_1;

  EPD_refresh();
}

// Standard ED2208 display refresh
void EPD_refresh(void)
{
  EPD_W21_WriteCMD(0x50); // CDI
  EPD_W21_WriteDATA(0x3F);

  EPD_W21_WriteCMD(DRF); // 0x12
  EPD_W21_WriteDATA(0x00);
  delay(20);             // 20ms delay ensures ED2208 asserts BUSY LOW
  lcd_chkstatus("EPD_refresh DRF");
}

static void display_solid_color(unsigned char color_byte)
{
  EPD_W21_WriteCMD(DTM);
  EPD_W21_CS_0;
  EPD_W21_DC_1;
  for (unsigned long i = 0; i < 192000; i++) {
    SPI_Write(color_byte);
  }
  EPD_W21_CS_1;

  EPD_refresh();
}

void EPD_Display_White(void)  { display_solid_color(White); }
void EPD_Display_Black(void)  { display_solid_color(Black); }
void EPD_Display_Yellow(void) { display_solid_color(Yellow); }
void EPD_Display_blue(void)   { display_solid_color(Blue); }
void EPD_Display_Green(void)  { display_solid_color(Green); }
void EPD_Display_red(void)    { display_solid_color(Red); }

// Partial window update (Specified area update) via ED2208 CMD 0x83
void EPD_PartialWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color)
{
  uint16_t x_end = x + width - 1;
  uint16_t y_end = y + height - 1;

  // Command 0x83: Partial Window coordinate setup
  EPD_W21_WriteCMD(CMD_PARTIAL_WINDOW);
  EPD_W21_WriteDATA((x >> 8) & 0x03);
  EPD_W21_WriteDATA(x & 0xFF);
  EPD_W21_WriteDATA((x_end >> 8) & 0x03);
  EPD_W21_WriteDATA(x_end & 0xFF);
  EPD_W21_WriteDATA((y >> 8) & 0x03);
  EPD_W21_WriteDATA(y & 0xFF);
  EPD_W21_WriteDATA((y_end >> 8) & 0x03);
  EPD_W21_WriteDATA(y_end & 0xFF);
  EPD_W21_WriteDATA(0x01); // Enable partial mode

  // Send pixel data for specified window
  EPD_W21_WriteCMD(DTM); // 0x10
  EPD_W21_WriteDATA(0x00);

  EPD_W21_CS_0;
  EPD_W21_DC_1;
  for (uint16_t j = 0; j < height; j++) {
    for (uint16_t i = 0; i < width; i += 2) {
      uint8_t data;
      if (i + 1 < width) {
        data = (color << 4) | (color & 0x0F);
      } else {
        data = (color << 4);
      }
      SPI_Write(data);
    }
    if (j % 10 == 0) yield();
  }
  EPD_W21_CS_1;

  // Refresh only the specified partial window
  EPD_refresh();
}
