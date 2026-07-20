#include <Arduino.h>
#include "config.h"
#include "app_state.h"
#include "battery.h"
#include "bluetooth.h"
#include "clock.h"
#include "display.h"
#include "dongle.h"
#include "ui.h"
#include "imu.h"

void readInputs() {
  if (imuInitialized) {
    int16_t accel[3];
    int16_t gyro[3];
    if (readIMUData(accel, gyro)) {
      // Calibrate gyroscope outputs by subtracting stationary biases
      int16_t gx = gyro[0] - gyroBiasX;
      int16_t gy = gyro[1] - gyroBiasY;
      int16_t gz = gyro[2] - gyroBiasZ;

      // Apply noise deadzone threshold
      if (abs(gx) < GYRO_DEADZONE) gx = 0;
      if (abs(gy) < GYRO_DEADZONE) gy = 0;
      if (abs(gz) < GYRO_DEADZONE) gz = 0;

      // Map yaw and pitch to cursor movement
      // Moving finger left/right is yaw (rotation around Z). Z gyro controls mouse X.
      // Moving finger up/down is pitch (rotation around Y). Y gyro controls mouse Y.
      int16_t mx = -gz / SENSITIVITY_DIVIDER;
      int16_t my = gy / SENSITIVITY_DIVIDER;  // Inverted sign for pitch to correct direction

      accumMouseX += mx;
      accumMouseY += my;
    }
  }

  // Read touchpad inputs
  bool currentLeftTouch = (digitalRead(PIN_TOUCH_LEFT) == TOUCH_PRESSED);
  bool currentTaskTouch = (digitalRead(PIN_TOUCH_TASK) == TOUCH_PRESSED);

  leftClicked = currentLeftTouch;

  // Task view is edge-triggered once on tap
  static bool lastTaskTouch = false;
  if (currentTaskTouch && !lastTaskTouch) {
    taskViewTriggered = true;
  }
  lastTaskTouch = currentTaskTouch;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TOUCH_LEFT, INPUT);
  pinMode(PIN_TOUCH_TASK, INPUT);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  if (!initDisplay()) {
    Serial.println("SSD1306 allocation failed");
    for (;;) {}
  }

  // Initialize the ICM-42670P IMU
  if (!initIMU()) {
    Serial.println("IMU initialization failed! Continuing without air mouse...");
  }

  display.clearDisplay();
  printOLED(25, 25, "Welcome", 2);
  display.display();
  delay(2000);
  display.clearDisplay();

  clockTime = ((INIT_HOUR * 3600) + (INIT_MIN * 60) + INIT_SEC) % 86400;
  lastUpdateMs = millis();
  dongleCheck = millis();

  setupDongle();
  setupBluetooth();

  xTaskCreate(audioTask, "AudioTask", 4096, NULL, 5, &audioTaskHandle);
}

void loop() {
  updateClock();
  updateBattery();

  static unsigned long lastInputReadMs = 0;
  if (millis() - lastInputReadMs >= 10) {
    lastInputReadMs = millis();
    readInputs();
  }

  loopBluetooth();
  loopOLED();
}
