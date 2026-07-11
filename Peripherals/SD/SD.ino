// SD.ino
//
// Read and write files on the microSD card of the Seeed Studio reTerminal E1002.
//
// The E1002 carrier board includes a microSD slot with a power-enable pin and
// a card-detect pin.  The SD bus is on the same HSPI bus used by the ePaper
// display, but only the SD CS is used here (no display).
//
// Required Arduino settings (Tools menu):
//   Board        : XIAO_ESP32S3
//   Upload Speed : 115200
//
// Pin mapping (HSPI):
//   GPIO  7  SCK
//   GPIO  8  MISO
//   GPIO  9  MOSI
//   GPIO 14  SD CS
//   GPIO 15  SD DET  (card-detect, active-low)
//   GPIO 16  SD EN   (power enable, active-high)
//   GPIO 43  UART1 TX  (carrier board USB-UART bridge)
//   GPIO 44  UART1 RX

#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>

// ===== Serial / logging =====================================================
#define LOG_BAUD    115200
#define LOG_TX_PIN  43
#define LOG_RX_PIN  44
#define LOG_PRINT(x)    do { Serial1.print(x);    Serial.print(x);    } while(0)
#define LOG_PRINTLN(x)  do { Serial1.println(x);  Serial.println(x);  } while(0)
#define LOG_PRINTF(...) do { Serial1.printf(__VA_ARGS__); Serial.printf(__VA_ARGS__); } while(0)

// ===== SD card pins =========================================================
#define SD_SCK_PIN   7
#define SD_MISO_PIN  8
#define SD_MOSI_PIN  9
#define SD_CS_PIN   14
#define SD_EN_PIN   16   // power enable, active-high
#define SD_DET_PIN  15   // card-detect, active-low

SPIClass hspi(HSPI);

// ===== Helpers ==============================================================

bool mountSD()
{
    pinMode(SD_EN_PIN, OUTPUT);
    digitalWrite(SD_EN_PIN, HIGH);
    delay(50);

    pinMode(SD_DET_PIN, INPUT_PULLUP);
    if (digitalRead(SD_DET_PIN) == HIGH) {
        LOG_PRINTLN(F("[SD] No card detected (DET pin HIGH)."));
        return false;
    }
    LOG_PRINTLN(F("[SD] Card detected."));

    hspi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, /*SS=*/-1);
    if (!SD.begin(SD_CS_PIN, hspi)) {
        LOG_PRINTLN(F("[SD] SD.begin() failed."));
        return false;
    }

    LOG_PRINTF("[SD] Mounted.  Size: %llu MB  Type: %u\n",
               SD.cardSize() / (1024ULL * 1024ULL), SD.cardType());
    return true;
}

void listDir(const char* path, uint8_t levels)
{
    File root = SD.open(path);
    if (!root || !root.isDirectory()) {
        LOG_PRINTF("[SD] Cannot open directory '%s'\n", path);
        return;
    }

    LOG_PRINTF("[SD] Dir: %s\n", path);
    File f = root.openNextFile();
    while (f) {
        if (f.isDirectory()) {
            LOG_PRINTF("[SD]   DIR  %s\n", f.name());
            if (levels > 0) listDir(f.name(), levels - 1);
        } else {
            LOG_PRINTF("[SD]   FILE %s  %u bytes\n", f.name(), (unsigned)f.size());
        }
        f = root.openNextFile();
    }
}

void writeFile(const char* path, const char* message)
{
    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        LOG_PRINTF("[SD] Cannot open '%s' for writing\n", path);
        return;
    }
    f.print(message);
    f.close();
    LOG_PRINTF("[SD] Wrote to '%s'\n", path);
}

void readFile(const char* path)
{
    File f = SD.open(path, FILE_READ);
    if (!f) {
        LOG_PRINTF("[SD] Cannot open '%s' for reading\n", path);
        return;
    }

    LOG_PRINTF("[SD] Reading '%s' (%u bytes):\n", path, (unsigned)f.size());
    while (f.available()) {
        Serial1.write(f.read());
        Serial.write(f.read());
    }
    LOG_PRINTLN(F(""));
    f.close();
}

void appendFile(const char* path, const char* message)
{
    File f = SD.open(path, FILE_APPEND);
    if (!f) {
        LOG_PRINTF("[SD] Cannot open '%s' for append\n", path);
        return;
    }
    f.print(message);
    f.close();
    LOG_PRINTF("[SD] Appended to '%s'\n", path);
}

// ===== setup / loop =========================================================

void setup()
{
    Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
    Serial.begin(LOG_BAUD);
    for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

    LOG_PRINTLN(F("\n[E1002] SD card demo"));

    if (!mountSD()) {
        LOG_PRINTLN(F("[E1002] Halting — no SD card."));
        while (true) delay(1000);
    }

    listDir("/", 1);

    // Write, read back, then append.
    writeFile("/hello.txt", "Hello from reTerminal E1002!\n");
    readFile("/hello.txt");
    appendFile("/hello.txt", "Another line.\n");
    readFile("/hello.txt");

    LOG_PRINTLN(F("[E1002] SD demo complete."));
}

void loop() {}
