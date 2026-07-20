#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include "bluetooth.h"
#include "app_state.h"
#include "config.h"

class BleServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    deviceConnected = true;
    Serial.println("Connected to Windows via BLE!");
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
    deviceConnected = false;
    Serial.println("BLE Disconnected!");
    if (!mode) {
      NimBLEDevice::startAdvertising();
    }
  }
};

void startBluetoothHardware() {
  if (!bleAdvertisingActive) {
    Serial.println("[BLE] Activating Bluetooth Advertising...");
    NimBLEDevice::getAdvertising()->start();
    bleAdvertisingActive = true;
  }
}

void stopBluetoothHardware() {
  if (bleAdvertisingActive || deviceConnected) {
    Serial.println("[BLE] Deactivating Bluetooth (Dongle Active)...");
    NimBLEDevice::getAdvertising()->stop();
    if (pServer && deviceConnected) {
      pServer->disconnect(0);
    }
    deviceConnected = false;
    bleAdvertisingActive = false;
  }
}

void setupBluetooth() {
  NimBLEDevice::init("ESP32-C3 Input Combo");
  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new BleServerCallbacks());

  pHid = new NimBLEHIDDevice(pServer);
  pMouseInput = pHid->getInputReport(1);
  pKeyInput = pHid->getInputReport(2);

  pHid->setManufacturer("Espressif");
  pHid->setPnp(0x02, 0x05ac, 0x0223, 0x0110);
  pHid->setHidInfo(0x00, 0x01);

  const uint8_t reportMap[] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x01, 0xa1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03,
    0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 0x81, 0x03, 0x05, 0x01,
    0x09, 0x30, 0x09, 0x31, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x02,
    0x81, 0x06, 0xc0, 0xc0,
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, 0x02, 0x05, 0x07, 0x19, 0xe0,
    0x29, 0xe7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x08, 0x81, 0x03, 0x95, 0x06, 0x75, 0x08, 0x15, 0x00,
    0x25, 0x65, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xc0
  };

  pHid->setReportMap((uint8_t*)reportMap, sizeof(reportMap));
  pHid->startServices();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setAppearance(0x03C1);
  pAdvertising->addServiceUUID(pHid->getHidService()->getUUID());

  bleAdvertisingActive = false;
}

void loopBluetooth() {
  if (!mode && deviceConnected) {
    // 1. Mouse movements & clicks
    int8_t mx = constrain(accumMouseX, -127, 127);
    int8_t my = constrain(accumMouseY, -127, 127);

    static bool lastSentLeftClick = false;
    if (mx != 0 || my != 0 || leftClicked != lastSentLeftClick) {
      uint8_t buttons = leftClicked ? 0x01 : 0x00;
      // Mouse report size is 3 bytes (buttons, X, Y)
      uint8_t mouseReport[3] = {buttons, (uint8_t)mx, (uint8_t)my};
      pMouseInput->setValue(mouseReport, sizeof(mouseReport));
      pMouseInput->notify();
      
      accumMouseX -= mx;
      accumMouseY -= my;
      lastSentLeftClick = leftClicked;
    }

    // 2. Keyboard Task View (Win + Tab) trigger
    static unsigned long keyTimer = 0;
    static enum { KEY_IDLE, KEY_PRESSED } keyState = KEY_IDLE;

    if (taskViewTriggered && keyState == KEY_IDLE) {
      // 0x08 = Left GUI (Win key), 0x2B = Tab key code
      uint8_t keyReport[8] = {0x08, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x00, 0x00};
      pKeyInput->setValue(keyReport, sizeof(keyReport));
      pKeyInput->notify();
      
      keyTimer = millis();
      keyState = KEY_PRESSED;
      taskViewTriggered = false; // consumed
    }

    if (keyState == KEY_PRESSED) {
      if (millis() - keyTimer >= 50) {
        uint8_t releaseReport[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        pKeyInput->setValue(releaseReport, sizeof(releaseReport));
        pKeyInput->notify();
        keyState = KEY_IDLE;
      }
    }
  }
}
