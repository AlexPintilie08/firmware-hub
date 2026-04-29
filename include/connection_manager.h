#pragma once

enum ConnectionMode {
  CONNECTION_MODE_BLE = 0,
  CONNECTION_MODE_WIFI = 1
};

void connectionManagerInit();
void connectionManagerUpdate();

void connectionManagerToggleMode();
void connectionManagerSetMode(ConnectionMode mode);

ConnectionMode connectionManagerGetMode();

bool connectionManagerIsBleMode();
bool connectionManagerIsWifiMode();

const char* connectionManagerModeName();