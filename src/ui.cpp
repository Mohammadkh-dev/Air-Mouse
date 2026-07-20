#include "ui.h"
#include "display.h"
#include "input.h"
#include "app_state.h"
#include "config.h"

static void resetToolStateForLaunch() {
  swState = 0;
  swRunning = false;
  swElapsed = 0;
  swPauseOffset = 0;
  tmState = 0;
  tmRunning = false;
  tmSetupSeconds = 0;
  tmSetupField = 0;
  tmSetH = 0;
  tmSetM = 0;
  tmSetS = 0;
  scField = 0;
  scSetH = clockTime / 3600;
  scSetM = (clockTime % 3600) / 60;
  scSetS = clockTime % 60;
}

static void handleSleepScreen() {
  if (initialSleepRender) {
    display.clearDisplay();
    printOLED(40, 40, "Sleep", 1);
    initialSleepRender = false;
  }
  timePrint(clockTime, 16, 20, 2);
  display.display();

  if (getButton(10) != -1) {
    noSleep = true;
    initialSleepRender = true;
    currentScreen = 0;
  }
}

static void handleHomeScreen() {
  timePrint(clockTime, 16, 25, 2);
  printOLED(0, 55, "Menu", 1);
  printOLED(110, 55, "Mic", 1);
  display.display();

  short btn = getButton(10);
  if (btn == 1) currentScreen = 1;
  else if (btn == 2) mic = !mic;
}

static void handleClockIconScreen() {
  display.drawCircle(64, 35, 12, SSD1306_WHITE);
  display.drawLine(64, 35, 64, 28, SSD1306_WHITE);
  display.drawCircle(64, 35, 1, SSD1306_WHITE);
  display.drawLine(64, 35, 70, 35, SSD1306_WHITE);

  printOLED(0, 55, "Click", 1);
  printOLED(95, 55, "Next", 1);
  display.display();

  short btn = getButton(10);
  if (btn == 1) {
    currentScreen = 2;
    menuIndex = 0;
  } else if (btn == 3 || btn == 4) {
    currentScreen = 0;
  }
}

static void handleMenuListScreen() {
  printOLED(0, 28, menuItems[menuIndex], 2);
  printOLED(0, 55, "Click", 1);
  printOLED(95, 55, "Next", 1);
  display.display();

  short btn = getButton(10);
  if (btn == 2) {
    menuIndex = (menuIndex + 1) % 4;
  } else if (btn == 1) {
    resetToolStateForLaunch();
    currentScreen = 3;
  } else if (btn == 3) {
    currentScreen = 1;
  } else if (btn == 4) {
    currentScreen = 0;
  }
}

static void handleStopwatchScreen() {
  if (swRunning) {
    swElapsed = swPauseOffset + (millis() - swStartTime);
  }
  printOLED(30, 15, "Stopwatch", 1);

  char buf[20];
  unsigned long m = (swElapsed / 60000) % 60;
  unsigned long s = (swElapsed / 1000) % 60;
  unsigned long cs = (swElapsed / 10) % 100;
  sprintf(buf, "%02lu:%02lu:%02lu", m, s, cs);
  printOLED(20, 32, String(buf), 2);

  if (swState == 0) {
    printOLED(0, 55, "Start", 1);
    printOLED(90, 55, "Back", 1);
  } else if (swState == 1) {
    printOLED(0, 55, "Pause", 1);
    printOLED(90, 55, "Lap", 1);
  } else if (swState == 2) {
    printOLED(0, 55, "Resume", 1);
    printOLED(90, 55, "Reset", 1);
  }
  display.display();

  short btn = getButton(10);
  if (btn == 1) {
    if (swState == 0) {
      swStartTime = millis();
      swRunning = true;
      swState = 1;
    } else if (swState == 1) {
      swPauseOffset = swElapsed;
      swRunning = false;
      swState = 2;
    } else if (swState == 2) {
      swStartTime = millis();
      swRunning = true;
      swState = 1;
    }
  } else if (btn == 2) {
    if (swState == 0) currentScreen = 2;
    else if (swState == 2) {
      swElapsed = 0;
      swPauseOffset = 0;
      swState = 0;
    }
  } else if (btn == 3) {
    swRunning = false;
    currentScreen = 2;
  } else if (btn == 4) {
    swRunning = false;
    currentScreen = 0;
  }
}

static void handleTimerSetupScreen() {
  char buf[20];
  sprintf(buf, "%02lu:%02lu:%02lu", tmSetH, tmSetM, tmSetS);
  printOLED(35, 15, "Set Timer", 1);
  printOLED(16, 32, String(buf), 2);
  printOLED(0, 55, "Next", 1);
  printOLED(95, 55, "Plus", 1);

  if (tmSetupField == 0) display.drawFastHLine(16, 48, 22, SSD1306_WHITE);
  else if (tmSetupField == 1) display.drawFastHLine(52, 48, 22, SSD1306_WHITE);
  else if (tmSetupField == 2) display.drawFastHLine(88, 48, 22, SSD1306_WHITE);
  display.display();

  short btn = getButton(10);
  if (btn == 2) {
    if (tmSetupField == 0) tmSetH = (tmSetH + 1) % 24;
    else if (tmSetupField == 1) tmSetM = (tmSetM + 1) % 60;
    else if (tmSetupField == 2) tmSetS = (tmSetS + 1) % 60;
  } else if (btn == 1) {
    tmSetupField++;
    if (tmSetupField > 2) {
      tmSetupSeconds = (tmSetH * 3600) + (tmSetM * 60) + tmSetS;
      if (tmSetupSeconds > 0) {
        tmStartTime = millis();
        tmRunning = true;
        tmState = 1;
      } else {
        currentScreen = 2;
      }
    }
  } else if (btn == 3) {
    currentScreen = 2;
  } else if (btn == 4) {
    currentScreen = 0;
  }
}

static void handleTimerRunScreen() {
  unsigned long currentElapsed = tmPauseOffset;
  if (tmRunning) {
    currentElapsed = tmPauseOffset + (millis() - tmStartTime);
  }

  long remaining = (long)tmSetupSeconds - (currentElapsed / 1000);
  if (remaining <= 0) {
    tmRunning = false;
    display.clearDisplay();
    printOLED(20, 25, "TIME'S UP!", 2);
    display.display();
    delay(3000);
    tmState = 0;
    return;
  }

  printOLED(45, 15, "Timer", 1);
  timePrint((unsigned long)remaining, 16, 32, 2);
  if (tmRunning) {
    printOLED(0, 55, "Pause", 1);
  } else {
    printOLED(0, 55, "Start", 1);
    printOLED(90, 55, "Reset", 1);
  }
  display.display();

  short btn = getButton(10);
  if (btn == 1) {
    if (tmRunning) {
      tmPauseOffset = currentElapsed;
      tmRunning = false;
    } else {
      tmStartTime = millis();
      tmRunning = true;
    }
  } else if (btn == 2) {
    if (!tmRunning) {
      tmState = 0;
      tmSetupField = 0;
      tmSetH = 0;
      tmSetM = 0;
      tmSetS = 0;
      tmPauseOffset = 0;
    }
  } else if (btn == 3) {
    tmRunning = false;
    currentScreen = 2;
  } else if (btn == 4) {
    tmRunning = false;
    currentScreen = 0;
  }
}

static void handleSetClockScreen() {
  char buf[20];
  sprintf(buf, "%02lu:%02lu:%02lu", scSetH, scSetM, scSetS);
  printOLED(35, 15, "Set Clock", 1);
  printOLED(16, 32, String(buf), 2);
  printOLED(0, 55, "Next", 1);
  printOLED(95, 55, "Plus", 1);

  if (scField == 0) display.drawFastHLine(16, 48, 22, SSD1306_WHITE);
  else if (scField == 1) display.drawFastHLine(52, 48, 22, SSD1306_WHITE);
  else if (scField == 2) display.drawFastHLine(88, 48, 22, SSD1306_WHITE);
  display.display();

  short btn = getButton(10);
  if (btn == 2) {
    if (scField == 0) scSetH = (scSetH + 1) % 24;
    else if (scField == 1) scSetM = (scSetM + 1) % 60;
    else if (scField == 2) scSetS = (scSetS + 1) % 60;
  } else if (btn == 1) {
    scField++;
    if (scField > 2) {
      clockTime = (scSetH * 3600) + (scSetM * 60) + scSetS;
      lastUpdateMs = millis();
      currentScreen = 2;
    }
  } else if (btn == 3) {
    currentScreen = 2;
  } else if (btn == 4) {
    currentScreen = 0;
  }
}

static void handleAboutScreen() {
  printOLED(38, 15, "About us", 1);
  printOLED(20, 32, "MMV2004", 2);
  printOLED(0, 55, "Hold L:Back", 1);
  display.display();

  short btn = getButton(10);
  if (btn == 3) currentScreen = 2;
  else if (btn == 4) currentScreen = 0;
}

static void handleActiveToolScreen() {
  switch (menuIndex) {
    case 0: handleStopwatchScreen(); break;
    case 1:
      if (tmState == 0) handleTimerSetupScreen();
      else handleTimerRunScreen();
      break;
    case 2: handleSetClockScreen(); break;
    case 3: handleAboutScreen(); break;
  }
}

void loopOLED() {
  if (millis() - lastOledRefreshMs < 150) {
    return;
  }
  lastOledRefreshMs = millis();

  display.clearDisplay();

  if (noSleep) {
    drawStatusBar();
  }

  if (!noSleep) {
    handleSleepScreen();
  } else if (currentScreen == 0) {
    handleHomeScreen();
  } else if (currentScreen == 1) {
    handleClockIconScreen();
  } else if (currentScreen == 2) {
    handleMenuListScreen();
  } else if (currentScreen == 3) {
    handleActiveToolScreen();
  }
}
