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
  if (!wifiStarted) {
    wifiManagerInit();
    apiServerInit();
    wifiStarted = true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin();
}

static void stopWifiStack() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(150);
}

static void startBleStack() {
  Serial.println("Starting BLE stack...");
  bleServiceInit();
  bleStarted = true;
}

static void stopBleStack() {
  bleServiceStop();
}

void connectionManagerInit() {
  Serial.println("Connection manager init");

  currentMode = CONNECTION_MODE_BLE;

  stopWifiStack();
  startBleStack();

  Serial.println("Connection manager: BLE mode");
}

void connectionManagerUpdate() {
  if (currentMode == CONNECTION_MODE_WIFI) {
    wifiManagerUpdate();
    apiServerUpdate();
  }

  if (currentMode == CONNECTION_MODE_BLE) {
    bleServiceUpdate();
  }

  if (millis() - lastModePrint > 5000) {
    lastModePrint = millis();

    Serial.print("Connection mode: ");
    Serial.println(connectionManagerModeName());
  }
}

void connectionManagerSetMode(ConnectionMode mode) {
  if (currentMode == mode) return;

  if (mode == CONNECTION_MODE_WIFI) {
    Serial.println("Switching to WIFI mode...");

    stopBleStack();
    delay(250);

    currentMode = CONNECTION_MODE_WIFI;

    startWifiStack();

    Serial.println("WIFI mode active");
    return;
  }

  if (mode == CONNECTION_MODE_BLE) {
    Serial.println("Switching to BLE mode...");

    stopWifiStack();
    delay(250);

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

/*
  Compatibilitate cu codul vechi, dacă mai ai fișiere care apelează
  getConnectionMode(), isWifiMode(), isBleMode(), switchToWifiMode(), switchToBleMode().
*/

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