// BuzzerWithTone.ino
//
// Play the Imperial March (Darth Vader theme) on the onboard passive buzzer
// of the Seeed Studio reTerminal E1002.
//
// Melody data adapted from https://github.com/robsoncouto/arduino-songs
// by Robson Couto, 2019 (found in RadioLib AFSK_Imperial_March example).
//
// Score format: pairs of (frequency_Hz, duration_divisor).
//   Divisor > 0  →  whole_note / divisor  (e.g. 4 = quarter note)
//   Divisor < 0  →  dotted note = (whole_note / abs(divisor)) * 1.5
//   Frequency 0  →  rest
//
// Required Arduino settings (Tools menu):
//   Board        : XIAO_ESP32S3
//   Upload Speed : 115200
//
// Pin mapping:
//   GPIO 45  Buzzer (passive)
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
#define BUZZER_PIN 45   // GPIO45

// ===== Imperial March melody ================================================
// Format: {freq_hz, divisor}  — pairs, so melody length must be even.
// Negative divisor = dotted note (duration * 1.5).
// 0 frequency = rest.
static const int kMelody[] = {
    // Darth Vader theme (Imperial March) — Star Wars
    // Source: https://musescore.com/user/202909/scores/1141521 (tenor sax part)
    // Adapted from https://github.com/robsoncouto/arduino-songs

    440,-4, 440,-4, 440,16, 440,16, 440,16, 440,16, 349,8,  0,8,
    440,-4, 440,-4, 440,16, 440,16, 440,16, 440,16, 349,8,  0,8,
    440,4,  440,4,  440,4,  349,-8, 523,16,

    440,4,  349,-8, 523,16, 440,2,
    659,4,  659,4,  659,4,  698,-8, 523,16,
    440,4,  349,-8, 523,16, 440,2,

    880,4,  440,-8, 440,16, 880,4,  831,-8, 784,16,
    622,16, 587,16, 622,8,  0,8,    440,8,  622,4,  587,-8, 554,16,

    523,16, 494,16, 523,16, 0,8,    349,8,  415,4,  349,-8, 440,-16,
    523,4,  440,-8, 523,16, 659,2,

    880,4,  440,-8, 440,16, 880,4,  831,-8, 784,16,
    622,16, 587,16, 622,8,  0,8,    440,8,  622,4,  587,-8, 554,16,

    523,16, 494,16, 523,16, 0,8,    349,8,  415,4,  349,-8, 440,-16,
    440,4,  349,-8, 523,16, 440,2,
};
static const int kMelodyLen = sizeof(kMelody) / sizeof(kMelody[0]);

// BPM 120 → whole note = 60000*4/120 = 2000 ms
static const int kWholeDurationMs = 2000;

// =============================================================================
// Score player
// =============================================================================

static void playImperialMarch()
{
    for (int i = 0; i < kMelodyLen; i += 2) {
        int freq    = kMelody[i];
        int divisor = kMelody[i + 1];

        int durationMs;
        if (divisor > 0) {
            durationMs = kWholeDurationMs / divisor;
        } else {
            // Dotted note: regular duration * 1.5
            durationMs = (kWholeDurationMs / (-divisor)) * 3 / 2;
        }

        // Play for 90 % of the duration, then silence for 10 % to articulate.
        int soundMs = durationMs * 9 / 10;
        int gapMs   = durationMs - soundMs;

        if (freq > 0) {
            tone(BUZZER_PIN, freq, soundMs);
        }
        delay(soundMs);
        noTone(BUZZER_PIN);
        delay(gapMs);
    }
}

// =============================================================================
// setup / loop
// =============================================================================

void setup()
{
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    LOG_PRINTLN(F("\n[E1002] BuzzerWithTone — Imperial March"));

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop()
{
    LOG_PRINTLN(F("[buzz] Playing Imperial March ..."));
    playImperialMarch();
    LOG_PRINTLN(F("[buzz] Done. Pausing 3 s."));
    delay(3000);
}
