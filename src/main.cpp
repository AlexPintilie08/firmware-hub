#include <Arduino.h>
<<<<<<< HEAD
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>

// ===== CONFIG =====
const char* ssid = "Net";
const char* password = "12345678";
const char* backendUrl = "http://172.20.10.4:4000/api/esp-update";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define PCF8591_ADDR 0x48

#define I2C_SDA 8
#define I2C_SCL 9

#define BTN_NEXT 2
#define BTN_PREV 3

// ===== OBJECTS =====
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
extern Adafruit_INA219 ina219; // IMPORTANT

// ===== STATE =====
int page = 0;
const int PAGE_COUNT = 8;

uint8_t lightRaw = 0;
uint8_t tempRaw = 0;

float filteredVoltage = 3.7;
float filteredCurrent = 50.0;
float filteredTempRaw = 150.0;

float tempC = 0;
int luxValue = 0;
int batteryPercent = 0;
int cpuLoad = 0;

float alpha = 0.05;

// ===== TIMERS =====
unsigned long tSensor, tDisplay, tWifi, tBackend, tButtons, tCPU;

// ===== PCF =====
uint8_t readPCF(uint8_t ch) {
  Wire.beginTransmission(PCF8591_ADDR);
  Wire.write(0x40 | ch);
  Wire.endTransmission();
  Wire.requestFrom(PCF8591_ADDR, 2);
  if (Wire.available()) Wire.read();
  if (Wire.available()) return Wire.read();
  return 0;
}

// ===== UI HELPERS =====
void drawTop(const char* title) {
  display.fillRect(0, 0, 128, 12, WHITE);
  display.setTextColor(BLACK);
  display.setCursor(2, 2);
  display.print(title);

  display.setCursor(100, 2);
  display.print(page + 1);
  display.print("/");
  display.print(PAGE_COUNT);

  display.setTextColor(WHITE);
}

void drawBar(int y, int value, int max) {
  int w = map(value, 0, max, 0, 100);
  display.drawRect(14, y, 100, 6, WHITE);
  display.fillRect(14, y, w, 6, WHITE);
}

void drawBottom() {
  display.drawFastHLine(0, 56, 128, WHITE);
  int w = map(page, 0, PAGE_COUNT-1, 8, 128);
  display.fillRect(0, 58, w, 4, WHITE);
}

// ===== BUTTONS =====
void buttonTask() {
  static unsigned long last = 0;
  static unsigned long holdN = 0;
  static unsigned long holdP = 0;

  bool n = digitalRead(BTN_NEXT) == LOW;
  bool p = digitalRead(BTN_PREV) == LOW;

  unsigned long now = millis();

  if (n && p && now - last > 400) {
    page = 0;
    last = now;
    return;
  }

  if (n) {
    if (!holdN) holdN = now;
    int speed = (now - holdN > 700) ? 150 : 300;

    if (now - last > speed) {
      page = (page + 1) % PAGE_COUNT;
      last = now;
    }
  } else holdN = 0;

  if (p) {
    if (!holdP) holdP = now;
    int speed = (now - holdP > 700) ? 150 : 300;

    if (now - last > speed) {
      page--;
      if (page < 0) page = PAGE_COUNT - 1;
      last = now;
    }
  } else holdP = 0;
}

// ===== SENSOR =====
void sensorTask() {
  if (millis() - tSensor < 200) return;
  tSensor = millis();

  lightRaw = readPCF(0);
  tempRaw = readPCF(1);

  filteredVoltage = alpha * ina219.getBusVoltage_V() + (1 - alpha) * filteredVoltage;
  filteredCurrent = alpha * ina219.getCurrent_mA() + (1 - alpha) * filteredCurrent;
  filteredTempRaw = alpha * tempRaw + (1 - alpha) * filteredTempRaw;

  batteryPercent = constrain(map(filteredVoltage * 100, 330, 420, 0, 100), 0, 100);
  luxValue = map(255 - lightRaw, 0, 255, 0, 1000);
  tempC = 19.5 + (235 - filteredTempRaw) * 0.3;
}

// ===== CPU =====
void cpuTask() {
  if (millis() - tCPU < 1000) return;
  tCPU = millis();
  cpuLoad = 15 + (millis() % 10);
}

// ===== WIFI =====
void wifiTask() {
  if (millis() - tWifi < 5000) return;
  tWifi = millis();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  }
}

// ===== DISPLAY =====
void displayTask() {
  if (millis() - tDisplay < 250) return;
  tDisplay = millis();

  display.clearDisplay();

  switch (page) {

    case 0:
      drawTop("STATUS");
      display.setCursor(0, 18);
      display.print("TEMP: "); display.print(tempC,1); display.print("C");
      display.setCursor(0, 30);
      display.print("LUX: "); display.print(luxValue);
      display.setCursor(0, 42);
      display.print("BAT: "); display.print(batteryPercent); display.print("%");
      break;

    case 1:
      drawTop("TEMPERATURA");
      display.setCursor(30, 26);
      display.setTextSize(2);
      display.print(tempC,1);
      display.print("C");
      display.setTextSize(1);
      break;

    case 2:
      drawTop("LUMINA");
      display.setCursor(40, 26);
      display.setTextSize(2);
      display.print(luxValue);
      display.setTextSize(1);
      drawBar(46, luxValue, 1000);
      break;

    case 3:
      drawTop("WIFI");
      display.setCursor(0, 20);
      display.print(WiFi.status() == WL_CONNECTED ? "ONLINE" : "OFFLINE");
      display.setCursor(0, 32);
      display.print(WiFi.localIP());
      break;

    case 4:
      drawTop("ENERGIE");
      display.setCursor(0, 20);
      display.print("V: "); display.print(filteredVoltage,2);
      display.setCursor(0, 32);
      display.print("I: "); display.print(filteredCurrent,1);
      break;

    case 5:
      drawTop("BATERIE");
      display.setCursor(40, 26);
      display.setTextSize(2);
      display.print(batteryPercent);
      display.print("%");
      display.setTextSize(1);
      drawBar(46, batteryPercent, 100);
      break;

    case 6:
      drawTop("SISTEM");
      display.setCursor(0, 20);
      display.print("CPU: "); display.print(cpuLoad); display.print("%");
      display.setCursor(0, 32);
      display.print("RAM: "); display.print(ESP.getFreeHeap()/1024);
      break;

    case 7:
      drawTop("RAW");
      display.setCursor(0, 20);
      display.print(lightRaw);
      display.setCursor(0, 32);
      display.print(tempRaw);
      break;
  }

  drawBottom();
  display.display();
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  WiFi.mode(WIFI_STA);
}

// ===== LOOP =====
void loop() {
  buttonTask();
  sensorTask();
  cpuTask();
  wifiTask();
  displayTask();
=======
#include <WebSocketsServer.h>
#include "wifi_manager.h"
#include "api_server.h"
#include "system_state.h"
#include "oled_manager.h"
#include "button_manager.h"
#include "sensor_manager.h"

static unsigned long sensorTimer = 0;
static unsigned long clientTimer = 0;
static unsigned long cpuTimer = 0;
static unsigned long busyAccum = 0;

WebSocketsServer webSocket(81);

// Temporizator dedicat pentru expedierea datelor WebSockets
static unsigned long wsTimer = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    stateInit();
    wifiInit();
    stateUpdateIp(wifiGetIP());

    oledInit();
    buttonsInit();
    sensorsInit();

    oledRequestRefresh();
    webSocket.begin();
    apiServerInit();

    addLog("System state initialized");
    addLog("WiFi initialized");
    addLog("OLED initialized");
    addLog("Sensors initialized");
    addLog("API server initialized");
    Serial.println("SYSTEM READY");
}

void loop() {
    unsigned long loopStart = micros();
    unsigned long nowMs = millis();

    webSocket.loop();
    apiServerHandle();
    buttonsUpdate();

    // Actualizare ultrarapidă pentru accelerometru și giroscop
    sensorsUpdate();

    if (nowMs - wsTimer >= 30) {
        // Preluăm variabilele externe din sensor_manager.h și creăm un JSON minimal
        // În loop, la timer-ul de WebSockets:
        String wsJson = "{\"roll\":" + String(roll, 2) + 
                ",\"pitch\":" + String(pitch, 2) + 
                ",\"accel\":" + String(getTelemetryState().accelZ, 2) + "}";
         webSocket.broadcastTXT(wsJson);
        
        wsTimer = nowMs;
    }

    // Actualizare lentă (doar 1 dată pe secundă) pt NTC/INA219
    if (nowMs - sensorTimer >= 1000) {
        sensorsUpdateSlow();
        oledRequestRefresh();
        sensorTimer = nowMs;
    }

    if (nowMs - clientTimer >= 2000) {
        stateUpdateClients(wifiGetClientCount());
        stateUpdateIp(wifiGetIP());
        oledRequestRefresh();
        clientTimer = nowMs;
    }

    unsigned long loopEnd = micros();
    busyAccum += (loopEnd - loopStart);

    if (nowMs - cpuTimer >= 1000) {
        int cpuLoad = busyAccum / 10000UL;
        if (cpuLoad > 100) cpuLoad = 100;
        sensorsSetCpuLoad(cpuLoad);
        oledRequestRefresh();
        busyAccum = 0;
        cpuTimer = nowMs;
    }

    if (oledNeedsRefresh()) {
        oledUpdate();
    }

    delay(10);
>>>>>>> ed6891eaf7230787d1585379404ecf0fa97a0999
}