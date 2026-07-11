// GxEPD2_reTerminal_E1002_SDcard.ino
//
// Display a 6-color image from SD card on the Seeed Studio reTerminal E1002
//   - 7.3" 6-Color ePaper, 800 x 480 pixels
//   - Panel: GDEP073E01 (ED2208 controller, 6-color ACeP — no orange!)
//   - Host MCU: XIAO ESP32-S3 (with 8 MB PSRAM)
//
// Required Arduino settings (Tools menu):
//   Board   : XIAO_ESP32S3
//   PSRAM   : OPI PSRAM   ← MUST be enabled; the image buffer lives here
//
// Pin mapping (HSPI bus shared between EPD and SD):
//   GPIO  7  SCK   — shared EPD + SD
//   GPIO  8  MISO  — SD only  (EPD has no MISO)
//   GPIO  9  MOSI  — shared EPD + SD
//   GPIO 10  EPD CS
//   GPIO 11  EPD DC
//   GPIO 12  EPD RST
//   GPIO 13  EPD BUSY
//   GPIO 14  SD CS
//   GPIO 15  SD DET  (card-detect, active-low; read-only)
//   GPIO 16  SD EN   (power enable, active-high)
//
// Image format (produced by convert_image.py in this sketch folder):
//   Raw binary, 2 pixels per byte packed as nibbles (MSB = left pixel):
//     0x0=Black  0x1=White  0x2=Green  0x3=Blue  0x4=Red  0x5=Yellow
//   800×480 pixels → exactly 192 000 bytes per file.
//
// Workflow:
//   1. Run:  python3 convert_image.py yourphoto.jpg /path/to/sd/images/photo.bin
//   2. Copy the .bin to the SD card under /images/ (or any path).
//   3. Set IMAGE_PATH below.
//   4. Upload once.  Swap images without reflashing — just edit IMAGE_PATH or
//      pick a file from the SD listing at runtime.

#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <GxEPD2_7C.h>

// ===== Serial / logging ======================================================
// The reTerminal E1002 carrier board's USB-to-UART bridge is wired to
// GPIO43 (TX) / GPIO44 (RX) — the same pins where the ROM bootloader prints.
// Arduino's default `Serial` targets the XIAO's native USB CDC port, which
// only works when "USB CDC On Boot" is enabled in the Tools menu.
// We use Serial1 (UART1) mapped to those pins so output appears regardless
// of the USB CDC setting.  Serial (USB CDC) is also started as a fallback.
#define LOG_BAUD    115200
#define LOG_TX_PIN  43
#define LOG_RX_PIN  44
// Convenience macro: write to both ports so you see output whether you are
// connected via the carrier's USB-UART or directly to the XIAO's USB port.
#define LOG_PRINT(x)   do { Serial1.print(x);   Serial.print(x);   } while(0)
#define LOG_PRINTLN(x) do { Serial1.println(x); Serial.println(x); } while(0)
#define LOG_PRINTF(...)do { Serial1.printf(__VA_ARGS__); Serial.printf(__VA_ARGS__); } while(0)

// ===== User configuration ====================================================

// Path to the pre-converted 6-color raw binary on the SD card.
// Must start with '/'.  Produced by convert_image.py (this sketch folder).
static const char* IMAGE_PATH = "/images/mocha.bin";

// =============================================================================

// ----- EPD pins --------------------------------------------------------------
#define EPD_SCK_PIN   7
#define EPD_MOSI_PIN  9
#define EPD_CS_PIN    10
#define EPD_DC_PIN    11
#define EPD_RES_PIN   12
#define EPD_BUSY_PIN  13

// ----- SD pins ---------------------------------------------------------------
#define SD_SCK_PIN    7    // shared with EPD
#define SD_MISO_PIN   8    // SD-only
#define SD_MOSI_PIN   9    // shared with EPD
#define SD_CS_PIN     14
#define SD_EN_PIN     16   // power-enable, active-high
#define SD_DET_PIN    15   // card-detect, active-low (optional)

// ----- Display ---------------------------------------------------------------
// 16 kB page buffer is enough for the XIAO ESP32-S3; the full-frame image
// data lives in PSRAM, not in the page buffer.
#define MAX_DISPLAY_BUFFER_SIZE 16000u
#define MAX_HEIGHT(EPD) \
    (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2) \
         ? EPD::HEIGHT \
         : (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2))

SPIClass hspi(HSPI);

GxEPD2_7C<GxEPD2_730c_GDEP073E01, MAX_HEIGHT(GxEPD2_730c_GDEP073E01)>
    display(GxEPD2_730c_GDEP073E01(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_BUSY_PIN));

// ----- Image geometry --------------------------------------------------------
#define IMAGE_WIDTH   800
#define IMAGE_HEIGHT  480
#define IMAGE_BYTES   ((IMAGE_WIDTH * IMAGE_HEIGHT) / 2)   // 192 000

// PSRAM buffer — allocated once in setup(), freed on error.
static uint8_t* s_imgBuf = nullptr;

// =============================================================================
// SD helpers
// =============================================================================

bool mountSD()
{
  // Power-enable the SD slot on the E1002 carrier board.
  pinMode(SD_EN_PIN, OUTPUT);
  digitalWrite(SD_EN_PIN, HIGH);
  delay(50);  // give the SD slot time to power up

  // Optional: bail early if no card is physically inserted.
  pinMode(SD_DET_PIN, INPUT_PULLUP);
  if (digitalRead(SD_DET_PIN) == HIGH) {
    LOG_PRINTLN(F("[SD] No card detected (DET pin HIGH)."));
    return false;
  }
  LOG_PRINTLN(F("[SD] Card detected."));

  if (!SD.begin(SD_CS_PIN, hspi)) {
    LOG_PRINTLN(F("[SD] SD.begin() failed — check card and wiring."));
    return false;
  }

  LOG_PRINTF("[SD] Card mounted.  Size: %llu MB\n",
             SD.cardSize() / (1024ULL * 1024ULL));
  return true;
}

// =============================================================================
// Image loading
// =============================================================================

bool loadImageFromSD(const char* path)
{
  File f = SD.open(path, FILE_READ);
  if (!f) {
    LOG_PRINTF("[img] Cannot open '%s'\n", path);
    return false;
  }

  size_t fileSize = f.size();
  LOG_PRINTF("[img] '%s'  %u bytes\n", path, (unsigned)fileSize);

  if (fileSize != IMAGE_BYTES) {
    LOG_PRINTF("[img] ERROR: expected %u bytes (%dx%d/2), got %u.\n",
               IMAGE_BYTES, IMAGE_WIDTH, IMAGE_HEIGHT, (unsigned)fileSize);
    f.close();
    return false;
  }

  // Allocate in PSRAM — never touches precious SRAM.
  if (!s_imgBuf) {
    s_imgBuf = (uint8_t*)ps_malloc(IMAGE_BYTES);
    if (!s_imgBuf) {
      LOG_PRINTLN(F("[img] ps_malloc() failed — is PSRAM enabled? (Tools > PSRAM > OPI PSRAM)"));
      f.close();
      return false;
    }
  }

  LOG_PRINTLN(F("[img] Reading into PSRAM ..."));
  size_t bytesRead = f.read(s_imgBuf, IMAGE_BYTES);
  f.close();

  if (bytesRead != IMAGE_BYTES) {
    LOG_PRINTF("[img] Short read: %u / %u bytes\n",
               (unsigned)bytesRead, IMAGE_BYTES);
    return false;
  }

  LOG_PRINTLN(F("[img] Load complete."));
  return true;
}

// =============================================================================
// Display rendering
// =============================================================================

void drawImage()
{
  if (!s_imgBuf) {
    LOG_PRINTLN(F("[epd] drawImage: no image buffer."));
    return;
  }

  LOG_PRINTLN(F("[epd] drawImage: starting full-panel render (~25-30 s) ..."));

  // drawNative with pgm=false reads from RAM (our PSRAM buffer).
  // Pixel layout: 2 pixels per byte, MSB nibble = left pixel.
  //   0x0=Black  0x1=White  0x2=Green  0x3=Blue  0x4=Red  0x5=Yellow
  display.setFullWindow();
  display.drawNative(
    s_imgBuf,      // pointer to PSRAM buffer (pgm=false → treated as RAM)
    0,             // x
    0,             // y
    0,             // sub-image x offset
    IMAGE_WIDTH,
    IMAGE_HEIGHT,
    false,         // invert: keep native nibble values
    false,         // mirror_y
    false          // pgm: data is in PSRAM (RAM), not Flash
  );

  LOG_PRINTLN(F("[epd] drawImage: done."));
}

// =============================================================================
// setup / loop
// =============================================================================

void setup()
{
  // Start UART1 on the carrier board's USB-UART pins first (no host needed).
  Serial1.begin(LOG_BAUD, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
  // Also start USB CDC — works when "USB CDC On Boot" is enabled.
  Serial.begin(LOG_BAUD);
  // Give USB CDC up to 2 s to enumerate; don't block if no host is connected.
  for (uint32_t t = millis(); !Serial && (millis() - t < 2000); ) delay(10);

  LOG_PRINTLN(F("\n\n============================================"));
  LOG_PRINTLN(F("[E1002] GxEPD2 reTerminal E1002 — SD-card Image Demo"));
  LOG_PRINTLN(F("============================================"));
  LOG_PRINTF("[mem] Free heap: %lu kB  PSRAM: %lu / %lu kB\n",
             (unsigned long)(ESP.getFreeHeap()   / 1024),
             (unsigned long)(ESP.getFreePsram()  / 1024),
             (unsigned long)(ESP.getPsramSize()  / 1024));

  // ----- EPD pins & SPI -----
  pinMode(EPD_RES_PIN, OUTPUT);
  pinMode(EPD_DC_PIN,  OUTPUT);
  pinMode(EPD_CS_PIN,  OUTPUT);

  // HSPI: add MISO (GPIO8) for SD card; EPD doesn't use it but it's harmless.
  hspi.begin(EPD_SCK_PIN, SD_MISO_PIN, EPD_MOSI_PIN, /*SS=*/-1);
  display.epd2.selectSPI(hspi, SPISettings(2000000, MSBFIRST, SPI_MODE0));
  display.init(0);

  LOG_PRINTF("[epd] Panel: %d x %d\n", display.width(), display.height());

  // ----- SD card -----
  LOG_PRINTLN(F("[E1002] Mounting SD card ..."));
  if (!mountSD()) {
    LOG_PRINTLN(F("[E1002] SD mount failed — halting."));
    display.hibernate();
    while (true) delay(1000);
  }

  // ----- Load image into PSRAM -----
  LOG_PRINTF("[E1002] Loading '%s' ...\n", IMAGE_PATH);
  if (!loadImageFromSD(IMAGE_PATH)) {
    LOG_PRINTF("[E1002] Failed to load '%s' — halting.\n", IMAGE_PATH);
    display.hibernate();
    while (true) delay(1000);
  }

  // ----- Render -----
  drawImage();

  LOG_PRINTLN(F("[E1002] Done. Hibernating."));
  display.hibernate();
}

void loop() {}
