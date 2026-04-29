#pragma once

enum ConnectionMode {
  CONNECTION_MODE_BLE = 0,
  CONNECTION_MODE_WIFI = 1
};

void connectionManagerInit();
void connectionManagerUpdate();
void connectionManagerFastUpdate();

void connectionManagerSetMode(ConnectionMode mode);
void connectionManagerToggleMode();

ConnectionMode connectionManagerGetMode();

bool connectionManagerIsBleMode();
bool connectionManagerIsWifiMode();

const char* connectionManagerModeName();

// compatibilitate cod vechi
ConnectionMode getConnectionMode();
bool isWifiMode();
bool isBleMode();
void switchToWifiMode();
void switchToBleMode();