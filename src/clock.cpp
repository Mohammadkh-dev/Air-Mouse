#include "clock.h"
#include "app_state.h"

void updateClock() {
  unsigned long currentMs = millis();
  if (currentMs - lastUpdateMs >= 1000) {
    unsigned long secondsElapsed = (currentMs - lastUpdateMs) / 1000;
    clockTime += secondsElapsed;
    clockTime = clockTime % 86400;
    lastUpdateMs += secondsElapsed * 1000;
  }
}
