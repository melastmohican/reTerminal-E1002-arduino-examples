/*****************************************************************************
* | File        :   EPD_7in3e.h
* | Author      :   Waveshare team / adapted for Seeed reTerminal E1002
* | Function    :   7.3inch e-Paper (E) Driver (ED2208, 6-color ACeP)
* | Info        :
******************************************************************************/
#ifndef __EPD_7IN3E_H_
#define __EPD_7IN3E_H_

#include "Debug.h"
#include "DEV_Config.h"

// Display resolution
#define EPD_7IN3E_WIDTH       800
#define EPD_7IN3E_HEIGHT      480

/**********************************
Color Index for 7.3" 6-color ACeP panel (ED2208)
Native hardware nibble values:
**********************************/
#define EPD_7IN3E_BLACK   0x0   /// 000
#define EPD_7IN3E_WHITE   0x1   /// 001
#define EPD_7IN3E_YELLOW  0x2   /// 010
#define EPD_7IN3E_RED     0x3   /// 011
// Note: Orange (0x4) is NOT present on the 6-color GDEP073E01 / 7.3inch (E)
#define EPD_7IN3E_BLUE    0x5   /// 101
#define EPD_7IN3E_GREEN   0x6   /// 110

void EPD_7IN3E_Init(void);
void EPD_7IN3E_Init_Fast(void);
void EPD_7IN3E_Clear(UBYTE color);
void EPD_7IN3E_Show7Block(void);
void EPD_7IN3E_Display(const UBYTE *Image);
void EPD_7IN3E_DisplayPart(const UBYTE *Image, UWORD xstart, UWORD ystart, UWORD image_width, UWORD image_heigh);
void EPD_7IN3E_Sleep(void);

#endif
