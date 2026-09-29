/*****************************************************************************
* | File        :   DEV_Config.cpp
* | Author      :   Waveshare team / adapted for Seeed reTerminal E1002
* | Function    :   Hardware underlying interface
* | Info        :
******************************************************************************/
#include "DEV_Config.h"
#include "Debug.h"
#include <SPI.h>

static SPIClass epd_spi(HSPI);

void GPIO_Config(void)
{
    pinMode(EPD_BUSY_PIN, INPUT);
    pinMode(EPD_RST_PIN,  OUTPUT);
    pinMode(EPD_DC_PIN,   OUTPUT);
    pinMode(EPD_CS_PIN,   OUTPUT);

    digitalWrite(EPD_CS_PIN, HIGH);
    digitalWrite(EPD_RST_PIN, HIGH);
}

void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode)
{
    if (Mode == 0) {
        pinMode(GPIO_Pin, INPUT);
    } else {
        pinMode(GPIO_Pin, OUTPUT);
    }
}

UBYTE DEV_Module_Init(void)
{
    GPIO_Config();

    // Bring up Serial1 (E1002 carrier board USB-UART on GPIO 43/44) and Serial (USB CDC)
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    // 2 MHz SPI clock matching GxEPD2 and reTerminal E1002 hardware specifications
    epd_spi.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, -1);
    epd_spi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

    return 0;
}

void DEV_GPIO_Init(void)
{
    epd_spi.end();
    pinMode(EPD_SCK_PIN, OUTPUT);
    pinMode(EPD_MOSI_PIN, OUTPUT);
}

void DEV_SPI_Init(void)
{
    epd_spi.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, -1);
    epd_spi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
}

void DEV_SPI_WriteByte(UBYTE data)
{
    epd_spi.transfer(data);
}

void DEV_SPI_Write_nByte(const UBYTE *pData, UDOUBLE len)
{
    for (UDOUBLE i = 0; i < len; i++) {
        epd_spi.transfer(pData[i]);
    }
}

void DEV_SPI_SendByte(UBYTE data)
{
    GPIO_Mode(EPD_MOSI_PIN, OUTPUT);
    digitalWrite(EPD_CS_PIN, GPIO_PIN_RESET);
    for (int i = 0; i < 8; i++)
    {
        if ((data & 0x80) == 0) digitalWrite(EPD_MOSI_PIN, GPIO_PIN_RESET);
        else                    digitalWrite(EPD_MOSI_PIN, GPIO_PIN_SET);

        data <<= 1;
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_SET);
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_RESET);
    }
    digitalWrite(EPD_CS_PIN, GPIO_PIN_SET);
}

UBYTE DEV_SPI_ReadByte(void)
{
    UBYTE j = 0xff;
    GPIO_Mode(EPD_MOSI_PIN, INPUT);
    digitalWrite(EPD_CS_PIN, GPIO_PIN_RESET);
    for (int i = 0; i < 8; i++)
    {
        j = j << 1;
        if (digitalRead(EPD_MOSI_PIN))  j = j | 0x01;
        else                            j = j & 0xfe;

        digitalWrite(EPD_SCK_PIN, GPIO_PIN_SET);
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_RESET);
    }
    digitalWrite(EPD_CS_PIN, GPIO_PIN_SET);
    GPIO_Mode(EPD_MOSI_PIN, OUTPUT);
    return j;
}

void DEV_Module_Exit(void)
{
}
