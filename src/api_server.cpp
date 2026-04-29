#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "system_state.h"

const char* BACKEND_URL = "http://192.168.0.113:4000/api/esp-update";

static unsigned long lastBackendSend = 0;

// Wi-Fi fallback live. 250ms = 4Hz, stabil pentru HTTP.
// Pentru BLE accelerometrul va merge separat la 50ms.
#define BACKEND_SEND_INTERVAL_MS 250

static String boolJson(bool value) {
  return value ? "true" : "false";
}

void apiServerInit() {
  Serial.println("API backend ready");
}

void apiServerUpdate() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (millis() - lastBackendSend < BACKEND_SEND_INTERVAL_MS) return;
  lastBackendSend = millis();

  WiFiClient client;
  HTTPClient http;

  http.setTimeout(250);
  http.setReuse(false);

  if (!http.begin(client, BACKEND_URL)) {
    Serial.println("HTTP begin failed");
    return;
  }

  http.addHeader("Content-Type", "application/json");

  String json;
  json.reserve(1200);

  json = "{";

  json += "\"wearable\":{";
  json += "\"status\":\"" + alertLevel + "\",";
  json += "\"battery\":" + String(batteryPercent) + ",";
  json += "\"connection\":\"wifi-backup\"";
  json += "},";

  json += "\"physiology\":{";
  json += "\"bpm\":" + String(bpmAvg > 0 ? bpmAvg : bpm, 0) + ",";
  json += "\"spo2\":" + String(spo2) + ",";
  json += "\"bodyTemperature\":" + String(bodyTempC, 1) + ",";
  json += "\"stressLevel\":\"" + stressLevel + "\"";
  json += "},";

  json += "\"motion\":{";
  json += "\"accX\":" + String(dynX, 3) + ",";
  json += "\"accY\":" + String(dynY, 3) + ",";
  json += "\"accZ\":" + String(dynZ, 3) + ",";
  json += "\"gyroX\":" + String(gyroX, 1) + ",";
  json += "\"gyroY\":" + String(gyroY, 1) + ",";
  json += "\"gyroZ\":" + String(gyroZ, 1) + ",";
  json += "\"accTotal\":" + String(accTotal, 3) + ",";
  json += "\"parachuteOpened\":" + boolJson(parachuteOpened) + ",";
  json += "\"positionChanged\":" + boolJson(positionChanged) + ",";
  json += "\"freeFallRisk\":" + boolJson(freeFallRisk) + ",";
  json += "\"excessiveRotation\":" + boolJson(excessiveRotation) + ",";
  json += "\"noMovement\":" + boolJson(noMovement);
  json += "},";

  json += "\"ai\":{";
  json += "\"riskScore\":" + String(riskScore) + ",";
  json += "\"prediction\":\"" + prediction + "\",";
  json += "\"alert\":\"" + alertLevel + "\"";
  json += "},";

  json += "\"system\":{";
  json += "\"cpuLoad\":{\"value\":" + String(cpuLoad) + ",\"unit\":\"%\"},";
  json += "\"voltage\":{\"value\":" + String(voltage, 2) + ",\"unit\":\"V\"},";
  json += "\"currentNow\":{\"value\":" + String(currentMa, 1) + ",\"unit\":\"mA\"},";
  json += "\"battery\":{\"percent\":" + String(batteryPercent) + "}";
  json += "},";

  json += "\"wireless\":{";
  json += "\"connected\":true,";
  json += "\"ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"rssi\":{\"value\":" + String(WiFi.RSSI()) + ",\"unit\":\"dBm\"}";
  json += "}";

  json += "}";

  int code = http.POST(json);

  if (code <= 0) {
    Serial.print("POST backend failed: ");
    Serial.println(code);
  }

  http.end();
}