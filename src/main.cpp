#include <Arduino.h>
#include <Wire.h>

#include "ble_service.h"

#define I2C_SDA 8
#define I2C_SCL 9

void sensorManagerInit();
void sensorManagerUpdate();

void oledManagerInit();
void oledManagerUpdate();

void buttonManagerInit();
void buttonManagerUpdate();

void wifiManagerInit();
void wifiManagerUpdate();

void apiServerInit();
void apiServerUpdate();

void cpuTask();

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("BOOT START");

  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("I2C OK");

  buttonManagerInit();
  Serial.println("BUTTON OK");

  oledManagerInit();
  Serial.println("OLED OK");

  wifiManagerInit();
  Serial.println("WIFI OK");

  apiServerInit();
  Serial.println("API OK");

  sensorManagerInit();
  Serial.println("SENSOR OK");

  // BLE dezactivat temporar pentru test reboot
  // bleServiceInit();
  // Serial.println("BLE OK");

  oledManagerUpdate();

  Serial.println("SETUP DONE");
}

void loop() {
  buttonManagerUpdate();
  sensorManagerUpdate();
  cpuTask();
  wifiManagerUpdate();
  oledManagerUpdate();
  apiServerUpdate();
  bleServiceUpdate();
  delay(2);
}