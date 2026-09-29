#include "Display_EPD_W21_spi.h"
#include <SPI.h>

static SPIClass epd_spi(HSPI);

void EPD_SPI_Init(void)
{
    // Bring up Serial1 (carrier board USB-to-UART on GPIO 43/44) and Serial (USB CDC)
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    pinMode(EPD_BUSY_PIN, INPUT);
    pinMode(EPD_RST_PIN,  OUTPUT);
    pinMode(EPD_DC_PIN,   OUTPUT);
    pinMode(EPD_CS_PIN,   OUTPUT);

    digitalWrite(EPD_CS_PIN, HIGH);

    // 2 MHz SPI clock matching GxEPD2 and reTerminal E1002 hardware specifications
    epd_spi.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, -1);
    epd_spi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
}

void SPI_Write(unsigned char value)
{
    epd_spi.transfer(value);
}

void EPD_W21_WriteCMD(unsigned char command)
{
    EPD_W21_CS_0;
    EPD_W21_DC_0;  // 0: command
    epd_spi.transfer(command);
    EPD_W21_CS_1;
}

void EPD_W21_WriteDATA(unsigned char datas)
{
    EPD_W21_CS_0;
    EPD_W21_DC_1;  // 1: data
    epd_spi.transfer(datas);
    EPD_W21_CS_1;
}
