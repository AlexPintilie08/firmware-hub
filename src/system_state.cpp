#include "system_state.h"
#include "hub_config.h"

static HubState hubState;
static OledState oledState;
static TelemetryState telemetryState;

static ComponentState ina219State;
static ComponentState ntcState;
static ComponentState bmi160State;
static ComponentState rtcState;
static ComponentState oledComponentState;
static ComponentState wifiComponentState;

static LogEntry logs[LOG_MAX];
static int logCount = 0;

void stateInit() {
  hubState.deviceName = HUB_NAME;
  hubState.status = "online";
  hubState.ip = "0.0.0.0";
  hubState.clients = 0;

  oledState.currentPage = 0;
  oledState.pageTitle = "HOME";
  oledState.lastActionSource = "boot";

  telemetryState.temperatureC = 0.0f;
  telemetryState.voltageV = 0.0f;
  telemetryState.currentmA = 0.0f;
  telemetryState.currentTotalmAh = 0.0f;
  telemetryState.batteryPercent = 0;
  telemetryState.batteryLifeH = 0.0f;
  telemetryState.cpuLoadPercent = 0;

  ina219State.name = "ina219";
  ina219State.hostDevice = HUB_NAME;
  ina219State.status = MODULE_INA219_PRESENT ? "online" : "offline";
  ina219State.message = MODULE_INA219_PRESENT ? "INA219 configured" : "INA219 offline";

  ntcState.name = "ntc";
  ntcState.hostDevice = HUB_NAME;
  ntcState.status = MODULE_NTC_PRESENT ? "online" : "offline";
  ntcState.message = MODULE_NTC_PRESENT ? "NTC configured" : "NTC offline";

  bmi160State.name = "bmi160";
  bmi160State.hostDevice = MOTION_NAME;
  bmi160State.status = MODULE_BMI160_PRESENT ? "online" : "offline";
  bmi160State.message = MODULE_BMI160_PRESENT ? "BMI160 configured" : "BMI160 offline";

  rtcState.name = "rtc";
  rtcState.hostDevice = HUB_NAME;
  rtcState.status = MODULE_RTC_PRESENT ? "online" : "offline";
  rtcState.message = MODULE_RTC_PRESENT ? "RTC configured" : "RTC offline";

  oledComponentState.name = "oled";
  oledComponentState.hostDevice = HUB_NAME;
  oledComponentState.status = MODULE_OLED_PRESENT ? "online" : "offline";
  oledComponentState.message = MODULE_OLED_PRESENT ? "OLED configured" : "OLED offline";

  wifiComponentState.name = "wifi";
  wifiComponentState.hostDevice = HUB_NAME;
  wifiComponentState.status = MODULE_WIFI_PRESENT ? "online" : "offline";
  wifiComponentState.message = MODULE_WIFI_PRESENT ? "WiFi configured" : "WiFi offline";
}

HubState& getHubState() {
  return hubState;
}

void stateUpdateIp(const String& ip) {
  hubState.ip = ip;
}

void stateUpdateClients(int clients) {
  hubState.clients = clients;
}

OledState& getOledState() {
  return oledState;
}

void oledSetPage(int page, const String& title, const String& source) {
  oledState.currentPage = page;
  oledState.pageTitle = title;
  oledState.lastActionSource = source;
}

TelemetryState& getTelemetryState() {
  return telemetryState;
}

ComponentState& getIna219State() {
  return ina219State;
}

ComponentState& getNtcState() {
  return ntcState;
}

ComponentState& getBmi160State() {
  return bmi160State;
}

ComponentState& getRtcState() {
  return rtcState;
}

ComponentState& getOledComponentState() {
  return oledComponentState;
}

ComponentState& getWifiComponentState() {
  return wifiComponentState;
}

LogEntry* getLogs() {
  return logs;
}

int getLogCount() {
  return logCount;
}

void addLog(const String& msg) {
  unsigned long ts = millis();

  if (logCount < LOG_MAX) {
    logs[logCount].timestamp = ts;
    logs[logCount].message = msg;
    logCount++;
  } else {
    for (int i = 1; i < LOG_MAX; i++) {
      logs[i - 1] = logs[i];
    }
    logs[LOG_MAX - 1].timestamp = ts;
    logs[LOG_MAX - 1].message = msg;
  }
}