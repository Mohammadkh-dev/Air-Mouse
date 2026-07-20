#include "battery.h"
#include "app_state.h"
#include "config.h"

void updateBattery() {
  uint32_t pinMilliVolts = analogReadMilliVolts(BATTERY_PIN);
  float batteryVoltage = (pinMilliVolts / 1000.0f) * 2.0f;
  float batteryPercentage = ((batteryVoltage - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN)) * 100.0f;

  if (batteryPercentage >= 80.0f) batteryLevel = 5;
  else if (batteryPercentage >= 60.0f) batteryLevel = 4;
  else if (batteryPercentage >= 40.0f) batteryLevel = 3;
  else if (batteryPercentage >= 20.0f) batteryLevel = 2;
  else if (batteryPercentage >= 10.0f) batteryLevel = 1;
  else batteryLevel = 0;
}
