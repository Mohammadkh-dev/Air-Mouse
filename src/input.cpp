#include "input.h"
#include "clock.h"
#include "app_state.h"
#include "config.h"

int getButton(int timeout) {
  for (int i = timeout; i > 0; i--) {
    updateClock();

    if (digitalRead(BTN_LEFT) == HIGH && digitalRead(BTN_RIGHT) == HIGH) {
      blockLeftClick = false;
      blockBothClick = false;
    }

    if (digitalRead(BTN_LEFT) == LOW && digitalRead(BTN_RIGHT) == LOW) {
      if (!blockBothClick) {
        blockBothClick = true;
        blockLeftClick = true;
        return 4;
      }
      delay(10);
      continue;
    }

    if (digitalRead(BTN_LEFT) == LOW && digitalRead(BTN_RIGHT) == HIGH) {
      if (blockLeftClick || blockBothClick) {
        delay(10);
        continue;
      }
      unsigned long pressTime = millis();
      bool isLongPress = false;

      while (digitalRead(BTN_LEFT) == LOW) {
        updateClock();
        if (digitalRead(BTN_RIGHT) == LOW) {
          blockBothClick = true;
          blockLeftClick = true;
          return 4;
        }
        if (millis() - pressTime > 1500) {
          isLongPress = true;
          blockLeftClick = true;
          return 3;
        }
        delay(10);
      }
      if (!isLongPress && !blockLeftClick) return 1;
    }

    if (digitalRead(BTN_RIGHT) == LOW && digitalRead(BTN_LEFT) == HIGH) {
      if (blockBothClick) {
        delay(10);
        continue;
      }
      while (digitalRead(BTN_RIGHT) == LOW) {
        updateClock();
        if (digitalRead(BTN_LEFT) == LOW) {
          blockBothClick = true;
          blockLeftClick = true;
          return 4;
        }
        delay(10);
      }
      return 2;
    }
    delay(1);
  }
  return -1;
}
