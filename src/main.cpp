#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include "MAX30105.h"
#include "heartRate.h"

// WIFI + BACKEND
const char* ssid = "Net";
const char* password = "12345678";
const char* backendUrl = "http://172.20.10.4:4000/api/esp-update";

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// I2C
#define I2C_SDA 8
#define I2C_SCL 9

// BUTTONS
#define BTN_NEXT 2
#define BTN_PREV 3

// NTC
#define NTC_PIN 1

// BMI160 I2C
#define BMI160_ADDR 0x68

// OBJECTS
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
extern Adafruit_INA219 ina219;
MAX30105 max30102;

// STATE
int page = 0;
const int PAGE_COUNT = 10;

bool oledOk = false;
bool inaOk = false;
bool maxOk = false;
bool bmiOk = false;

float bodyTempC = 36.5;
float voltage = 3.7;
float currentMa = 0;
int batteryPercent = 0;
int cpuLoad = 0;

float bpm = 0;
float bpmAvg = 0;
int spo2 = 0;

long irValue = 0;
long redValue = 0;

float accX = 0, accY = 0, accZ = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;
float accTotal = 0;

bool parachuteOpened = false;
bool positionChanged = false;
bool freeFallRisk = false;
bool excessiveRotation = false;
bool noMovement = false;

String stressLevel = "NORMAL";
String alertLevel = "SAFE";
String prediction = "normal";
int riskScore = 0;

// TIMERS
unsigned long tSensors = 0;
unsigned long tMax = 0;
unsigned long tDisplay = 0;
unsigned long tWifi = 0;
unsigned long tBackend = 0;
unsigned long tButtons = 0;
unsigned long tCpu = 0;

unsigned long lastMovementTime = 0;

// ---------- I2C HELPERS ----------
void writeReg8(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(addr, len);
  if (Wire.available() < len) return false;

  for (int i = 0; i < len; i++) {
    buffer[i] = Wire.read();
  }

  return true;
}

// ---------- BMI160 DIRECT ----------
bool initBMI160() {
  uint8_t chipId = 0;

  if (!readBytes(BMI160_ADDR, 0x00, &chipId, 1)) {
    return false;
  }

  if (chipId != 0xD1) {
    return false;
  }

  writeReg8(BMI160_ADDR, 0x7E, 0x11); // accel normal
  delay(50);
  writeReg8(BMI160_ADDR, 0x7E, 0x15); // gyro normal
  delay(100);

  writeReg8(BMI160_ADDR, 0x40, 0x28); // accel config
  writeReg8(BMI160_ADDR, 0x41, 0x03); // accel range +-2g

  writeReg8(BMI160_ADDR, 0x42, 0x28); // gyro config
  writeReg8(BMI160_ADDR, 0x43, 0x00); // gyro range +-2000 dps

  return true;
}

void readBMI160() {
  if (!bmiOk) return;

  uint8_t data[12];

  if (!readBytes(BMI160_ADDR, 0x0C, data, 12)) {
    return;
  }

  int16_t gx = (int16_t)((data[1] << 8) | data[0]);
  int16_t gy = (int16_t)((data[3] << 8) | data[2]);
  int16_t gz = (int16_t)((data[5] << 8) | data[4]);

  int16_t ax = (int16_t)((data[7] << 8) | data[6]);
  int16_t ay = (int16_t)((data[9] << 8) | data[8]);
  int16_t az = (int16_t)((data[11] << 8) | data[10]);

  gyroX = gx / 16.4;
  gyroY = gy / 16.4;
  gyroZ = gz / 16.4;

  accX = ax / 16384.0;
  accY = ay / 16384.0;
  accZ = az / 16384.0;

  accTotal = sqrt(accX * accX + accY * accY + accZ * accZ);

  if (abs(accTotal - 1.0) > 0.12) {
    positionChanged = true;
    lastMovementTime = millis();
  } else {
    positionChanged = false;
  }

  freeFallRisk = accTotal < 0.35;
  excessiveRotation = abs(gyroX) > 450 || abs(gyroY) > 450 || abs(gyroZ) > 450;
  noMovement = millis() - lastMovementTime > 6000;

  parachuteOpened = accTotal > 2.2;
}

// ---------- NTC ----------
float readNTC() {
  int adc = analogRead(NTC_PIN);

  if (adc <= 0 || adc >= 4095) {
    return bodyTempC;
  }

  const float seriesR = 10000.0;
  const float nominalR = 10000.0;
  const float nominalT = 25.0;
  const float beta = 3950.0;

  float r = seriesR * ((4095.0 / adc) - 1.0);

  float steinhart = r / nominalR;
  steinhart = log(steinhart);
  steinhart /= beta;
  steinhart += 1.0 / (nominalT + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;

  return steinhart;
}

// ---------- UI ----------
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 13, WHITE);
  display.setTextColor(BLACK);

  display.setCursor(2, 3);
  display.print(title);

  // WiFi icon
  display.setCursor(78, 3);
  display.print(WiFi.status() == WL_CONNECTED ? "W" : "x");

  // battery icon
  display.drawRect(94, 3, 20, 7, BLACK);
  display.fillRect(114, 5, 2, 3, BLACK);
  int bw = map(batteryPercent, 0, 100, 0, 18);
  display.fillRect(95, 4, bw, 5, BLACK);

  display.setCursor(118, 3);
  display.print(page + 1);

  display.setTextColor(WHITE);
}

void drawFooter() {
  display.drawFastHLine(0, 56, 128, WHITE);

  int w = map(page + 1, 1, PAGE_COUNT, 10, 128);
  display.fillRect(0, 58, w, 4, WHITE);
}

void drawValueBig(int x, int y, float value, const char* unit, int decimals = 0) {
  display.setTextSize(2);
  display.setCursor(x, y);
  display.print(value, decimals);
  display.setTextSize(1);
  display.print(unit);
  display.setTextSize(1);
}

void drawBar(int x, int y, int w, int h, int percent) {
  percent = constrain(percent, 0, 100);
  display.drawRect(x, y, w, h, WHITE);
  display.fillRect(x, y, map(percent, 0, 100, 0, w), h, WHITE);
}

// ---------- BUTTONS ----------
void buttonTask() {
  if (millis() - tButtons < 120) return;
  tButtons = millis();

  static unsigned long lastAction = 0;
  static unsigned long holdNext = 0;
  static unsigned long holdPrev = 0;

  bool next = digitalRead(BTN_NEXT) == LOW;
  bool prev = digitalRead(BTN_PREV) == LOW;
  unsigned long now = millis();

  if (next && prev && now - lastAction > 500) {
    page = 0;
    lastAction = now;
    return;
  }

  if (next) {
    if (holdNext == 0) holdNext = now;
    int speed = (now - holdNext > 800) ? 180 : 350;

    if (now - lastAction > speed) {
      page = (page + 1) % PAGE_COUNT;
      lastAction = now;
    }
  } else {
    holdNext = 0;
  }

  if (prev) {
    if (holdPrev == 0) holdPrev = now;
    int speed = (now - holdPrev > 800) ? 180 : 350;

    if (now - lastAction > speed) {
      page--;
      if (page < 0) page = PAGE_COUNT - 1;
      lastAction = now;
    }
  } else {
    holdPrev = 0;
  }
}

// ---------- SENSOR TASK ----------
void sensorTask() {
  if (millis() - tSensors < 250) return;
  tSensors = millis();

  float ntc = readNTC();
  bodyTempC = 0.08 * ntc + 0.92 * bodyTempC;

  if (inaOk) {
    voltage = 0.06 * ina219.getBusVoltage_V() + 0.94 * voltage;
    currentMa = 0.06 * ina219.getCurrent_mA() + 0.94 * currentMa;
  }

  batteryPercent = constrain(
    map(voltage * 100, 330, 420, 0, 100),
    0,
    100
  );

  readBMI160();
}

// ---------- MAX30102 TASK ----------
void maxTask() {
  if (!maxOk) return;
  if (millis() - tMax < 25) return;
  tMax = millis();

  irValue = max30102.getIR();
  redValue = max30102.getRed();

  if (irValue > 50000) {
    if (checkForBeat(irValue)) {
      static unsigned long lastBeat = 0;
      unsigned long now = millis();
      unsigned long delta = now - lastBeat;
      lastBeat = now;

      if (delta > 250 && delta < 2000) {
        bpm = 60000.0 / delta;

        if (bpm > 40 && bpm < 220) {
          if (bpmAvg == 0) bpmAvg = bpm;
          bpmAvg = 0.15 * bpm + 0.85 * bpmAvg;
        }
      }
    }

    float ratio = (float)redValue / (float)irValue;
    spo2 = constrain(110 - (int)(25 * ratio), 70, 100);
  } else {
    bpm = 0;
    bpmAvg = 0;
    spo2 = 0;
  }
}

// ---------- AI / RISK ----------
void riskTask() {
  riskScore = 0;

  if (bpmAvg > 145) riskScore += 35;
  else if (bpmAvg > 120) riskScore += 20;
  else if (bpmAvg > 95) riskScore += 10;

  if (spo2 > 0 && spo2 < 90) riskScore += 35;
  else if (spo2 > 0 && spo2 < 94) riskScore += 20;

  if (bodyTempC > 38.5) riskScore += 25;
  else if (bodyTempC > 37.8) riskScore += 12;

  if (freeFallRisk) riskScore += 35;
  if (excessiveRotation) riskScore += 25;
  if (noMovement) riskScore += 35;
  if (batteryPercent < 20) riskScore += 10;

  riskScore = constrain(riskScore, 0, 100);

  if (riskScore >= 65) {
    alertLevel = "DANGER";
    prediction = "accident risk";
  } else if (riskScore >= 35) {
    alertLevel = "WARNING";
    prediction = "abnormal";
  } else {
    alertLevel = "SAFE";
    prediction = "normal";
  }

  if (bpmAvg > 140 || spo2 < 92 || bodyTempC > 38.2) {
    stressLevel = "RIDICAT";
  } else if (bpmAvg > 105 || bodyTempC > 37.4) {
    stressLevel = "MEDIU";
  } else {
    stressLevel = "NORMAL";
  }
}

// ---------- CPU ----------
void cpuTask() {
  if (millis() - tCpu < 1000) return;
  tCpu = millis();

  cpuLoad = 12;
  if (WiFi.status() == WL_CONNECTED) cpuLoad += 8;
  if (maxOk) cpuLoad += 7;
  if (bmiOk) cpuLoad += 5;
  cpuLoad += millis() % 8;
  cpuLoad = constrain(cpuLoad, 0, 100);
}

// ---------- WIFI ----------
void wifiTask() {
  if (millis() - tWifi < 5000) return;
  tWifi = millis();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  }
}

// ---------- DISPLAY ----------
void displayTask() {
  if (!oledOk) return;
  if (millis() - tDisplay < 250) return;
  tDisplay = millis();

  display.clearDisplay();

  switch (page) {
    case 0:
      drawHeader("STATUS");
      display.setCursor(0, 18);
      display.print("ALERT: ");
      display.print(alertLevel);
      display.setCursor(0, 30);
      display.print("RISK: ");
      display.print(riskScore);
      display.print("%");
      drawBar(0, 43, 110, 7, riskScore);
      break;

    case 1:
      drawHeader("PULS");
      drawValueBig(28, 24, bpmAvg, " BPM", 0);
      display.setCursor(0, 48);
      display.print(irValue > 50000 ? "finger detected" : "no finger");
      break;

    case 2:
      drawHeader("OXIGEN");
      drawValueBig(36, 24, spo2, " %", 0);
      display.setCursor(0, 48);
      display.print("SpO2 blood oxygen");
      break;

    case 3:
      drawHeader("TEMP CORP");
      drawValueBig(30, 24, bodyTempC, " C", 1);
      display.setCursor(0, 48);
      display.print("NTC body temp");
      break;

    case 4:
      drawHeader("MISCARE");
      display.setCursor(0, 18);
      display.print("X:");
      display.print(accX, 2);
      display.print(" Y:");
      display.print(accY, 2);
      display.setCursor(0, 30);
      display.print("Z:");
      display.print(accZ, 2);
      display.print(" G:");
      display.print(accTotal, 2);
      display.setCursor(0, 44);
      display.print("Pos:");
      display.print(positionChanged ? "changed" : "stable");
      break;

    case 5:
      drawHeader("ROTATIE");
      display.setCursor(0, 18);
      display.print("GX:");
      display.print(gyroX, 0);
      display.setCursor(0, 30);
      display.print("GY:");
      display.print(gyroY, 0);
      display.setCursor(0, 42);
      display.print("GZ:");
      display.print(gyroZ, 0);
      break;

    case 6:
      drawHeader("AI ALERT");
      display.setCursor(0, 18);
      display.print("Stress: ");
      display.print(stressLevel);
      display.setCursor(0, 30);
      display.print("Fall: ");
      display.print(freeFallRisk ? "YES" : "NO");
      display.setCursor(0, 42);
      display.print("NoMove: ");
      display.print(noMovement ? "YES" : "NO");
      break;

    case 7:
      drawHeader("BATERIE");
      display.setCursor(0, 18);
      display.print("V: ");
      display.print(voltage, 2);
      display.print(" V");
      display.setCursor(0, 30);
      display.print("I: ");
      display.print(currentMa, 1);
      display.print(" mA");
      drawBar(0, 44, 110, 7, batteryPercent);
      break;

    case 8:
      drawHeader("RETEA");
      display.setCursor(0, 18);
      display.print(WiFi.status() == WL_CONNECTED ? "ONLINE" : "OFFLINE");
      display.setCursor(0, 30);
      display.print("RSSI: ");
      display.print(WiFi.RSSI());
      display.setCursor(0, 42);
      display.print(WiFi.localIP());
      break;

    case 9:
      drawHeader("SISTEM");
      display.setCursor(0, 18);
      display.print("CPU: ");
      display.print(cpuLoad);
      display.print("%");
      display.setCursor(0, 30);
      display.print("RAM: ");
      display.print(ESP.getFreeHeap() / 1024);
      display.print(" KB");
      display.setCursor(0, 42);
      display.print("Freq: ");
      display.print(ESP.getCpuFreqMHz());
      display.print(" MHz");
      break;
  }

  drawFooter();
  display.display();
}

// ---------- BACKEND ----------
void backendTask() {
  if (millis() - tBackend < 1500) return;
  tBackend = millis();

  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(500);
  http.begin(backendUrl);
  http.addHeader("Content-Type", "application/json");

  String json = "{";

  json += "\"wearable\":{";
  json += "\"status\":\"" + alertLevel + "\",";
  json += "\"battery\":" + String(batteryPercent) + ",";
  json += "\"connection\":\"" + String(WiFi.status() == WL_CONNECTED ? "online" : "offline") + "\"";
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
  json += "\"parachuteOpened\":" + String(parachuteOpened ? "true" : "false") + ",";
  json += "\"positionChanged\":" + String(positionChanged ? "true" : "false") + ",";
  json += "\"freeFallRisk\":" + String(freeFallRisk ? "true" : "false") + ",";
  json += "\"excessiveRotation\":" + String(excessiveRotation ? "true" : "false") + ",";
  json += "\"noMovement\":" + String(noMovement ? "true" : "false");
  json += "},";

  json += "\"ai\":{";
  json += "\"riskScore\":" + String(riskScore) + ",";
  json += "\"prediction\":\"" + prediction + "\",";
  json += "\"alert\":\"" + alertLevel + "\"";
  json += "},";

  json += "\"system\":{";
  json += "\"cpuLoad\":{\"value\":" + String(cpuLoad) + ",\"unit\":\"%\"},";
  json += "\"voltage\":{\"value\":" + String(voltage, 2) + ",\"unit\":\"V\"},";
  json += "\"currentNow\":{\"value\":" + String(currentMa, 1) + ",\"unit\":\"mA\"}";
  json += "},";

  json += "\"wireless\":{";
  json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"rssi\":{\"value\":" + String(WiFi.RSSI()) + ",\"unit\":\"dBm\"}";
  json += "}";

  json += "}";

  int code = http.POST(json);
  Serial.print("POST: ");
  Serial.println(code);

  http.end();
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  analogReadResolution(12);

  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Wearable boot...");
  display.display();

  inaOk = ina219.begin();

  maxOk = max30102.begin(Wire, I2C_SPEED_FAST);
  if (maxOk) {
    max30102.setup();
    max30102.setPulseAmplitudeRed(0x2A);
    max30102.setPulseAmplitudeIR(0x2A);
  }

  bmiOk = initBMI160();
  lastMovementTime = millis();

  WiFi.mode(WIFI_STA);
}

// ---------- LOOP ----------
void loop() {
  buttonTask();
  sensorTask();
  maxTask();
  riskTask();
  cpuTask();
  wifiTask();
  displayTask();
  backendTask();
}