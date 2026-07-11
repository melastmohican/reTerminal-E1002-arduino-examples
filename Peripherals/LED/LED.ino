// LED.ino
//
// Control the onboard user LED on the Seeed Studio reTerminal E1002.
//
// The LED is on GPIO6 and uses INVERTED logic — drive LOW to turn ON,
// HIGH to turn OFF.
//
// Required Arduino settings (Tools menu):
//   Board        : XIAO_ESP32S3
//   Upload Speed : 115200
//
// Pin mapping:
//   GPIO  6  User LED (active-LOW / inverted)
//   GPIO 43  UART1 TX  (carrier board USB-UART bridge)
//   GPIO 44  UART1 RX

#include <Arduino.h>

// ===== Serial / logging =====================================================
#define LOG_BAUD    115200
#define LOG_TX_PIN  43
#define LOG_RX_PIN  44
#define LOG_PRINT(x)    do { Serial1.print(x);    Serial.print(x);    } while(0)
#define LOG_PRINTLN(x)  do { Serial1.println(x);  Serial.println(x);  } while(0)
#define LOG_PRINTF(...) do { Serial1.printf(__VA_ARGS__); Serial.printf(__VA_ARGS__); } while(0)

// ===== LED ==================================================================
// GPIO6, inverted: LOW = ON, HIGH = OFF
#define LED_PIN  6

static void ledOn()  { digitalWrite(LED_PIN, LOW);  }
static void ledOff() { digitalWrite(LED_PIN, HIGH); }

void setup()
{
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    LOG_PRINTLN(F("\n[E1002] LED demo (GPIO6, active-LOW)"));

    pinMode(LED_PIN, OUTPUT);
    ledOff();  // start with LED off (HIGH)
}

void loop()
{
    // ----- Simple blink -----
    LOG_PRINTLN(F("[led] ON"));
    ledOn();
    delay(500);

    LOG_PRINTLN(F("[led] OFF"));
    ledOff();
    delay(500);

    // ----- Slow fade using analogWrite (LEDC) -----
    // analogWrite maps 0–255 to the PWM duty cycle.
    // Because the LED is inverted, 0 = full brightness, 255 = off.
    LOG_PRINTLN(F("[led] Fade on"));
    for (int v = 255; v >= 0; v--) {
        analogWrite(LED_PIN, v);
        delay(5);
    }

    LOG_PRINTLN(F("[led] Fade off"));
    for (int v = 0; v <= 255; v++) {
        analogWrite(LED_PIN, v);
        delay(5);
    }

    delay(200);
}
