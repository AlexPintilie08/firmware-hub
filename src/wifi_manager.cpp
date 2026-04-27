#include "wifi_manager.h"
#include "hub_config.h"
#include <WiFi.h>

static const char *ssid = HUB_WIFI_SSID;
static const char *password = HUB_WIFI_PASSWORD;

void wifiInit() {
  Serial.println("\n--- CONFIGURARE STA ---");

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_5dBm);
  WiFi.setSleep(false);
  delay(500);

  WiFi.begin(ssid, password);

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

String wifiGetIP() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "0.0.0.0";
}

int wifiGetClientCount() {
  return 1;
}

bool wifiIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

int wifiGetRSSI() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.RSSI();
  }
  return -127;
}