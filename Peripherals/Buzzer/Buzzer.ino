// Buzzer.ino
//
// Drive the onboard passive buzzer on the Seeed Studio reTerminal E1002.
//
// The buzzer is connected to GPIO2 and is driven with a simple square wave
// via the ESP32 LEDC (LED Control) peripheral, which provides hardware PWM
// at any desired frequency without blocking the CPU.
//
// Required Arduino settings (Tools menu):
//   Board        : XIAO_ESP32S3
//   Upload Speed : 115200
//
// Pin mapping:
//   GPIO 45  Buzzer (passive, driven by LEDC PWM)
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

// ===== Buzzer ===============================================================
#define BUZZER_PIN  45    // GPIO45

// Simple helper: play a tone at the given frequency for the given duration.
static void beep(uint32_t freqHz, uint32_t durationMs)
{
    tone(BUZZER_PIN, freqHz, durationMs);
    delay(durationMs);
    noTone(BUZZER_PIN);
}

void setup()
{
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    LOG_PRINTLN(F("\n[E1002] Buzzer demo"));

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop()
{
    // ---- Boot chime: ascending three-note sequence ----
    LOG_PRINTLN(F("[buzz] Ascending chime"));
    beep(440, 200); delay(50);   // A4
    beep(550, 200); delay(50);   // C#5
    beep(660, 300); delay(300);  // E5

    // ---- Short double-beep ----
    LOG_PRINTLN(F("[buzz] Double beep"));
    beep(1000, 100); delay(100);
    beep(1000, 100); delay(500);

    // ---- Alert: fast low-high sweep ----
    LOG_PRINTLN(F("[buzz] Alert sweep"));
    for (int f = 500; f <= 1500; f += 50) {
        tone(BUZZER_PIN, f);
        delay(20);
    }
    noTone(BUZZER_PIN);
    delay(1000);
}
