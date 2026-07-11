// SHT4x.ino
//
// Read temperature and humidity from the onboard SHT4x sensor on the
// Seeed Studio reTerminal E1002.
//
// The SHT40 / SHT41 / SHT45 are high-accuracy I2C temperature & humidity
// sensors from Sensirion.  This example reads a measurement every 2 seconds
// and prints it to both UART1 and USB CDC.
//
// Required library (install via Arduino Library Manager):
//   "Adafruit SHT4x Library" by Adafruit
//   (depends on "Adafruit BusIO" — install that too if prompted)
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
#include "Adafruit_SHT4x.h"

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

Adafruit_SHT4x sht4;

void setup()
{
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    LOG_PRINTLN(F("\n[E1002] SHT4x temperature & humidity demo"));

    Wire.begin(I2C_SDA, I2C_SCL);

    if (!sht4.begin(&Wire)) {
        LOG_PRINTLN(F("[SHT4x] Sensor not found — check wiring and I2C address."));
        while (true) delay(1000);
    }

    LOG_PRINTF("[SHT4x] Found sensor: serial 0x%08X\n", sht4.readSerial());

    // Precision modes: SHT4X_HIGH_PRECISION / SHT4X_MED_PRECISION / SHT4X_LOW_PRECISION
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    // Heater: SHT4X_NO_HEATER (normal use) or SHT4X_HIGH_MED/LOW_HEATER_*
    sht4.setHeater(SHT4X_NO_HEATER);
}

void loop()
{
    sensors_event_t humidity, temp;

    uint32_t t = millis();
    sht4.getEvent(&humidity, &temp);
    uint32_t elapsed = millis() - t;

    LOG_PRINTF("[SHT4x] Temp: %.2f °C  Humidity: %.2f %%RH  (took %u ms)\n",
               temp.temperature,
               humidity.relative_humidity,
               elapsed);

    delay(2000);
}
