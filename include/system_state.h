#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>

struct HubState {
  String deviceName;
  String status;
  String ip;
  int clients;
};

struct TelemetryState {
  float accelZ;
  float temperatureC;
  float voltageV;
  float currentmA;
  float currentTotalmAh;
  int batteryPercent;
  float batteryLifeH;
  int cpuLoadPercent;
};

struct OledState {
  int currentPage;
  String pageTitle;
  String lastActionSource;
};

struct ComponentState {
  String name;
  String hostDevice;
  String status;
  String message;
};

struct LogEntry {
  unsigned long timestamp;
  String message;
};

#define LOG_MAX 20

void stateInit();

HubState& getHubState();
void stateUpdateIp(const String& ip);
void stateUpdateClients(int clients);

OledState& getOledState();
void oledSetPage(int page, const String& title, const String& source);

TelemetryState& getTelemetryState();

ComponentState& getIna219State();
ComponentState& getNtcState();
ComponentState& getBmi160State();
ComponentState& getRtcState();
ComponentState& getOledComponentState();
ComponentState& getWifiComponentState();

LogEntry* getLogs();
int getLogCount();
void addLog(const String& msg);

#endif