/*****************************************************************************
* | File        :   DEV_Config.h
* | Author      :   Waveshare team / adapted for Seeed reTerminal E1002
* | Function    :   Hardware underlying interface
* | Info        :
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>
#include <SPI.h>

/**
 * data
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * GPIO config for Seeed Studio reTerminal E1002 (XIAO ESP32-S3):
 *   SCK  -> GPIO 7
 *   MOSI -> GPIO 9
 *   CS   -> GPIO 10
 *   DC   -> GPIO 11
 *   RST  -> GPIO 12
 *   BUSY -> GPIO 13 (Active LOW on ED2208: 0=busy, 1=idle)
**/
#define EPD_SCK_PIN     7
#define EPD_MOSI_PIN    9
#define EPD_CS_PIN      10
#define EPD_DC_PIN      11
#define EPD_RST_PIN     12
#define EPD_BUSY_PIN    13

#define GPIO_PIN_SET    1
#define GPIO_PIN_RESET  0

/**
 * GPIO read and write
**/
#define DEV_Digital_Write(_pin, _value) digitalWrite(_pin, _value == 0 ? LOW : HIGH)
#define DEV_Digital_Read(_pin) digitalRead(_pin)

/**
 * delay x ms
**/
#define DEV_Delay_ms(__xms) delay(__xms)

/*------------------------------------------------------------------------------------------------------*/
UBYTE DEV_Module_Init(void);
void DEV_GPIO_Init(void);
void DEV_SPI_Init(void);

void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode);
void DEV_SPI_WriteByte(UBYTE data);
void DEV_SPI_SendByte(UBYTE data);
UBYTE DEV_SPI_ReadByte(void);
void DEV_SPI_Write_nByte(const UBYTE *pData, UDOUBLE len);
void DEV_Module_Exit(void);

#endif
