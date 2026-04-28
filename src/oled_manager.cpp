#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "system_state.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static bool oledOk = false;
static unsigned long lastDraw = 0;

extern bool bleConnected;

// ---------- ICONS ----------
void drawWifiIcon(int x, int y, bool ok) {
  if (!ok) {
    display.drawLine(x, y, x + 8, y + 8, BLACK);
    display.drawLine(x + 8, y, x, y + 8, BLACK);
    return;
  }

  display.drawPixel(x + 4, y + 8, BLACK);
  display.drawCircle(x + 4, y + 8, 3, BLACK);
  display.drawCircle(x + 4, y + 8, 6, BLACK);
  display.fillRect(x - 3, y + 8, 16, 8, WHITE);
}

void drawBtIcon(int x, int y, bool ok) {
  display.drawLine(x + 3, y, x + 3, y + 10, BLACK);
  display.drawLine(x + 3, y, x + 8, y + 4, BLACK);
  display.drawLine(x + 8, y + 4, x + 3, y + 7, BLACK);
  display.drawLine(x + 3, y + 7, x + 8, y + 10, BLACK);
  display.drawLine(x + 8, y + 10, x + 3, y + 5, BLACK);

  if (ok) {
    display.fillCircle(x + 11, y + 2, 2, BLACK);
  }
}

void drawBatteryIcon(int x, int y, int percent) {
  percent = constrain(percent, 0, 100);

  display.drawRect(x, y, 14, 7, BLACK);
  display.fillRect(x + 14, y + 2, 2, 3, BLACK);

  int fillWidth = map(percent, 0, 100, 0, 10);
  display.fillRect(x + 2, y + 2, fillWidth, 3, BLACK);
}

// ---------- COMMON UI ----------
void drawTopBar(const char* title) {
  display.fillRect(0, 0, 128, 14, WHITE);
  display.setTextColor(BLACK);

  display.setCursor(2, 3);
  display.print(title);

  String pageText = String(page + 1) + "/" + String(PAGE_COUNT);
  int pageX = 74 - pageText.length() * 3;
  display.setCursor(pageX, 3);
  display.print(pageText);

  drawWifiIcon(88, 2, WiFi.status() == WL_CONNECTED);
  drawBtIcon(101, 2, bleConnected);
  drawBatteryIcon(113, 3, batteryPercent);

  display.setTextColor(WHITE);
}

void drawBar(int x, int y, int w, int h, int percent) {
  percent = constrain(percent, 0, 100);
  display.drawRect(x, y, w, h, WHITE);
  display.fillRect(x, y, map(percent, 0, 100, 0, w), h, WHITE);
}

void drawBigInt(int x, int y, int value, const char* unit) {
  display.setTextSize(2);
  display.setCursor(x, y);
  display.print(value);
  display.setTextSize(1);
  display.print(unit);
  display.setTextSize(1);
}

void drawBigFloat(int x, int y, float value, const char* unit, int decimals) {
  display.setTextSize(2);
  display.setCursor(x, y);
  display.print(value, decimals);
  display.setTextSize(1);
  display.print(unit);
  display.setTextSize(1);
}

// ---------- PAGES ----------
void pageStatus() {
  drawTopBar("STATUS");

  display.setCursor(0, 18);
  display.print("ALERTA: ");
  display.print(alertLevel);

  display.setCursor(0, 30);
  display.print("RISC: ");
  display.print(riskScore);
  display.print("%");

  display.setCursor(0, 42);
  display.print("Predictie:");
  display.setCursor(0, 52);
  display.print(prediction);
}

void pagePulse() {
  drawTopBar("PULS");

  if (irValue < 25000) {
    display.setCursor(0, 24);
    display.print("Pune degetul");
    display.setCursor(0, 38);
    display.print("pe senzor");
    return;
  }

  if (bpmAvg <= 0) {
    display.setCursor(0, 30);
    display.print("Calibrare...");
    return;
  }

  drawBigInt(25, 24, (int)bpmAvg, " BPM");

  display.setCursor(0, 50);
  if (bpmAvg < 55) display.print("puls scazut");
  else if (bpmAvg <= 95) display.print("puls normal");
  else if (bpmAvg <= 125) display.print("puls crescut");
  else display.print("stres/efort mare");
}

void pageSpo2() {
  drawTopBar("OXIGEN");

  if (spo2 <= 0) {
    display.setCursor(0, 30);
    display.print("Astept semnal...");
    return;
  }

  drawBigInt(38, 24, spo2, "%");

  display.setCursor(0, 50);
  if (spo2 < 90) display.print("oxigen critic");
  else if (spo2 < 94) display.print("oxigen scazut");
  else display.print("oxigen normal");
}

void pageBodyTemp() {
  drawTopBar("TEMP");

  drawBigFloat(25, 24, bodyTempC, "C", 1);

  display.setCursor(0, 50);
  if (bodyTempC > 38.0) display.print("temp ridicata");
  else if (bodyTempC < 35.5) display.print("temp joasa");
  else display.print("temp normala");
}

void pageAccel() {
  drawTopBar("ACCEL");

  display.setCursor(0, 17);
  display.print("X:");
  display.print(dynX, 2);
  display.print(" Y:");
  display.print(dynY, 2);

  display.setCursor(0, 29);
  display.print("Z:");
  display.print(dynZ, 2);
  display.print(" G:");
  display.print(accTotal, 2);

  float dyn = abs(accTotal - 1.0);

  display.setCursor(0, 41);
  display.print("Dinamic:");
  display.print(dyn, 2);
  display.print("g");

  display.setCursor(0, 53);
  display.print(positionChanged ? "pozitie schimbata" : "pozitie stabila");
}

void pageGyro() {
  drawTopBar("GIRO");

  display.setCursor(0, 18);
  display.print("GX:");
  display.print(gyroX, 0);
  display.print(" dps");

  display.setCursor(0, 31);
  display.print("GY:");
  display.print(gyroY, 0);
  display.print(" dps");

  display.setCursor(0, 44);
  display.print("GZ:");
  display.print(gyroZ, 0);
  display.print(" dps");
}

void pageAI() {
  drawTopBar("AI");

  display.setCursor(0, 17);
  display.print("Stress:");
  display.print(stressLevel);

  display.setCursor(0, 29);
  display.print("Fall:");
  display.print(freeFallRisk ? "DA" : "NU");

  display.setCursor(62, 29);
  display.print("Rot:");
  display.print(excessiveRotation ? "DA" : "NU");

  display.setCursor(0, 41);
  display.print("NoMove:");
  display.print(noMovement ? "DA" : "NU");

  display.setCursor(0, 53);
  display.print("Risk:");
  display.print(riskScore);
  display.print("%");
}

void pageBattery() {
  drawTopBar("POWER");

  display.setCursor(0, 17);
  display.print("V:");
  display.print(voltage, 2);
  display.print("V ");

  display.print("I:");
  display.print(currentMa, 0);
  display.print("mA");

  display.setCursor(0, 30);
  display.print("Total:");
  display.print(currentTotalmAh, 1);
  display.print("mAh");

  display.setCursor(0, 43);
  display.print("Life:");
  display.print(estimatedBatteryLifeHours, 1);
  display.print("h");

  drawBar(74, 45, 48, 7, batteryPercent);
}

void pageNetwork() {
  drawTopBar("RETEA");

  display.setCursor(0, 18);
  display.print(WiFi.status() == WL_CONNECTED ? "WiFi ONLINE" : "WiFi OFFLINE");

  display.setCursor(0, 31);
  display.print("RSSI:");
  display.print(WiFi.RSSI());

  display.setCursor(0, 44);
  display.print(WiFi.localIP());
}

void pageSystem() {
  drawTopBar("SISTEM");

  display.setCursor(0, 18);
  display.print("CPU:");
  display.print(cpuLoad);
  display.print("%");

  display.setCursor(0, 31);
  display.print("RAM:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB");

  display.setCursor(0, 44);
  display.print("Freq:");
  display.print(ESP.getCpuFreqMHz());
  display.print("MHz");
}

// ---------- PUBLIC ----------
void oledManagerInit() {
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  if (!oledOk) return;

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Wearable boot...");
  display.display();

  lastDraw = 0;
  page = 0;
}

void oledManagerUpdate() {
  if (!oledOk) return;

  static bool firstDraw = true;

  if (!firstDraw && millis() - lastDraw < 250) return;

  firstDraw = false;
  lastDraw = millis();

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  switch (page) {
    case 0: pageStatus(); break;
    case 1: pagePulse(); break;
    case 2: pageSpo2(); break;
    case 3: pageBodyTemp(); break;
    case 4: pageAccel(); break;
    case 5: pageGyro(); break;
    case 6: pageAI(); break;
    case 7: pageBattery(); break;
    case 8: pageNetwork(); break;
    case 9: pageSystem(); break;
    default:
      page = 0;
      pageStatus();
      break;
  }

  display.display();
}