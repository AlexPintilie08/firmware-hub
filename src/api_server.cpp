#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "system_state.h"

const char* BACKEND_URL = "http://172.20.10.4:4000/api/esp-update";

static unsigned long lastBackendSend = 0;

void apiServerInit() {
  Serial.println("API backend client ready");
}

String boolJson(bool value) {
  return value ? "true" : "false";
}

void apiServerUpdate() {
  unsigned long now = millis();

  if (now - lastBackendSend < 1500) return;
  lastBackendSend = now;

  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(600);
  http.begin(BACKEND_URL);
  http.addHeader("Content-Type", "application/json");

  String json = "{";

  json += "\"wearable\":{";
  json += "\"status\":\"" + alertLevel + "\",";
  json += "\"battery\":" + String(batteryPercent) + ",";
  json += "\"connection\":\"online\"";
  json += "},";

  json += "\"physiology\":{";
  json += "\"bpm\":" + String(bpmAvg, 0) + ",";
  json += "\"spo2\":" + String(spo2) + ",";
  json += "\"bodyTemperature\":" + String(bodyTempC, 1) + ",";
  json += "\"stressLevel\":\"" + stressLevel + "\"";
  json += "},";

  json += "\"motion\":{";
  json += "\"accX\":" + String(accX, 2) + ",";
  json += "\"accY\":" + String(accY, 2) + ",";
  json += "\"accZ\":" + String(accZ, 2) + ",";
  json += "\"gyroX\":" + String(gyroX, 1) + ",";
  json += "\"gyroY\":" + String(gyroY, 1) + ",";
  json += "\"gyroZ\":" + String(gyroZ, 1) + ",";
  json += "\"accTotal\":" + String(accTotal, 2) + ",";
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

  Serial.print("POST backend: ");
  Serial.println(code);

  http.end();
}