#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <Arduino.h>

// reTerminal E1002 carrier board USB-to-UART bridge:
// Serial1 on GPIO 43 (TX) and GPIO 44 (RX).
// Serial is native USB CDC on XIAO ESP32-S3.
#define LOG_BAUD    115200
#define LOG_TX_PIN  43
#define LOG_RX_PIN  44

#define LOG_PRINT(x)    do { Serial1.print(x);    Serial.print(x);    } while(0)
#define LOG_PRINTLN(x)  do { Serial1.println(x);  Serial.println(x);  } while(0)
#define LOG_PRINTF(...) do { Serial1.printf(__VA_ARGS__); Serial.printf(__VA_ARGS__); } while(0)

#define USE_DEBUG 1
#if USE_DEBUG
    #define Debug(__info, ...) LOG_PRINTF("Debug: " __info, ##__VA_ARGS__)
#else
    #define Debug(__info, ...)
#endif

#endif
