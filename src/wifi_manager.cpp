#include <Arduino.h>
#include <WiFi.h>

const char* WIFI_SSID = "Net";
const char* WIFI_PASSWORD = "12345678";

#define WIFI_CONNECT_TIMEOUT_MS 8000
#define WIFI_RETRY_INTERVAL_MS 10000

static unsigned long lastRetry = 0;
static bool wifiConfigured = false;

void wifiManagerInit() {
  Serial.println("\n--- WIFI STA INIT ---");

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  wifiConfigured = true;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not connected yet");
  }

  lastRetry = millis();
}

void wifiManagerUpdate() {
  if (!wifiConfigured) return;

  if (WiFi.status() == WL_CONNECTED) return;

  if (millis() - lastRetry < WIFI_RETRY_INTERVAL_MS) return;
  lastRetry = millis();

  Serial.println("WiFi retry...");

  WiFi.disconnect(false);
  delay(50);
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