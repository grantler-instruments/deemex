#include "config.h"
#include "./Display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)

class SSD1306Display final : public Display {
public:
  SSD1306Display()
    : oled_(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

  bool begin() override {
    if (!oled_.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      return false;
    }
    oled_.clearDisplay();
    oled_.setTextSize(1);
    oled_.setTextColor(SSD1306_WHITE);
    oled_.display();
    return true;
  }

  void update(const char* mode, int midiChannel, const DmxMessageHistory* history,
              int historySize,
              int historyHead) override {
    oled_.clearDisplay();
    drawHeader(mode, midiChannel);
    drawHistory(history, historySize, historyHead);
    oled_.display();
  }

private:
  Adafruit_SSD1306 oled_;

  /* ---------- Header ---------- */

  void drawHeader(const char* mode, int midiChannel) {
    // Static buffers avoid repeated stack or heap pressure
    static char buf[64];

    unsigned long displayUptime = (millis() / 1000) % 86400;  // Reset every 24h

    oled_.setCursor(0, 0);
    snprintf(buf, sizeof(buf),
             "deemex v%d.%d.%d t:%lu",
             VERSION_MAJOR,
             VERSION_MINOR,
             VERSION_PATCH,
             displayUptime);

    oled_.println(buf);

    oled_.setCursor(0, 10);
    if (strcmp(mode, "midi") == 0) {
      snprintf(buf, sizeof(buf),
               "mode:%s ch:%d",
               mode,
               midiChannel);
    } else {
      snprintf(buf, sizeof(buf),
               "mode:%s",
               mode);
    }
    oled_.println(buf);

    oled_.drawLine(0, 18, SCREEN_WIDTH, 18, SSD1306_WHITE);
  }

  void drawHistory(const DmxMessageHistory* history,
                   int size,
                   int head) {
    int y = 20;

    for (int i = 0; i < size; ++i) {
      int idx = (head + i) % size;
      const DmxMessageHistory& h = history[idx];

      if (h.timestamp == 0) {
        continue;
      }

      drawHistoryLine(h, y);
      y += 8;

      if (y > SCREEN_HEIGHT - 8) {
        break;
      }
    }
  }

  void drawHistoryLine(const DmxMessageHistory& h, int y) {
    char line[24];

    snprintf(line, sizeof(line),
             "ch:%3u val:%3u",
             h.channel,
             h.value);

    oled_.setCursor(0, y);
    oled_.print(line);
  }
};