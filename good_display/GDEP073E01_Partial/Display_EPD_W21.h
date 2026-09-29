#ifndef _DISPLAY_EPD_W21_H_
#define _DISPLAY_EPD_W21_H_
#include <Arduino.h>

#define EPD_WIDTH   800
#define EPD_HEIGHT  480

// 8-bit packed colors (2 pixels of same color per byte)
#define Black   0x00  
#define White   0x11  
#define Yellow  0x22 
#define Red     0x33  
#define Blue    0x55  
#define Green   0x66  
#define Clean   0x77  

// 4-bit single pixel colors
#define COLOR_BLACK   0x00  
#define COLOR_WHITE   0x01  
#define COLOR_YELLOW  0x02 
#define COLOR_RED     0x03  
#define COLOR_BLUE    0x05  
#define COLOR_GREEN   0x06  
#define COLOR_CLEAN   0x07  

// ED2208 Register Commands
#define PSR         0x00
#define PWRR        0x01
#define POF         0x02
#define POFS        0x03
#define PON         0x04
#define BTST1       0x05
#define BTST2       0x06
#define DSLP        0x07
#define BTST3       0x08
#define DTM         0x10
#define DRF         0x12
#define PLL         0x30
#define CDI         0x50
#define TCON        0x60
#define TRES        0x61
#define REV         0x70
#define CMD_PARTIAL_WINDOW  0x83
#define VDCS        0x82
#define T_VDCS      0x84
#define PWS         0xE3

// Driver functions
void EPD_W21_Init(void);
void EPD_init(void);
void EPD_sleep(void);
void EPD_refresh(void);
void lcd_chkstatus(const char* label = "unknown");
void PIC_display(const unsigned char* picData);
void PIC_display_Clear(void);
void EPD_Display_White(void);
void EPD_Display_Black(void);
void EPD_Display_Yellow(void);
void EPD_Display_blue(void);
void EPD_Display_Green(void);
void EPD_Display_red(void);
void EPD_PartialWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);

#endif
