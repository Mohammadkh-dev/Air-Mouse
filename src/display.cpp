#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "display.h"
#include "app_state.h"
#include "config.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool initDisplay() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    return false;
  }
  return true;
}

void clearDisplay() {
  display.clearDisplay();
  display.display();
}

void printOLED(int x, int y, String text, int size) {
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(x, y);
  display.print(text);
}

void drawStatusBar() {
  display.fillRect(0, 0, 128, 12, SSD1306_BLACK);

  if (mode == true) {
    display.drawRect(2, 4, 6, 4, SSD1306_WHITE);
    display.fillRect(8, 2, 7, 8, SSD1306_WHITE);
    display.drawPixel(4, 5, SSD1306_BLACK);
    display.drawPixel(4, 6, SSD1306_BLACK);
    printOLED(18, 2, "Dongle", 1);
  } else {
    display.drawLine(2, 2, 2, 8, SSD1306_WHITE);
    display.drawLine(2, 2, 4, 4, SSD1306_WHITE);
    display.drawLine(4, 4, 0, 6, SSD1306_WHITE);
    display.drawLine(0, 4, 4, 6, SSD1306_WHITE);
    display.drawLine(4, 6, 2, 8, SSD1306_WHITE);

    String displayBtName = btName;
    if (btName.length() > 10) {
      displayBtName = btName.substring(0, 8) + "..";
    }
    printOLED(8, 2, displayBtName, 1);
  }

  int micX = 96;
  display.drawRoundRect(micX - 2, 1, 5, 6, 2, SSD1306_WHITE);
  display.drawLine(micX, 7, micX, 9, SSD1306_WHITE);
  display.drawLine(micX - 2, 9, micX + 2, 9, SSD1306_WHITE);
  if (!mic) display.drawLine(micX - 4, 1, micX + 4, 9, SSD1306_WHITE);

  int batX = 110;
  display.drawRect(batX, 2, 14, 8, SSD1306_WHITE);
  display.fillRect(batX + 14, 4, 2, 4, SSD1306_WHITE);
  for (int i = 0; i < batteryLevel && i < 5; i++) {
    display.fillRect(batX + 2 + (i * 2), 4, 1, 4, SSD1306_WHITE);
  }
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);
}

void timePrint(unsigned long time, int x, int y, int size) {
  char buffer[20];
  unsigned long h = time / 3600;
  unsigned long m = (time % 3600) / 60;
  unsigned long s = time % 60;
  sprintf(buffer, "%02lu:%02lu:%02lu", h, m, s);
  printOLED(x, y, String(buffer), size);
}
