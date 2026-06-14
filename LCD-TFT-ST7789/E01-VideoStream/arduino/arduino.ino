// Code provided by Wicky
#include <WiFi.h>
#include <FS.h>
#include <HTTPClient.h>

#include <TFT_eSPI.h>
#include <JPEGDEC.h>
#include "secrets.h"

#define JPG_MAX_SIZE 60000

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

const char *streamURL = LOCAL_STREAM_URL;

TFT_eSPI tft = TFT_eSPI();
JPEGDEC jpeg;

uint8_t *jpgBuffer = nullptr;

int JPEGDraw(JPEGDRAW *pDraw)
{
  tft.pushImage(
      pDraw->x,
      pDraw->y,
      pDraw->iWidth,
      pDraw->iHeight,
      pDraw->pPixels);

  return 1;
}

void connectWiFi()
{
  WiFi.begin(ssid, password);

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected");
  Serial.println(WiFi.localIP());
}

void streamMJPEG()
{
  HTTPClient http;
  http.begin(streamURL);

  int code = http.GET();

  if (code != HTTP_CODE_OK)
  {
    Serial.printf("HTTP error %d\n", code);
    http.end();
    delay(1000);
    return;
  }

  Serial.println("MJPEG connected");

  WiFiClient *stream = http.getStreamPtr();

  while (http.connected())
  {
    String line;
    int contentLength = 0;

    while (true)
    {
      if (!stream->available())
        continue;

      line = stream->readStringUntil('\n');
      line.trim();

      if (line.length() == 0)
        break;

      if (line.startsWith("Content-Length:"))
      {
        contentLength = line.substring(15).toInt();
      }
    }

    if (contentLength <= 0)
      continue;

    if (contentLength > JPG_MAX_SIZE)
    {
      Serial.printf("Frame too large: %d\n", contentLength);
      break;
    }

    int received = 0;
    uint8_t *ptr = jpgBuffer;

    uint32_t start = millis();

    while (received < contentLength)
    {
      int available = stream->available();
      if (available > 0)
      {
        int toRead = min(available, contentLength - received);
        int r = stream->read(ptr, toRead);

        ptr += r;
        received += r;
      }
    }

    if (received != contentLength)
    {
      Serial.printf("Short read %d/%d\n", received, contentLength);
      continue;
    }

    jpeg.openRAM(jpgBuffer, received, JPEGDraw);
    jpeg.decode(0, 0, 0);
    jpeg.close();

    uint32_t dt = millis() - start;

    Serial.printf("JPEG=%d bytes  frame=%lu ms (~%lu FPS)\n",
                  received, dt, 1000 / max(dt, (uint32_t)1));

    // consume trailing CRLF if exists
    if (stream->available())
      stream->read();
  }

  Serial.println("MJPEG disconnected");
  http.end();
}

void setup()
{
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);

  jpgBuffer = (uint8_t *)malloc(JPG_MAX_SIZE);

  if (!jpgBuffer)
  {
    Serial.println("JPEG buffer allocation failed");
    while (1)
      ;
  }

  connectWiFi();
}

void loop()
{
  streamMJPEG();
  delay(10);
}