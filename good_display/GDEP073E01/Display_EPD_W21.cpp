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
      LOG_PRINTF("[GDEP073E01] BUSY timeout during: %s! (pin=%d)\n", label, (int)isEPD_W21_BUSY);
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
  EPD_W21_WriteCMD(POF);     // 0x02 Power off
  EPD_W21_WriteDATA(0x00);
  delay(20);
  lcd_chkstatus("Power OFF");

  EPD_W21_WriteCMD(0X07);    // Deep sleep
  EPD_W21_WriteDATA(0xA5);
}

// Convert Good Display raw byte image value to 4-bit ED2208 hardware color
static unsigned char Color_get(unsigned char color)
{
  switch (color)
  {
    case 0x00: return COLOR_BLACK;
    case 0xff: return COLOR_WHITE;
    case 0xfc: return COLOR_YELLOW;
    case 0xe0: return COLOR_RED;
    case 0x03: return COLOR_BLUE;
    case 0x1c: return COLOR_GREEN;
    default:   return COLOR_BLACK;
  }
}

// Standard ED2208 display refresh
void EPD_refresh(void)
{
  EPD_W21_WriteCMD(0x50); // CDI
  EPD_W21_WriteDATA(0x3F);

  EPD_W21_WriteCMD(0x12); // DRF (Display Refresh)
  EPD_W21_WriteDATA(0x00);
  delay(20);              // 20ms delay ensures ED2208 asserts BUSY LOW
  lcd_chkstatus("EPD_refresh DRF");
}

// Display 1-byte-per-pixel array (384,000 bytes) packed to 4-bit nibbles
void PIC_display(const unsigned char* picData)
{
  EPD_W21_WriteCMD(DTM); // 0x10

  EPD_W21_CS_0;
  EPD_W21_DC_1;
  for (unsigned int i = 0; i < 480; i++)
  {
    unsigned int k = 0;
    for (unsigned int j = 0; j < 800 / 2; j++)
    {
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

void PIC_display_Clear(void)
{
  EPD_W21_WriteCMD(DTM);

  EPD_W21_CS_0;
  EPD_W21_DC_1;
  for (unsigned int i = 0; i < 480; i++)
  {
    for (unsigned int j = 0; j < 800 / 2; j++)
    {
      SPI_Write(White);
    }
  }
  EPD_W21_CS_1;

  EPD_refresh();
}

static void EPD_Display_Color_Internal(unsigned char color_byte)
{
  EPD_W21_WriteCMD(DTM); // 0x10

  EPD_W21_CS_0;
  EPD_W21_DC_1;
  for (unsigned long i = 0; i < 192000; i++)
  {
    SPI_Write(color_byte);
  }
  EPD_W21_CS_1;

  EPD_refresh();
}

void EPD_Display_White(void)  { EPD_Display_Color_Internal(White); }
void EPD_Display_Black(void)  { EPD_Display_Color_Internal(Black); }
void EPD_Display_Yellow(void) { EPD_Display_Color_Internal(Yellow); }
void EPD_Display_blue(void)   { EPD_Display_Color_Internal(Blue); }
void EPD_Display_Green(void)  { EPD_Display_Color_Internal(Green); }
void EPD_Display_red(void)    { EPD_Display_Color_Internal(Red); }
