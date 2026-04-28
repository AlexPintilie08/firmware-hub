#include <Arduino.h>
#include <NimBLEDevice.h>
#include "system_state.h"

static NimBLEServer* server = nullptr;
static NimBLECharacteristic* dataChar = nullptr;

 bool bleConnected = false;
static unsigned long lastBleSend = 0;

#define BLE_SERVICE_UUID   "7b4a0001-9c7d-4f9a-bb2f-esp32c3a10001"
#define BLE_DATA_UUID      "7b4a0002-9c7d-4f9a-bb2f-esp32c3a10002"

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    bleConnected = true;
  }

  void onDisconnect(NimBLEServer* pServer) {
    bleConnected = false;
    NimBLEDevice::startAdvertising();
  }
};

static String buildBlePayload() {
  String s = "{";

  s += "\"st\":\"" + alertLevel + "\",";
  s += "\"risk\":" + String(riskScore) + ",";
  s += "\"bpm\":" + String(bpmAvg, 0) + ",";
  s += "\"spo2\":" + String(spo2) + ",";
  s += "\"temp\":" + String(bodyTempC, 1) + ",";
  s += "\"bat\":" + String(batteryPercent) + ",";

  s += "\"ax\":" + String(accX, 2) + ",";
  s += "\"ay\":" + String(accY, 2) + ",";
  s += "\"az\":" + String(accZ, 2) + ",";

  s += "\"gx\":" + String(gyroX, 0) + ",";
  s += "\"gy\":" + String(gyroY, 0) + ",";
  s += "\"gz\":" + String(gyroZ, 0) + ",";

  s += "\"fall\":" + String(freeFallRisk ? 1 : 0) + ",";
  s += "\"rot\":" + String(excessiveRotation ? 1 : 0) + ",";
  s += "\"nomove\":" + String(noMovement ? 1 : 0);

  s += "}";

  return s;
}

void bleServiceInit() {
  NimBLEDevice::init("Parachute-Wearable");

  server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* service = server->createService(BLE_SERVICE_UUID);

  dataChar = service->createCharacteristic(
    BLE_DATA_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  dataChar->setValue("ready");

  service->start();

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);

  NimBLEDevice::startAdvertising();

  Serial.println("BLE NimBLE started");
}

void bleServiceUpdate() {
  if (!bleConnected || dataChar == nullptr) return;

  unsigned long now = millis();

  if (now - lastBleSend < 1000) return;
  lastBleSend = now;

  String payload = buildBlePayload();

  dataChar->setValue(payload.c_str());
  dataChar->notify();
}