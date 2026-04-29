#include <Arduino.h>
#include <NimBLEDevice.h>
#include "ble_service.h"
#include "system_state.h"

bool bleConnected = false;

static NimBLEServer* bleServer = nullptr;
static NimBLECharacteristic* dataChar = nullptr;
static bool bleInitialized = false;
static unsigned long lastBleSend = 0;
static unsigned long lastDebugPrint = 0;

#define BLE_SERVICE_UUID "7b4a0001-9c7d-4f9a-bb2f-a1b2c3d4e001"
#define BLE_DATA_UUID    "7b4a0002-9c7d-4f9a-bb2f-a1b2c3d4e002"

#define BLE_SEND_INTERVAL_MS 50
#define BLE_DEBUG_INTERVAL_MS 3000

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server) {
    bleConnected = true;
    Serial.println("BLE connected");
  }

  void onDisconnect(NimBLEServer* server) {
    bleConnected = false;
    Serial.println("BLE disconnected");

    if (bleInitialized) {
      NimBLEDevice::startAdvertising();
    }
  }
};

static String boolJson(bool v) {
  return v ? "true" : "false";
}

static String safeText(const String& value, const String& fallback) {
  if (value.length() == 0) return fallback;
  return value;
}

static String buildBlePayload() {
  String json = "{";

  json += "\"t\":" + String(millis()) + ",";
  json += "\"s\":\"" + alertLevel + "\",";
  json += "\"r\":" + String(riskScore) + ",";
  json += "\"p\":\"" + prediction + "\",";

  json += "\"b\":" + String(bpmAvg > 0 ? bpmAvg : bpm, 0) + ",";
  json += "\"o\":" + String(spo2) + ",";
  json += "\"c\":" + String(bodyTempC, 1) + ",";
  json += "\"q\":\"" + stressLevel + "\",";

  json += "\"bt\":" + String(batteryPercent) + ",";
  json += "\"v\":" + String(voltage, 2) + ",";
  json += "\"i\":" + String(currentMa, 1) + ",";

  json += "\"x\":" + String(dynX, 2) + ",";
  json += "\"y\":" + String(dynY, 2) + ",";
  json += "\"z\":" + String(dynZ, 2) + ",";
  json += "\"a\":" + String(accTotal, 2) + ",";

  json += "\"u\":" + String(gyroX, 0) + ",";
  json += "\"v2\":" + String(gyroY, 0) + ",";
  json += "\"w\":" + String(gyroZ, 0) + ",";

  json += "\"f\":" + String(freeFallRisk ? 1 : 0) + ",";
  json += "\"g\":" + String(excessiveRotation ? 1 : 0) + ",";
  json += "\"n\":" + String(noMovement ? 1 : 0);

  json += "}";

  return json;
}

void bleServiceInit() {
  if (bleInitialized) return;

  Serial.println("BLE init");

  NimBLEDevice::init("SKYSAFE-WEAR");
  NimBLEDevice::setPower(ESP_PWR_LVL_P6);

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());

  NimBLEService* service = bleServer->createService(BLE_SERVICE_UUID);

  dataChar = service->createCharacteristic(
    BLE_DATA_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  dataChar->setValue("ready");

  service->start();

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);

  NimBLEDevice::startAdvertising();

  bleInitialized = true;

  Serial.println("BLE ready");
}
void bleServiceStart() {
  if (!bleInitialized) {
    bleServiceInit();
    return;
  }

  Serial.println("BLE start");

  bleConnected = false;
  NimBLEDevice::startAdvertising();
}
void bleServiceUpdate() {
  if (!bleInitialized) return;
  if (!dataChar) return;

  bool realConnected = false;

  if (bleServer != nullptr) {
    realConnected = bleServer->getConnectedCount() > 0;
  }

  bleConnected = realConnected;

  if (millis() - lastDebugPrint > BLE_DEBUG_INTERVAL_MS) {
    lastDebugPrint = millis();

    Serial.print("BLE | connected=");
    Serial.print(bleConnected ? "YES" : "NO");
    Serial.print(" | clients=");
    Serial.println(bleServer ? bleServer->getConnectedCount() : 0);
  }

  if (!bleConnected) return;

  if (millis() - lastBleSend < BLE_SEND_INTERVAL_MS) return;
  lastBleSend = millis();

  String payload = buildBlePayload();

  dataChar->setValue(payload.c_str());
  dataChar->notify();
}

void bleServiceStop() {
  if (!bleInitialized) return;

  Serial.println("BLE stop");

  NimBLEDevice::stopAdvertising();

  if (bleServer && bleServer->getConnectedCount() > 0) {
    bleServer->disconnect(0);
  }

  bleConnected = false;
}