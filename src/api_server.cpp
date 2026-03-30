#include "api_server.h"
#include <WebServer.h>
#include "wifi_manager.h"
#include "system_state.h"
#include "hub_config.h"
#include "oled_manager.h"

static WebServer server(80);

static const char* pageTitleFromIndex(int page) {
  switch (page) {
    case 0: return "HOME";
    case 1: return "TEMP";
    case 2: return "POWER";
    case 3: return "NET";
    case 4: return "MOD";
    default: return "UNK";
  }
}

static void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
server.send(200, "text/plain", "Mr Hub API online");
}

static void handleData() {
  HubState& hub = getHubState();
  OledState& oled = getOledState();
  TelemetryState& t = getTelemetryState();

  ComponentState& wifi = getWifiComponentState();
  ComponentState& ina = getIna219State();
  ComponentState& ntc = getNtcState();
  ComponentState& bmi = getBmi160State();
  ComponentState& rtc = getRtcState();
  ComponentState& oledComp = getOledComponentState();

  String json = "{";

  json += "\"hub\":{";
  json += "\"name\":\"" + hub.deviceName + "\",";
  json += "\"status\":\"" + hub.status + "\",";
  json += "\"ip\":\"" + hub.ip + "\",";
  json += "\"clients\":" + String(hub.clients) + ",";
  json += "\"wifiConnected\":" + String(wifiIsConnected() ? "true" : "false") + ",";
  json += "\"rssi\":" + String(wifiGetRSSI());
  json += "},";

  json += "\"telemetry\":{";
  json += "\"temperature\":" + String(t.temperatureC, 1) + ",";
  json += "\"voltage\":" + String(t.voltageV, 2) + ",";
  json += "\"current\":" + String(t.currentmA, 1) + ",";
  json += "\"currentTotalmAh\":" + String(t.currentTotalmAh, 1) + ",";
  json += "\"batteryPercent\":" + String(t.batteryPercent) + ",";
  json += "\"batteryLifeH\":" + String(t.batteryLifeH, 1) + ",";
  json += "\"cpuLoadPercent\":" + String(t.cpuLoadPercent);
  json += "},";

  json += "\"components\":{";
  json += "\"wifi\":{\"status\":\"" + wifi.status + "\",\"message\":\"" + wifi.message + "\"},";
  json += "\"oled\":{\"status\":\"" + oledComp.status + "\",\"message\":\"" + oledComp.message + "\"},";
  json += "\"ina219\":{\"status\":\"" + ina.status + "\",\"message\":\"" + ina.message + "\"},";
  json += "\"ntc\":{\"status\":\"" + ntc.status + "\",\"message\":\"" + ntc.message + "\"},";
  json += "\"bmi160\":{\"status\":\"" + bmi.status + "\",\"message\":\"" + bmi.message + "\"},";
  json += "\"rtc\":{\"status\":\"" + rtc.status + "\",\"message\":\"" + rtc.message + "\"},";
  json += "\"motion\":{\"status\":\"offline\",\"message\":\"Ms Motion offline\"}";
  json += "},";

  json += "\"oled\":{";
  json += "\"page\":" + String(oled.currentPage) + ",";
  json += "\"title\":\"" + oled.pageTitle + "\",";
  json += "\"lastActionSource\":\"" + oled.lastActionSource + "\"";
  json += "},";

  json += "\"logs\":[";
  LogEntry* logs = getLogs();
  for (int i = 0; i < getLogCount(); i++) {
    json += "{";
    json += "\"timestamp\":" + String(logs[i].timestamp) + ",";
    json += "\"message\":\"" + logs[i].message + "\"";
    json += "}";
    if (i < getLogCount() - 1) json += ",";
  }
  json += "]";

  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
server.send(200, "application/json", json);
}

static void handleNextPage() {
  OledState& oled = getOledState();

  oled.currentPage = (oled.currentPage + 1) % OLED_PAGE_COUNT;
  oled.pageTitle = pageTitleFromIndex(oled.currentPage);
  oled.lastActionSource = "web";

  addLog("OLED page changed: " + oled.pageTitle + " via WEB NEXT");
  oledRequestRefresh();

  server.sendHeader("Access-Control-Allow-Origin", "*");
server.send(200, "text/plain", "OK");

}

static void handlePrevPage() {
  OledState& oled = getOledState();

  oled.currentPage--;
  if (oled.currentPage < 0) {
    oled.currentPage = OLED_PAGE_COUNT - 1;
  }

  oled.pageTitle = pageTitleFromIndex(oled.currentPage);
  oled.lastActionSource = "web";

  addLog("OLED page changed: " + oled.pageTitle + " via WEB PREV");
  oledRequestRefresh();

  server.sendHeader("Access-Control-Allow-Origin", "*");
server.send(200, "text/plain", "OK");
}

void apiServerInit() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleData);
  server.on("/api/next", HTTP_GET, handleNextPage);
  server.on("/api/prev", HTTP_GET, handlePrevPage);

  server.begin();
  Serial.println("API server pornit.");
}

void apiServerHandle() {
  server.handleClient();
}