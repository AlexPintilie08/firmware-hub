#include <Arduino.h>
#include <Wire.h>

#include "debug_manager.h"
#include "cpu_manager.h"
#include "connection_manager.h"
#include "ble_service.h"

#define I2C_SDA 8
#define I2C_SCL 9

void sensorManagerInit();
void sensorManagerUpdate();

void oledManagerInit();
void oledManagerUpdate();

void buttonManagerInit();
void buttonManagerUpdate();

void cpuTask();

/*
  TASK RAPID:
  - senzori
  - accelerometru
  - BLE (IMPORTANT pentru lag mic)
*/
void taskSensorsUi(void* parameter) {
  for (;;) {
    buttonManagerUpdate();
    sensorManagerUpdate();
    cpuTask();
    oledManagerUpdate();

    // 🔥 AICI e cheia pentru live accelerometru pe telefon
    connectionManagerFastUpdate();

    vTaskDelay(pdMS_TO_TICKS(20)); // ~50Hz
  }
}

/*
  TASK LENT:
  - WiFi
  - backend HTTP
*/
void taskConnection(void* parameter) {
  for (;;) {
    connectionManagerFastUpdate();
    connectionManagerUpdate();

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

/*
  DEBUG
*/
void taskDebug(void* parameter) {
  for (;;) {
    debugManagerUpdate();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  debugManagerInit();

  debugPrintCheckpoint("Wire init start");
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  Wire.setTimeOut(30);
  debugPrintCheckpoint("Wire init done");

  debugPrintCheckpoint("Buttons init start");
  buttonManagerInit();
  debugPrintCheckpoint("Buttons init done");

  debugPrintCheckpoint("OLED init start");
  oledManagerInit();
  debugPrintCheckpoint("OLED init done");

  debugPrintCheckpoint("Sensors init start");
  sensorManagerInit();
  debugPrintCheckpoint("Sensors init done");

  debugPrintCheckpoint("Connection init start");
  connectionManagerInit();
  debugPrintCheckpoint("Connection init done");

  xTaskCreate(taskSensorsUi, "SensorsUI", 10000, NULL, 3, NULL);
  xTaskCreate(taskConnection, "Connection", 12000, NULL, 1, NULL);
  xTaskCreate(taskDebug, "Debug", 4096, NULL, 1, NULL);

  debugPrintCheckpoint("All tasks created");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}