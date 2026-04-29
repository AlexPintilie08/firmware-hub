#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "system_state.h"

const char* BACKEND_URL = "http://192.168.0.113:4000/api/esp-update";
static unsigned long lastBackendSend = 0;

static String boolJson(bool value) {
  return value ? "true" : "false";
}

void apiServerInit() {
  Serial.println("API backend ready");
}

void apiServerUpdate() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (millis() - lastBackendSend < 2000) return;
  lastBackendSend = millis();

  WiFiClient client;
  HTTPClient http;

  http.setTimeout(300);

  if (!http.begin(client, BACKEND_URL)) {
    Serial.println("HTTP begin failed");
    return;
  }

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
  json += "\"accX\":" + String(dynX, 2) + ",";
  json += "\"accY\":" + String(dynY, 2) + ",";
  json += "\"accZ\":" + String(dynZ, 2) + ",";
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