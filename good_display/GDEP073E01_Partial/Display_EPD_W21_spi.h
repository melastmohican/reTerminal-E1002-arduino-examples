#ifndef _DISPLAY_EPD_W21_SPI_
#define _DISPLAY_EPD_W21_SPI_
#include <Arduino.h>

// reTerminal E1002 pin mapping (XIAO ESP32-S3):
//   SCK=7, MOSI=9, CS=10, DC=11, RST=12, BUSY=13
#define EPD_SCK_PIN   7
#define EPD_MOSI_PIN  9
#define EPD_CS_PIN    10
#define EPD_DC_PIN    11
#define EPD_RST_PIN   12
#define EPD_BUSY_PIN  13

// reTerminal E1002 carrier board USB-to-UART bridge:
// Serial1 on GPIO 43 (TX) and GPIO 44 (RX).
// Serial is native USB CDC on XIAO ESP32-S3.
#define LOG_BAUD    115200
#define LOG_TX_PIN  43
#define LOG_RX_PIN  44

#define LOG_PRINT(x)    do { Serial1.print(x);    Serial.print(x);    } while(0)
#define LOG_PRINTLN(x)  do { Serial1.println(x);  Serial.println(x);  } while(0)
#define LOG_PRINTF(...) do { Serial1.printf(__VA_ARGS__); Serial.printf(__VA_ARGS__); } while(0)

#define isEPD_W21_BUSY digitalRead(EPD_BUSY_PIN)  // BUSY: LOW=busy, HIGH=idle
#define EPD_W21_RST_0  digitalWrite(EPD_RST_PIN, LOW)
#define EPD_W21_RST_1  digitalWrite(EPD_RST_PIN, HIGH)
#define EPD_W21_DC_0   digitalWrite(EPD_DC_PIN, LOW)
#define EPD_W21_DC_1   digitalWrite(EPD_DC_PIN, HIGH)
#define EPD_W21_CS_0   digitalWrite(EPD_CS_PIN, LOW)
#define EPD_W21_CS_1   digitalWrite(EPD_CS_PIN, HIGH)

void EPD_SPI_Init(void);
void SPI_Write(unsigned char value);
void EPD_W21_WriteDATA(unsigned char datas);
void EPD_W21_WriteCMD(unsigned char command);

#endif
