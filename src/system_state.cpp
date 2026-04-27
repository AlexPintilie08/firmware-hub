#include "system_state.h"
#include "hub_config.h"

static HubState hubState;
static TelemetryState telemetryState;
static OledState oledState;
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
    hubState.status = "initializing";
    
    oledState.currentPage = 0;
    oledState.pageTitle = "Dashboard";

    ina219State.name = "INA219";
    ina219State.status = "offline";
    
    ntcState.name = "NTC";
    ntcState.status = "offline";

    wifiComponentState.name = "WiFi";
    wifiComponentState.status = "offline";

    oledComponentState.name = "OLED";
    oledComponentState.status = "online";
}

HubState& getHubState() { return hubState; }
void stateUpdateIp(const String& ip) { hubState.ip = ip; }
void stateUpdateClients(int clients) { hubState.clients = clients; }
OledState& getOledState() { return oledState; }
void oledSetPage(int page, const String& title, const String& source) {
    oledState.currentPage = page;
    oledState.pageTitle = title;
    oledState.lastActionSource = source;
}
TelemetryState& getTelemetryState() { return telemetryState; }
ComponentState& getIna219State() { return ina219State; }
ComponentState& getNtcState() { return ntcState; }
ComponentState& getBmi160State() { return bmi160State; }
ComponentState& getRtcState() { return rtcState; }
ComponentState& getOledComponentState() { return oledComponentState; }
ComponentState& getWifiComponentState() { return wifiComponentState; }
LogEntry* getLogs() { return logs; }
int getLogCount() { return logCount; }

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