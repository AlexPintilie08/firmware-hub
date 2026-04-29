#include <Arduino.h>
#include <NimBLEDevice.h>
#include "ble_service.h"
#include "system_state.h"

bool bleConnected = false;

static NimBLEServer* bleServer = nullptr;
static NimBLECharacteristic* dataChar = nullptr;
static bool bleInitialized = false;
static unsigned long lastBleSend = 0;

#define BLE_SERVICE_UUID "7b4a0001-9c7d-4f9a-bb2f-a1b2c3d4e001"
#define BLE_DATA_UUID    "7b4a0002-9c7d-4f9a-bb2f-a1b2c3d4e002"

#define BLE_SEND_INTERVAL_MS 50

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

static String buildBlePayload() {
  String json = "{";

  json += "\"bpm\":" + String(bpmAvg > 0 ? bpmAvg : bpm, 0) + ",";
  json += "\"spo2\":" + String(spo2) + ",";
  json += "\"temp\":" + String(bodyTempC, 1) + ",";
  json += "\"risk\":" + String(riskScore) + ",";

  json += "\"ax\":" + String(dynX, 3) + ",";
  json += "\"ay\":" + String(dynY, 3) + ",";
  json += "\"az\":" + String(dynZ, 3) + ",";
  json += "\"at\":" + String(accTotal, 3);

  json += "}";

  return json;
}

void bleServiceInit() {
  if (bleInitialized) return;

  Serial.println("BLE init");

  NimBLEDevice::init("ESP-WEAR");
  NimBLEDevice::setPower(ESP_PWR_LVL_P6);

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());

  NimBLEService* service = bleServer->createService(BLE_SERVICE_UUID);

  dataChar = service->createCharacteristic(
    BLE_DATA_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  dataChar->setValue("ready");


  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);

  NimBLEDevice::startAdvertising();

  bleInitialized = true;
}

void bleServiceUpdate() {
  static unsigned long lastPrint = 0;

  if (!bleInitialized) return;
  if (!dataChar) return;

  bool realConnected = false;

  if (bleServer != nullptr) {
    realConnected = bleServer->getConnectedCount() > 0;
  }

  bleConnected = realConnected;

  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    Serial.print("BLE update | connected=");
    Serial.print(bleConnected ? "YES" : "NO");
    Serial.print(" | clients=");
    Serial.print(bleServer ? bleServer->getConnectedCount() : 0);
    Serial.print(" | dataChar=");
    Serial.println(dataChar ? "OK" : "NULL");
  }

  if (!bleConnected) return;

  if (millis() - lastBleSend < BLE_SEND_INTERVAL_MS) return;
  lastBleSend = millis();

  String payload = buildBlePayload();

  Serial.print("BLE SEND: ");
  Serial.println(payload);

  dataChar->setValue(payload.c_str());
  dataChar->notify();
}

void bleServiceStop() {
  if (!bleInitialized) return;

  Serial.println("BLE stop");

  NimBLEDevice::stopAdvertising();

  if (bleServer && bleConnected) {
    bleServer->disconnect(0);
  }

  bleConnected = false;
}