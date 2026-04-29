#include <Arduino.h>
#include <WiFi.h>

const char* WIFI_SSID = "Net";
const char* WIFI_PASSWORD = "12345678";

void wifiManagerInit() {
  Serial.println("\n--- CONFIGURARE WIFI STA ---");

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.setSleep(false);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectat cu succes!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Eroare la conectarea WiFi.");
  }
}

void wifiManagerUpdate() {
  static unsigned long lastRetry = 0;

  if (WiFi.status() == WL_CONNECTED) return;

  if (millis() - lastRetry < 10000) return;
  lastRetry = millis();

  Serial.println("WiFi retry...");
  WiFi.disconnect(false);
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

String wifiGetIP() {
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return "0.0.0.0";
}

bool wifiIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

int wifiGetRSSI() {
  if (WiFi.status() == WL_CONNECTED) return WiFi.RSSI();
  return -127;
}