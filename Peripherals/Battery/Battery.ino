// Battery.ino
//
// Read and report battery voltage on the Seeed Studio reTerminal E1002.
//
// The carrier board includes a Li-Po battery connector with a voltage divider
// that halves the battery voltage before it reaches the ADC pin.  The enable
// pin must be driven HIGH before reading; drive it LOW afterward to avoid
// draining current through the divider.
//
// Required Arduino settings (Tools menu):
//   Board        : XIAO_ESP32S3
//   Upload Speed : 115200
//
// Pin mapping:
//   GPIO  1  Battery ADC input (after 1:2 voltage divider)
//   GPIO 21  Battery monitor enable (active-high)
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

// ===== Battery pins =========================================================
#define BATT_ADC_PIN  1   // GPIO1  — ADC input (voltage divider output)
#define BATT_EN_PIN  21   // GPIO21 — enable monitoring circuit (active-high)

void setup()
{
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    LOG_PRINTLN(F("\n[E1002] Battery voltage monitor"));

    // 12-bit ADC, full 3.3 V range (11 dB attenuation ≈ 150-3100 mV input).
    analogReadResolution(12);
    analogSetPinAttenuation(BATT_ADC_PIN, ADC_11db);

    // Enable circuit and let it stabilise.
    pinMode(BATT_EN_PIN, OUTPUT);
    digitalWrite(BATT_EN_PIN, HIGH);
    delay(100);
}

void loop()
{
    // Enable → sample → disable to minimise quiescent current.
    digitalWrite(BATT_EN_PIN, HIGH);
    delay(5);  // settle time

    int mv = analogReadMilliVolts(BATT_ADC_PIN);

    digitalWrite(BATT_EN_PIN, LOW);

    // The divider halves the battery voltage, so multiply by 2.
    float battV = (mv / 1000.0f) * 2.0f;

    LOG_PRINTF("[batt] ADC: %d mV  →  Battery: %.2f V\n", mv, battV);

    delay(2000);
}
