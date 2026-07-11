// RTC.ino
//
// Read and set the onboard RTC (PCF8563) on the Seeed Studio reTerminal E1002.
//
// The PCF8563 is a low-power I2C real-time clock / calendar with alarm and
// timer functions.  This example shows how to set the time once at startup
// and then read it every second.
//
// Required library (install via Arduino Library Manager):
//   "PCF8563 RTC" by Seeed Studio  (search for "PCF8563")
//   OR use any PCF8563-compatible library.
//
// Required Arduino settings (Tools menu):
//   Board        : XIAO_ESP32S3
//   Upload Speed : 115200
//
// Pin mapping (I2C):
//   GPIO 19  SDA
//   GPIO 20  SCL
//   GPIO 43  UART1 TX  (carrier board USB-UART bridge)
//   GPIO 44  UART1 RX

#include <Arduino.h>
#include <Wire.h>
#include "PCF8563.h"   // Install: "PCF8563 RTC" by Seeed Studio

// ===== Serial / logging =====================================================
#define LOG_BAUD    115200
#define LOG_TX_PIN  43
#define LOG_RX_PIN  44
#define LOG_PRINT(x)    do { Serial1.print(x);    Serial.print(x);    } while(0)
#define LOG_PRINTLN(x)  do { Serial1.println(x);  Serial.println(x);  } while(0)
#define LOG_PRINTF(...) do { Serial1.printf(__VA_ARGS__); Serial.printf(__VA_ARGS__); } while(0)

// ===== I2C pins =============================================================
#define I2C_SDA  19
#define I2C_SCL  20

PCF8563 rtc;

// Set this to the current time before compiling.
// Format: year (2-digit), month, day, weekday (0=Sun), hour, minute, second
static const int kSetYear    = 25;
static const int kSetMonth   =  7;
static const int kSetDay     = 10;
static const int kSetHour    = 12;
static const int kSetMinute  =  0;
static const int kSetSecond  =  0;

// Set to false after the first flash to avoid resetting time on every reboot.
static const bool kSetTime = true;

void setup()
{
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    LOG_PRINTLN(F("\n[E1002] RTC demo (PCF8563)"));

    Wire.begin(I2C_SDA, I2C_SCL);
    rtc.init();

    if (kSetTime) {
        LOG_PRINTF("[rtc] Setting time to 20%02d-%02d-%02d %02d:%02d:%02d\n",
                   kSetYear, kSetMonth, kSetDay,
                   kSetHour, kSetMinute, kSetSecond);
        rtc.stopClock();
        rtc.setYear(kSetYear);
        rtc.setMonth(kSetMonth);
        rtc.setDay(kSetDay);
        // Note: this PCF8563 library version has no setWeekday() method.
        rtc.setHour(kSetHour);
        rtc.setMinut(kSetMinute);
        rtc.setSecond(kSetSecond);
        rtc.startClock();
    }
}

void loop()
{
    Time now = rtc.getTime();

    LOG_PRINTF("[rtc] 20%02d-%02d-%02d %02d:%02d:%02d\n",
               now.year, now.month, now.day,
               now.hour, now.minute, now.second);

    delay(1000);
}
