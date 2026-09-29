#include <Arduino.h>
#include <SPI.h>
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"
#include "image.h"

// Screen & Chessboard Dimensions
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480
#define GRID_SIZE     8                             // 8x8 chessboard
#define BLOCK_WIDTH   (SCREEN_WIDTH / GRID_SIZE)    // 100 pixels
#define BLOCK_HEIGHT  (SCREEN_HEIGHT / GRID_SIZE)   // 60 pixels

static uint8_t chessboard[GRID_SIZE][GRID_SIZE];

static void initChessboard(void)
{
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      chessboard[i][j] = ((i + j) % 2 == 0) ? COLOR_WHITE : COLOR_BLACK;
    }
  }
  LOG_PRINTLN("[GDEP073E01_Partial] Chessboard array initialized");
}

static void displayChessboard(void)
{
  uint8_t* rowBuffer = (uint8_t*)malloc(SCREEN_WIDTH / 2);
  if (!rowBuffer) {
    LOG_PRINTLN("[GDEP073E01_Partial] Failed to allocate rowBuffer!");
    return;
  }

  EPD_W21_WriteCMD(DTM); // 0x10

  EPD_W21_CS_0;
  EPD_W21_DC_1;
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    int gridY = y / BLOCK_HEIGHT;
    for (int x = 0; x < SCREEN_WIDTH; x += 2) {
      int gridX = x / BLOCK_WIDTH;
      uint8_t c1 = chessboard[gridY][gridX];
      uint8_t c2 = (x + 1 < SCREEN_WIDTH) ? chessboard[gridY][(x + 1) / BLOCK_WIDTH] : c1;
      rowBuffer[x / 2] = (c1 << 4) | (c2 & 0x0F);
    }
    for (int i = 0; i < SCREEN_WIDTH / 2; i++) {
      SPI_Write(rowBuffer[i]);
    }
    if (y % 60 == 0) yield();
  }
  EPD_W21_CS_1;

  free(rowBuffer);

  EPD_refresh();

  LOG_PRINTLN("[GDEP073E01_Partial] Chessboard rendered to screen");
}

static void manualPartialUpdate(int gridX, int gridY, uint8_t color, const char* name)
{
  if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
    chessboard[gridY][gridX] = color;
  }
  uint16_t x = gridX * BLOCK_WIDTH;
  uint16_t y = gridY * BLOCK_HEIGHT;

  LOG_PRINTF("[GDEP073E01_Partial] Partial update block (%d, %d) [%d,%d -> %dx%d] -> %s (0x%02X)...\n",
             gridX, gridY, x, y, BLOCK_WIDTH, BLOCK_HEIGHT, name, color);

  EPD_PartialWindow(x, y, BLOCK_WIDTH, BLOCK_HEIGHT, color);

  LOG_PRINTLN("[GDEP073E01_Partial] Block update complete.");
}

void setup()
{
  EPD_SPI_Init();

  LOG_PRINTLN("\n==================================================");
  LOG_PRINTLN(" Good Display GDEP073E01 Partial Refresh Demo");
  LOG_PRINTLN(" reTerminal E1002 (XIAO ESP32-S3)");
  LOG_PRINTLN(" Features: ED2208 CMD 0x83 Specified Area Refresh");
  LOG_PRINTLN("==================================================");

  LOG_PRINTLN("[GDEP073E01_Partial] 1. Initializing EPD for Full Screen Image...");
  EPD_init();
  LOG_PRINTLN("[GDEP073E01_Partial] Displaying gImage_1 (Upright Landscape)...");
  PIC_display(gImage_1);

  LOG_PRINTLN("[GDEP073E01_Partial] Image displayed. Waiting 5s before Chessboard partial demo...");
  delay(5000);

  initChessboard();
}

void loop()
{
  static int state = 0;

  if (state == 0) {
    LOG_PRINTLN("\n[GDEP073E01_Partial] --- State 0: Displaying initial 8x8 Chessboard ---");
    displayChessboard();
    state = 1;
    delay(5000);
  }
  else if (state == 1) {
    LOG_PRINTLN("\n[GDEP073E01_Partial] --- State 1: Partial update (2, 2) -> RED ---");
    manualPartialUpdate(2, 2, COLOR_RED, "RED");
    state = 2;
    delay(5000);
  }
  else if (state == 2) {
    LOG_PRINTLN("\n[GDEP073E01_Partial] --- State 2: Partial update (4, 4) -> YELLOW ---");
    manualPartialUpdate(4, 4, COLOR_YELLOW, "YELLOW");
    state = 3;
    delay(5000);
  }
  else if (state == 3) {
    LOG_PRINTLN("\n[GDEP073E01_Partial] --- State 3: Partial update (1, 5) -> BLUE ---");
    manualPartialUpdate(1, 5, COLOR_BLUE, "BLUE");
    state = 4;
    delay(5000);
  }
  else if (state == 4) {
    LOG_PRINTLN("\n[GDEP073E01_Partial] --- State 4: Partial update (6, 3) -> GREEN ---");
    manualPartialUpdate(6, 3, COLOR_GREEN, "GREEN");
    state = 5;
    delay(5000);
  }
  else if (state == 5) {
    LOG_PRINTLN("\n[GDEP073E01_Partial] --- State 5: Clear screen to white & deep sleep ---");
    PIC_display_Clear();
    EPD_sleep();
    LOG_PRINTLN("[GDEP073E01_Partial] Screen cleared. Demo completed successfully.");
    state = 6;
  }
  else {
    delay(1000);
  }
}
