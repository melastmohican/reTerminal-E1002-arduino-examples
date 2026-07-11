// Button.ino
//
// Read the three user buttons on the Seeed Studio reTerminal E1002.
//
// The buttons are connected with hardware pull-up resistors on the carrier
// board, so the GPIO sees HIGH when the button is open and LOW when pressed.
// Debouncing is handled with a simple time-based filter.
//
// Required Arduino settings (Tools menu):
//   Board        : XIAO_ESP32S3
//   Upload Speed : 115200
//
// Pin mapping:
//   GPIO  3  KEY0 (user button 0, active-low)
//   GPIO  4  KEY1 (user button 1, active-low)
//   GPIO  5  KEY2 (user button 2, active-low)
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

// ===== Button pins ==========================================================
#define BTN_KEY0  3   // GPIO3
#define BTN_KEY1  4   // GPIO4
#define BTN_KEY2  5   // GPIO5

// Minimum time a state must be stable before it is accepted (ms).
#define DEBOUNCE_MS  50

// Per-button state tracker.
struct ButtonState {
    int      pin;
    bool     lastStable;   // last debounced state
    bool     lastRaw;      // last raw reading
    uint32_t lastChangeMs; // millis() when raw state last changed
};

static ButtonState buttons[] = {
    { BTN_KEY0, HIGH, HIGH, 0 },
    { BTN_KEY1, HIGH, HIGH, 0 },
    { BTN_KEY2, HIGH, HIGH, 0 },
};
static const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

// Call this every loop iteration.  Returns true if the stable state changed.
static bool updateButton(ButtonState& b)
{
    bool raw = digitalRead(b.pin);
    uint32_t now = millis();

    if (raw != b.lastRaw) {
        b.lastRaw      = raw;
        b.lastChangeMs = now;
    }

    if ((now - b.lastChangeMs) >= DEBOUNCE_MS && raw != b.lastStable) {
        b.lastStable = raw;
        return true;  // stable state changed
    }
    return false;
}

void setup()
{
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    LOG_PRINTLN(F("\n[E1002] Button demo  (KEY0=GPIO3  KEY1=GPIO4  KEY2=GPIO5)"));
    LOG_PRINTLN(F("[E1002] Press any button ..."));

    // Hardware pull-ups are on the carrier board; use plain INPUT.
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttons[i].pin, INPUT);
        buttons[i].lastRaw    = digitalRead(buttons[i].pin);
        buttons[i].lastStable = buttons[i].lastRaw;
    }
}

void loop()
{
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (updateButton(buttons[i])) {
            const char* action = (buttons[i].lastStable == LOW) ? "PRESSED" : "released";
            LOG_PRINTF("[btn] KEY%d (GPIO%d) %s\n", i, buttons[i].pin, action);
        }
    }
    delay(10);
}
