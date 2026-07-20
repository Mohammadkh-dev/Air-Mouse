#include <Wire.h>
#include "imu.h"

bool imuInitialized = false;
uint8_t imuAddr = 0x68;
int16_t gyroBiasX = 0;
int16_t gyroBiasY = 0;
int16_t gyroBiasZ = 0;

static bool writeRegister(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return (Wire.endTransmission() == 0);
}

static bool readRegisters(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  
  Wire.requestFrom(addr, len);
  if (Wire.available() < len) return false;
  for (uint8_t i = 0; i < len; i++) {
    buffer[i] = Wire.read();
  }
  return true;
}

bool initIMU() {
  // Probe I2C address (0x68 or 0x69)
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() != 0) {
    Wire.beginTransmission(0x69);
    if (Wire.endTransmission() != 0) {
      Serial.println("[IMU] Device not found at 0x68 or 0x69");
      return false;
    }
    imuAddr = 0x69;
  } else {
    imuAddr = 0x68;
  }
  
  Serial.printf("[IMU] Found device at 0x%02X\n", imuAddr);
  
  // Verify Device ID (WHO_AM_I)
  uint8_t whoami = 0;
  if (!readRegisters(imuAddr, 0x75, &whoami, 1) || whoami != 0x67) {
    Serial.printf("[IMU] WHO_AM_I failed: 0x%02X (expected 0x67)\n", whoami);
    return false;
  }
  Serial.println("[IMU] WHO_AM_I matched 0x67");
  
  // Software Reset
  writeRegister(imuAddr, 0x02, 0x10);
  delay(50); // Wait for reset to complete
  
  // Enable both Gyro and Accel in Low Noise (LN) mode
  writeRegister(imuAddr, 0x1F, 0x0F);
  delay(50); // Wait for sensors to wake up
  
  // Configure Gyro: ±250 dps UI range, 100 Hz ODR
  writeRegister(imuAddr, 0x20, 0x69);
  delay(10);
  
  // Configure Accel: ±2g UI range, 100 Hz ODR
  writeRegister(imuAddr, 0x21, 0x69);
  delay(10);
  
  imuInitialized = true;
  
  // Calibrate Gyro to find stationary biases
  calibrateIMU();
  
  return true;
}

void calibrateIMU() {
  Serial.println("[IMU] Calibrating gyro biases... Please keep the ring stationary!");
  int32_t sumX = 0, sumY = 0, sumZ = 0;
  int samples = 100;
  int count = 0;
  
  while (count < samples) {
    int16_t accel[3], gyro[3];
    if (readIMUData(accel, gyro)) {
      sumX += gyro[0];
      sumY += gyro[1];
      sumZ += gyro[2];
      count++;
    }
    delay(10);
  }
  
  gyroBiasX = sumX / samples;
  gyroBiasY = sumY / samples;
  gyroBiasZ = sumZ / samples;
  Serial.printf("[IMU] Calibration complete. Biases -> X: %d, Y: %d, Z: %d\n", gyroBiasX, gyroBiasY, gyroBiasZ);
}

bool readIMUData(int16_t accel[3], int16_t gyro[3]) {
  uint8_t buffer[12];
  if (!readRegisters(imuAddr, 0x0B, buffer, 12)) {
    return false;
  }
  
  // Standard two's complement reconstruction
  accel[0] = (int16_t)((buffer[0] << 8) | buffer[1]);
  accel[1] = (int16_t)((buffer[2] << 8) | buffer[3]);
  accel[2] = (int16_t)((buffer[4] << 8) | buffer[5]);
  
  gyro[0] = (int16_t)((buffer[6] << 8) | buffer[7]);
  gyro[1] = (int16_t)((buffer[8] << 8) | buffer[9]);
  gyro[2] = (int16_t)((buffer[10] << 8) | buffer[11]);
  
  return true;
}
