// Demo_BMP.ino
//
// Display a BMP image from SD card on the Seeed Studio reTerminal E1002
//   - 7.3" 6-Color ePaper, 800 x 480 pixels
//   - Panel: GDEP073E01 (ED2208 controller, 6-color ACeP — no orange!)
//   - Host MCU: XIAO ESP32-S3 (with 8 MB PSRAM)
//
// Supports standard 24-bit and 32-bit uncompressed BMP files.
// The image is color-matched to the 6 available e-ink colors using
// nearest-neighbor Euclidean distance in RGB space.
//
// Required Arduino settings (Tools menu):
//   Board   : XIAO_ESP32S3
//   PSRAM   : OPI PSRAM   <- MUST be enabled; the quantized image buffer lives here
//
// Performance strategy:
//   BMP rows are read from SD once (with 480 seeks for the bottom-to-top flip),
//   quantized to a nibble-packed PSRAM buffer (192 kB), then rendered in a single
//   fast drawNative() call.  This avoids the multi-minute drawPixel() + page-loop
//   penalty of the naive approach.
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
// Image format:
//   Standard Windows BMP, 24-bit (RGB) or 32-bit (RGBA), uncompressed.
//   Recommended resolution: 800 x 480 px (exact panel size).
//   To prepare:  python3 convert_image.py yourphoto.jpg /path/to/sd/image.bmp
//
// Workflow:
//   1. Run:  python3 convert_image.py yourphoto.jpg /path/to/sd/images/photo.bmp
//   2. Copy the .bmp to the SD card (any path).
//   3. Set BMP_PATH below to match the path on the SD card.
//   4. Upload once.  Swap images without reflashing — just copy a new .bmp.

#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <GxEPD2_7C.h>

// ===== Serial / logging ======================================================
// The reTerminal E1002 carrier board's USB-to-UART bridge is wired to
// GPIO43 (TX) / GPIO44 (RX) — the same pins where the ROM bootloader prints.
// We use Serial1 (UART1) so output appears even when USB CDC is not active.
// Serial (USB CDC) is also started as a fallback.
#define LOG_BAUD    115200
#define LOG_TX_PIN  43
#define LOG_RX_PIN  44
#define LOG_PRINT(x)    do { Serial1.print(x);    Serial.print(x);    } while(0)
#define LOG_PRINTLN(x)  do { Serial1.println(x);  Serial.println(x);  } while(0)
#define LOG_PRINTF(...) do { Serial1.printf(__VA_ARGS__); Serial.printf(__VA_ARGS__); } while(0)

// ===== User configuration ====================================================

// Path to the BMP file on the SD card.  Must start with '/'.
// Produced by convert_image.py (this sketch folder).
static const char* BMP_PATH = "/images/image.bmp";

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
// Nibble-packed: 2 pixels per byte, 800*480/2 = 192 000 bytes
#define IMAGE_BYTES   ((IMAGE_WIDTH * IMAGE_HEIGHT) / 2)

// PSRAM buffer — allocated once in setup(), freed on error.
static uint8_t* s_imgBuf = nullptr;

// =============================================================================
// 6-color palette for nearest-color matching
// =============================================================================

// Reference RGB values for the 6 e-ink colors on GDEP073E01 (no orange).
// The array index is also the nibble value used by drawNative():
//   0=Black  1=White  2=Green  3=Blue  4=Red  5=Yellow
static const uint8_t kPalette[6][3] = {
    {  0,   0,   0},  // 0: Black
    {255, 255, 255},  // 1: White
    {  0, 255,   0},  // 2: Green
    {  0,   0, 255},  // 3: Blue
    {255,   0,   0},  // 4: Red
    {255, 255,   0},  // 5: Yellow
};

// Returns the nibble value (0–5) of the closest e-ink color for an RGB pixel.
// Uses squared Euclidean distance — no sqrt needed.
static uint8_t nearestNibble(uint8_t r, uint8_t g, uint8_t b)
{
    long bestDist = LONG_MAX;
    int  bestIdx  = 0;
    for (int i = 0; i < 6; i++) {
        long dr = (long)r - kPalette[i][0];
        long dg = (long)g - kPalette[i][1];
        long db = (long)b - kPalette[i][2];
        long d  = dr*dr + dg*dg + db*db;
        if (d < bestDist) { bestDist = d; bestIdx = i; }
    }
    return (uint8_t)bestIdx;
}

// =============================================================================
// BMP file helpers
// =============================================================================

static uint16_t bmpRead16(File& f)
{
    uint16_t v;
    ((uint8_t*)&v)[0] = (uint8_t)f.read();
    ((uint8_t*)&v)[1] = (uint8_t)f.read();
    return v;
}

static uint32_t bmpRead32(File& f)
{
    uint32_t v;
    ((uint8_t*)&v)[0] = (uint8_t)f.read();
    ((uint8_t*)&v)[1] = (uint8_t)f.read();
    ((uint8_t*)&v)[2] = (uint8_t)f.read();
    ((uint8_t*)&v)[3] = (uint8_t)f.read();
    return v;
}

// =============================================================================
// SD helpers
// =============================================================================

bool mountSD()
{
    // Power-enable the SD slot on the E1002 carrier board.
    pinMode(SD_EN_PIN, OUTPUT);
    digitalWrite(SD_EN_PIN, HIGH);
    delay(50);

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
// BMP loading — decode into PSRAM nibble buffer
// =============================================================================

// Reads a BMP from the SD card and decodes it into the global PSRAM nibble
// buffer (s_imgBuf).  Supports 24-bit and 32-bit uncompressed Windows BMP.
//
// The BMP's bottom-to-top row order is corrected during decode: each row is
// read with a single seek so the buffer is stored top-to-bottom, ready for
// drawNative().
//
// Row format in s_imgBuf: 2 pixels per byte, MSB nibble = left pixel.
//   0=Black  1=White  2=Green  3=Blue  4=Red  5=Yellow
bool loadBmpFromSD(const char* path)
{
    LOG_PRINTF("[bmp] Opening '%s' ...\n", path);
    File f = SD.open(path, FILE_READ);
    if (!f) {
        LOG_PRINTF("[bmp] Cannot open '%s'\n", path);
        return false;
    }

    // ----- BMP file header ---------------------------------------------------
    if (bmpRead16(f) != 0x4D42) {       // 'BM'
        LOG_PRINTLN(F("[bmp] Not a valid BMP file (bad magic bytes)."));
        f.close(); return false;
    }

    bmpRead32(f);                        // file size (ignored)
    bmpRead32(f);                        // reserved
    uint32_t dataOffset = bmpRead32(f);  // offset to pixel data

    // ----- DIB (info) header -------------------------------------------------
    bmpRead32(f);                        // DIB header size
    int32_t  imgW     = (int32_t)bmpRead32(f);
    int32_t  imgH     = (int32_t)bmpRead32(f);
    bmpRead16(f);                        // color planes (must be 1)
    uint16_t bpp      = bmpRead16(f);
    uint32_t compress = bmpRead32(f);

    if (compress != 0) {
        LOG_PRINTF("[bmp] Compression type %u not supported — "
                   "re-save as uncompressed 24-bit BMP.\n", compress);
        f.close(); return false;
    }
    if (bpp != 24 && bpp != 32) {
        LOG_PRINTF("[bmp] Bit depth %u not supported (need 24 or 32).\n", bpp);
        f.close(); return false;
    }

    // Positive height -> rows stored bottom-to-top (standard BMP convention).
    // Negative height -> top-to-bottom (less common but valid).
    bool flip = (imgH > 0);
    if (imgH < 0) imgH = -imgH;

    LOG_PRINTF("[bmp] %d x %d, %u-bit, offset=%u, flip=%d\n",
               imgW, imgH, bpp, dataOffset, flip);

    if (imgW != IMAGE_WIDTH || imgH != IMAGE_HEIGHT) {
        LOG_PRINTF("[bmp] WARNING: image size %dx%d != panel %dx%d — "
                   "run convert_image.py to resize first.\n",
                   imgW, imgH, IMAGE_WIDTH, IMAGE_HEIGHT);
    }

    // ----- Allocate PSRAM nibble buffer --------------------------------------
    if (!s_imgBuf) {
        s_imgBuf = (uint8_t*)ps_malloc(IMAGE_BYTES);
        if (!s_imgBuf) {
            LOG_PRINTLN(F("[bmp] ps_malloc() failed — "
                          "is PSRAM enabled? (Tools > PSRAM > OPI PSRAM)"));
            f.close(); return false;
        }
        LOG_PRINTF("[bmp] PSRAM buffer: %u bytes at %p\n", IMAGE_BYTES, s_imgBuf);
    }

    // ----- Decode BMP rows into nibble buffer --------------------------------
    uint8_t  bytesPerPx = bpp / 8;
    // BMP rows are zero-padded to a multiple of 4 bytes.
    uint32_t rowBytes   = ((uint32_t)imgW * bytesPerPx + 3) & ~3u;

    // Stack row buffer: 800 px x 4 bytes = 3200 bytes max.
    uint8_t rowBuf[rowBytes];

    LOG_PRINTLN(F("[bmp] Decoding into PSRAM ..."));
    uint32_t nibbleIdx = 0;

    for (int32_t row = 0; row < imgH; row++) {
        // Seek to the correct BMP row (flip bottom-to-top -> top-to-bottom).
        uint32_t rowPos = flip
            ? dataOffset + (uint32_t)(imgH - 1 - row) * rowBytes
            : dataOffset + (uint32_t)row * rowBytes;
        f.seek(rowPos);
        f.read(rowBuf, rowBytes);

        // Quantize and pack two pixels per byte.
        for (int32_t col = 0; col < imgW; col += 2) {
            // BMP stores BGR (or BGRA).
            uint8_t b0 = rowBuf[ col      * bytesPerPx + 0];
            uint8_t g0 = rowBuf[ col      * bytesPerPx + 1];
            uint8_t r0 = rowBuf[ col      * bytesPerPx + 2];
            uint8_t b1 = rowBuf[(col + 1) * bytesPerPx + 0];
            uint8_t g1 = rowBuf[(col + 1) * bytesPerPx + 1];
            uint8_t r1 = rowBuf[(col + 1) * bytesPerPx + 2];

            uint8_t n0 = nearestNibble(r0, g0, b0);  // left pixel  -> MSB nibble
            uint8_t n1 = nearestNibble(r1, g1, b1);  // right pixel -> LSB nibble
            s_imgBuf[nibbleIdx++] = (n0 << 4) | n1;
        }
    }

    f.close();
    LOG_PRINTF("[bmp] Decode complete.  Nibble buffer: %u bytes used.\n", nibbleIdx);
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

    LOG_PRINTLN(F("[epd] Rendering via drawNative() (~25-30 s) ..."));

    // drawNative with pgm=false reads from RAM (our PSRAM buffer).
    // Pixel layout: 2 pixels per byte, MSB nibble = left pixel.
    //   0=Black  1=White  2=Green  3=Blue  4=Red  5=Yellow
    display.setFullWindow();
    display.drawNative(
        s_imgBuf,      // PSRAM buffer  (pgm=false -> treated as RAM)
        0,             // x
        0,             // y
        0,             // sub-image x offset
        IMAGE_WIDTH,
        IMAGE_HEIGHT,
        false,         // invert: keep native nibble values
        false,         // mirror_y
        false          // pgm: data is in PSRAM (RAM), not Flash
    );

    LOG_PRINTLN(F("[epd] drawNative: done."));
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
    LOG_PRINTLN(F("[E1002] GxEPD2 reTerminal E1002 — SD-card BMP Demo (PSRAM)"));
    LOG_PRINTLN(F("============================================"));
    LOG_PRINTF("[mem] Free heap: %lu kB  PSRAM: %lu / %lu kB\n",
               (unsigned long)(ESP.getFreeHeap()  / 1024),
               (unsigned long)(ESP.getFreePsram() / 1024),
               (unsigned long)(ESP.getPsramSize() / 1024));

    // ----- EPD pins & SPI -----
    pinMode(EPD_RES_PIN, OUTPUT);
    pinMode(EPD_DC_PIN,  OUTPUT);
    pinMode(EPD_CS_PIN,  OUTPUT);

    // HSPI: add MISO (GPIO8) for SD card; EPD doesn't use it but it's harmless.
    hspi.begin(EPD_SCK_PIN, SD_MISO_PIN, EPD_MOSI_PIN, /*SS=*/-1);
    display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    display.init(0);

    LOG_PRINTF("[epd] Panel: %d x %d\n", display.width(), display.height());

    // ----- SD card -----
    LOG_PRINTLN(F("[E1002] Mounting SD card ..."));
    if (!mountSD()) {
        LOG_PRINTLN(F("[E1002] SD mount failed — halting."));
        display.hibernate();
        while (true) delay(1000);
    }

    // ----- Decode BMP into PSRAM -----
    LOG_PRINTF("[E1002] Loading '%s' into PSRAM ...\n", BMP_PATH);
    if (!loadBmpFromSD(BMP_PATH)) {
        LOG_PRINTF("[E1002] Failed to load '%s' — halting.\n", BMP_PATH);
        display.hibernate();
        while (true) delay(1000);
    }

    LOG_PRINTF("[mem] Free heap: %lu kB  PSRAM: %lu / %lu kB\n",
               (unsigned long)(ESP.getFreeHeap()  / 1024),
               (unsigned long)(ESP.getFreePsram() / 1024),
               (unsigned long)(ESP.getPsramSize() / 1024));

    // ----- Render -----
    drawImage();

    LOG_PRINTLN(F("[E1002] Done. Hibernating."));
    display.hibernate();
}

void loop() {}
