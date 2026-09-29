/*****************************************************************************
* | File        :   EPD_7in3e.cpp
* | Author      :   Waveshare team / adapted for Seeed reTerminal E1002
* | Function    :   7.3inch e-Paper (E) Driver (ED2208, 6-color ACeP)
* | Info        :
******************************************************************************/
#include "EPD_7in3e.h"

/******************************************************************************
function :  Software reset
******************************************************************************/
static void EPD_7IN3E_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(50);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(20);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(20);
}

/******************************************************************************
function :  send command (CS_0 -> DC_0 -> transfer -> CS_1)
******************************************************************************/
static void EPD_7IN3E_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :  send data (CS_0 -> DC_1 -> transfer -> CS_1)
******************************************************************************/
static void EPD_7IN3E_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :  Wait until busy_pin goes HIGH (0 = busy, 1 = ready/idle)
            Includes 60s timeout protection and detailed duration logging.
******************************************************************************/
static void EPD_7IN3E_ReadBusyH(const char *label)
{
    Debug("e-Paper busy H [%s]...\r\n", label);
    unsigned long start = millis();
    while (!DEV_Digital_Read(EPD_BUSY_PIN)) {
        if (millis() - start > 60000) {
            Debug("EPD_7IN3E BUSY timeout during %s! (%lu ms, pin=%d)\r\n",
                  label, millis() - start, (int)DEV_Digital_Read(EPD_BUSY_PIN));
            break;
        }
        DEV_Delay_ms(10);
    }
    Debug("e-Paper busy H release [%s] (%lu ms)\r\n", label, millis() - start);
}

/******************************************************************************
function :  Turn On Display (Trigger Refresh)
            CRITICAL: ED2208 requires a 50ms delay after 0x12 before BUSY
            asserts LOW. Without this delay, ReadBusyH returns immediately and
            subsequent commands corrupt the panel into red/brown noise bands!
******************************************************************************/
static void EPD_7IN3E_TurnOnDisplay(void)
{
    EPD_7IN3E_SendCommand(0x50); // CDI
    EPD_7IN3E_SendData(0x3F);

    EPD_7IN3E_SendCommand(0x12); // DISPLAY_REFRESH (DRF)
    EPD_7IN3E_SendData(0x00);
    DEV_Delay_ms(50);            // Mandatory delay for hardware BUSY line assertion
    EPD_7IN3E_ReadBusyH("DRF Refresh");
}

/******************************************************************************
function :  Initialize the e-Paper registers
******************************************************************************/
void EPD_7IN3E_Init(void)
{
    EPD_7IN3E_Reset();
    DEV_Delay_ms(50);
    // Do not check busy on raw hardware reset: controller is uninitialized

    EPD_7IN3E_SendCommand(0xAA);    // CMDH
    EPD_7IN3E_SendData(0x49);
    EPD_7IN3E_SendData(0x55);
    EPD_7IN3E_SendData(0x20);
    EPD_7IN3E_SendData(0x08);
    EPD_7IN3E_SendData(0x09);
    EPD_7IN3E_SendData(0x18);

    EPD_7IN3E_SendCommand(0x01);    // PWRR
    EPD_7IN3E_SendData(0x3F);

    EPD_7IN3E_SendCommand(0x00);    // PSR
    EPD_7IN3E_SendData(0x5F);
    EPD_7IN3E_SendData(0x69);

    EPD_7IN3E_SendCommand(0x03);    // POFS
    EPD_7IN3E_SendData(0x00);
    EPD_7IN3E_SendData(0x54);
    EPD_7IN3E_SendData(0x00);
    EPD_7IN3E_SendData(0x44);

    EPD_7IN3E_SendCommand(0x05);    // BTST1
    EPD_7IN3E_SendData(0x40);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x2C);

    EPD_7IN3E_SendCommand(0x06);    // BTST2
    EPD_7IN3E_SendData(0x6F);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x17);
    EPD_7IN3E_SendData(0x49);

    EPD_7IN3E_SendCommand(0x08);    // BTST3
    EPD_7IN3E_SendData(0x6F);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x22);

    EPD_7IN3E_SendCommand(0x30);    // PLL: 0x08 for ED2208 (0x03 was for 7in3f!)
    EPD_7IN3E_SendData(0x08);

    EPD_7IN3E_SendCommand(0x50);    // CDI
    EPD_7IN3E_SendData(0x3F);

    EPD_7IN3E_SendCommand(0x60);    // TCON
    EPD_7IN3E_SendData(0x02);
    EPD_7IN3E_SendData(0x00);

    EPD_7IN3E_SendCommand(0x61);    // TRES
    EPD_7IN3E_SendData(0x03);
    EPD_7IN3E_SendData(0x20);
    EPD_7IN3E_SendData(0x01);
    EPD_7IN3E_SendData(0xE0);

    EPD_7IN3E_SendCommand(0x84);    // T_VDCS
    EPD_7IN3E_SendData(0x01);

    EPD_7IN3E_SendCommand(0xE3);    // PWS
    EPD_7IN3E_SendData(0x2F);

    EPD_7IN3E_SendCommand(0x04);    // PWR on
    DEV_Delay_ms(20);
    EPD_7IN3E_ReadBusyH("Init PWR_ON");
}

/******************************************************************************
function :  Initialize the e-Paper registers for Fast Refresh
******************************************************************************/
void EPD_7IN3E_Init_Fast(void)
{
    EPD_7IN3E_Reset();
    DEV_Delay_ms(50);

    EPD_7IN3E_SendCommand(0xAA);    // CMDH
    EPD_7IN3E_SendData(0x49);
    EPD_7IN3E_SendData(0x55);
    EPD_7IN3E_SendData(0x20);
    EPD_7IN3E_SendData(0x08);
    EPD_7IN3E_SendData(0x09);
    EPD_7IN3E_SendData(0x18);

    EPD_7IN3E_SendCommand(0x01);    // PWRR
    EPD_7IN3E_SendData(0x3F);
    EPD_7IN3E_SendData(0x00);
    EPD_7IN3E_SendData(0x32);
    EPD_7IN3E_SendData(0x2A);
    EPD_7IN3E_SendData(0x0E);
    EPD_7IN3E_SendData(0x2A);

    EPD_7IN3E_SendCommand(0x00);    // PSR
    EPD_7IN3E_SendData(0x5F);
    EPD_7IN3E_SendData(0x69);

    EPD_7IN3E_SendCommand(0x03);    // POFS
    EPD_7IN3E_SendData(0x00);
    EPD_7IN3E_SendData(0x54);
    EPD_7IN3E_SendData(0x00);
    EPD_7IN3E_SendData(0x44);

    EPD_7IN3E_SendCommand(0x05);    // BTST1
    EPD_7IN3E_SendData(0x40);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x2C);

    EPD_7IN3E_SendCommand(0x06);    // BTST2
    EPD_7IN3E_SendData(0x6F);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x16);
    EPD_7IN3E_SendData(0x25);

    EPD_7IN3E_SendCommand(0x08);    // BTST3
    EPD_7IN3E_SendData(0x6F);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x1F);
    EPD_7IN3E_SendData(0x22);

    EPD_7IN3E_SendCommand(0x30);    // PLL: 0x08 for ED2208 (0x03 was for 7in3f!)
    EPD_7IN3E_SendData(0x08);

    EPD_7IN3E_SendCommand(0x50);    // CDI
    EPD_7IN3E_SendData(0x3F);

    EPD_7IN3E_SendCommand(0x60);    // TCON
    EPD_7IN3E_SendData(0x02);
    EPD_7IN3E_SendData(0x00);

    EPD_7IN3E_SendCommand(0x61);    // TRES
    EPD_7IN3E_SendData(0x03);
    EPD_7IN3E_SendData(0x20);
    EPD_7IN3E_SendData(0x01);
    EPD_7IN3E_SendData(0xE0);

    EPD_7IN3E_SendCommand(0x82);    // VDCS
    EPD_7IN3E_SendData(0x1E);

    EPD_7IN3E_SendCommand(0x84);    // T_VDCS
    EPD_7IN3E_SendData(0x01);

    EPD_7IN3E_SendCommand(0x86);    // AGID
    EPD_7IN3E_SendData(0x00);

    EPD_7IN3E_SendCommand(0xE3);    // PWS
    EPD_7IN3E_SendData(0x2F);

    EPD_7IN3E_SendCommand(0xE0);    // CCSET
    EPD_7IN3E_SendData(0x00);

    EPD_7IN3E_SendCommand(0xE6);    // TSSET
    EPD_7IN3E_SendData(0x00);

    EPD_7IN3E_SendCommand(0x04);    // PWR on
    DEV_Delay_ms(20);
    EPD_7IN3E_ReadBusyH("FastInit PWR_ON");
}

/******************************************************************************
function :  Clear screen with solid color
******************************************************************************/
void EPD_7IN3E_Clear(UBYTE color)
{
    UWORD Width = (EPD_7IN3E_WIDTH % 2 == 0) ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1);
    UWORD Height = EPD_7IN3E_HEIGHT;
    UBYTE color_byte = (color << 4) | color;

    EPD_7IN3E_SendCommand(0x10);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            DEV_SPI_WriteByte(color_byte);
        }
        if (j % 30 == 0) yield();
    }
    DEV_Digital_Write(EPD_CS_PIN, 1);

    EPD_7IN3E_TurnOnDisplay();
}

/******************************************************************************
function :  Show 6 color blocks (all native colors of the 6-color panel)
******************************************************************************/
void EPD_7IN3E_Show7Block(void)
{
    const UBYTE Color_six[6] = {
        EPD_7IN3E_BLACK,
        EPD_7IN3E_YELLOW,
        EPD_7IN3E_RED,
        EPD_7IN3E_BLUE,
        EPD_7IN3E_GREEN,
        EPD_7IN3E_WHITE
    };

    EPD_7IN3E_SendCommand(0x10);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    // 800 * 480 / 2 = 192,000 bytes total. 192,000 / 6 = 32,000 bytes per stripe.
    for (int k = 0; k < 6; k++) {
        UBYTE c = (Color_six[k] << 4) | Color_six[k];
        for (unsigned long j = 0; j < 32000; j++) {
            DEV_SPI_WriteByte(c);
            if (j % 4000 == 0) yield();
        }
    }
    DEV_Digital_Write(EPD_CS_PIN, 1);

    EPD_7IN3E_TurnOnDisplay();
}

/******************************************************************************
function :  Sends the image buffer in RAM to e-Paper and displays
******************************************************************************/
void EPD_7IN3E_Display(const UBYTE *Image)
{
    UWORD Width = (EPD_7IN3E_WIDTH % 2 == 0) ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1);
    UWORD Height = EPD_7IN3E_HEIGHT;

    EPD_7IN3E_SendCommand(0x10);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            DEV_SPI_WriteByte(Image[i + j * Width]);
        }
        if (j % 30 == 0) yield();
    }
    DEV_Digital_Write(EPD_CS_PIN, 1);

    EPD_7IN3E_TurnOnDisplay();
}

/******************************************************************************
function :  Displays a sub-window of an image buffer
******************************************************************************/
void EPD_7IN3E_DisplayPart(const UBYTE *Image, UWORD xstart, UWORD ystart, UWORD image_width, UWORD image_heigh)
{
    UWORD Width = (EPD_7IN3E_WIDTH % 2 == 0) ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1);
    UWORD Height = EPD_7IN3E_HEIGHT;

    EPD_7IN3E_SendCommand(0x10);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    for (UWORD i = 0; i < Height; i++) {
        for (UWORD j = 0; j < Width; j++) {
            if (i >= ystart && i < ystart + image_heigh &&
                j >= xstart / 2 && j < (xstart + image_width) / 2) {
                DEV_SPI_WriteByte(Image[(j - xstart / 2) + (image_width / 2 * (i - ystart))]);
            } else {
                DEV_SPI_WriteByte(0x11); // White background
            }
        }
        if (i % 30 == 0) yield();
    }
    DEV_Digital_Write(EPD_CS_PIN, 1);

    EPD_7IN3E_TurnOnDisplay();
}

/******************************************************************************
function :  Enter sleep mode
******************************************************************************/
void EPD_7IN3E_Sleep(void)
{
    EPD_7IN3E_SendCommand(0x02); // POWER_OFF
    EPD_7IN3E_SendData(0x00);
    DEV_Delay_ms(50);
    EPD_7IN3E_ReadBusyH("Power OFF");

    EPD_7IN3E_SendCommand(0x07); // DEEP_SLEEP
    EPD_7IN3E_SendData(0xA5);
}
