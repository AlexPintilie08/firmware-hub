#include <Arduino.h>
#include <WiFi.h>

// WIFI
const char* WIFI_SSID = "Net";
const char* WIFI_PASSWORD = "12345678";

static unsigned long lastWifiCheck = 0;

void wifiManagerInit() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void wifiManagerUpdate() {
  unsigned long now = millis();

  if (now - lastWifiCheck < 5000) return;
  lastWifiCheck = now;

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}