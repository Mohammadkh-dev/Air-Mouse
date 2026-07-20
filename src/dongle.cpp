#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <driver/i2s.h>
#include "dongle.h"
#include "bluetooth.h"
#include "app_state.h"
#include "config.h"
/*
changed this from
static void packetState(const wifi_tx_info_t* info, esp_now_send_status_t status)
to
below because it was not the way the esp32 core at the curent esp32 core version (2.0.7) expects the callback to be defined. The callback signature has changed in the newer versions of the esp32 core which was available on arduino ide but not here and i dont know why.
*/
static void packetState(const uint8_t* mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    sendFailureCounter++;
  } else {
    sendFailureCounter = 0;
  }
}

void setupDongle() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed!");
  }
  esp_now_register_send_cb(packetState);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 24,
    .dma_buf_len = AUDIO_SAMPLES,
    .use_apll = false
  };

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT_0, &pin_config);
  i2s_start(I2S_PORT_0);
}

void loopDongle() {
  if (mode == true) {
    stopBluetoothHardware();

    if (sendFailureCounter > 60) {
      mode = false;
      Serial.println("Dongle Disconnected. Switching to Bluetooth Mode...");
    }

    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT_0, &rawBuffer, sizeof(rawBuffer), &bytesIn, portMAX_DELAY);
    if (result == ESP_OK && bytesIn > 0) {
      int samplesRead = bytesIn / 4;
      for (int i = 0; i < samplesRead; i++) {
        if (mic) {
          dataPacket.audio[i] = (int16_t)(rawBuffer[i] >> 14);
        } else {
          dataPacket.audio[i] = 0;
        }
      }
    }

    dataPacket.mouseX = accumMouseX;
    accumMouseX -= dataPacket.mouseX;
    dataPacket.mouseY = accumMouseY;
    accumMouseY -= dataPacket.mouseY;
    dataPacket.rightClick = false;
    dataPacket.leftClick = leftClicked;
    
    if (taskViewTriggered) {
      dataPacket.action = 1;
      taskViewTriggered = false; // consumed
    } else {
      dataPacket.action = 0;
    }

    esp_now_send(receiverAddress, (uint8_t*)&dataPacket, sizeof(dataPacket));
    dongleCheck = millis();
  } else {
    startBluetoothHardware();

    if (millis() - dongleCheck > 10000) {
      Serial.println("Probing Dongle Connection...");
      dataPacket.action = 99;
      esp_err_t testSend = esp_now_send(receiverAddress, (uint8_t*)&dataPacket, sizeof(dataPacket));

      if (testSend == ESP_OK && sendFailureCounter < 60) {
        mode = true;
        Serial.println("Dongle reconnected successfully!");
      }
      dongleCheck = millis();
    }
  }
}

void audioTask(void* pvParameters) {
  for (;;) {
    loopDongle();

    if (!mode) {
      vTaskDelay(pdMS_TO_TICKS(10));
    } else {
      taskYIELD();
    }
  }
}
