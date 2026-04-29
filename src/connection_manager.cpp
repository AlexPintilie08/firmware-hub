#include <Arduino.h>
#include <WiFi.h>

#include "connection_manager.h"
#include "ble_service.h"

void wifiManagerInit();
void wifiManagerUpdate();

void apiServerInit();
void apiServerUpdate();

static ConnectionMode currentMode = CONNECTION_MODE_BLE;
static bool wifiStarted = false;
static bool bleStarted = false;
static unsigned long lastModePrint = 0;

static void startWifiStack() {
  Serial.println("Starting WIFI stack...");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  if (!wifiStarted) {
    wifiManagerInit();
    apiServerInit();
    wifiStarted = true;
  } else if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin();
  }

  Serial.println("WIFI stack ready");
}

static void stopWifiStackSoft() {
  Serial.println("Soft stopping WIFI...");

  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);

  Serial.println("WIFI soft stopped");
}

static void startBleStack() {
  Serial.println("Starting BLE stack...");

  if (!bleStarted) {
    bleServiceInit();
    bleStarted = true;
  } else {
    bleServiceStart();
  }

  Serial.println("BLE stack ready");
}

static void stopBleStackSoft() {
  Serial.println("Soft stopping BLE...");

  bleConnected = false;

  Serial.println("BLE soft stopped");
}

void connectionManagerInit() {
  Serial.println("Connection manager init");

  currentMode = CONNECTION_MODE_BLE;

  stopWifiStackSoft();
  startBleStack();

  Serial.println("Connection manager: BLE mode");
}

void connectionManagerUpdate() {
  if (currentMode == CONNECTION_MODE_WIFI) {
    wifiManagerUpdate();
    apiServerUpdate();
  }

  if (millis() - lastModePrint > 5000) {
    lastModePrint = millis();
    Serial.print("Connection mode: ");
    Serial.println(connectionManagerModeName());
  }
}

void connectionManagerFastUpdate() {
  if (currentMode == CONNECTION_MODE_BLE) {
    bleServiceUpdate();
  }
}

void connectionManagerSetMode(ConnectionMode mode) {
  if (currentMode == mode) return;

  if (mode == CONNECTION_MODE_WIFI) {
    Serial.println("Switching to WIFI mode...");

    stopBleStackSoft();
    delay(100);

    currentMode = CONNECTION_MODE_WIFI;
    startWifiStack();

    Serial.println("WIFI mode active");
    return;
  }

  if (mode == CONNECTION_MODE_BLE) {
    Serial.println("Switching to BLE mode...");

    stopWifiStackSoft();
    delay(100);

    currentMode = CONNECTION_MODE_BLE;
    startBleStack();

    Serial.println("BLE mode active");
    return;
  }
}

void connectionManagerToggleMode() {
  if (currentMode == CONNECTION_MODE_WIFI) {
    connectionManagerSetMode(CONNECTION_MODE_BLE);
  } else {
    connectionManagerSetMode(CONNECTION_MODE_WIFI);
  }
}

ConnectionMode connectionManagerGetMode() {
  return currentMode;
}

bool connectionManagerIsBleMode() {
  return currentMode == CONNECTION_MODE_BLE;
}

bool connectionManagerIsWifiMode() {
  return currentMode == CONNECTION_MODE_WIFI;
}

const char* connectionManagerModeName() {
  if (currentMode == CONNECTION_MODE_WIFI) return "WIFI";
  return "BLE";
}

ConnectionMode getConnectionMode() {
  return connectionManagerGetMode();
}

bool isWifiMode() {
  return connectionManagerIsWifiMode();
}

bool isBleMode() {
  return connectionManagerIsBleMode();
}

void switchToWifiMode() {
  connectionManagerSetMode(CONNECTION_MODE_WIFI);
}

void switchToBleMode() {
  connectionManagerSetMode(CONNECTION_MODE_BLE);
}