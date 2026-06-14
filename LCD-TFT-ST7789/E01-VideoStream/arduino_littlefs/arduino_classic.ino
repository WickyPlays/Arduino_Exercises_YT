#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

#define FORMAT_LITTLEFS_IF_FAILED true

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

TFT_eSPI tft = TFT_eSPI();

bool tft_output(int16_t x, int16_t y,
                uint16_t w, uint16_t h,
                uint16_t *bitmap) {
  if (y >= tft.height()) return 0;

  tft.pushImage(x, y, w, h, bitmap);

  return 1;
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  listDir(LittleFS, "/", 0);

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);

  TJpgDec.setCallback(tft_output);
  TJpgDec.setJpgScale(1);
  TJpgDec.drawFsJpg(
    0,
    0,
    "/chungus.jpg",
    LittleFS);

  Serial.println("Test complete");
}

void loop() {
}