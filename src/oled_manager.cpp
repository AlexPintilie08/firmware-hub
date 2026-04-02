#include "oled_manager.h"
#include "system_state.h"
#include "hub_config.h"
#include "wifi_manager.h"
#include "sensor_manager.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define I2C_SDA I2C_SDA_PIN
#define I2C_SCL I2C_SCL_PIN

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static bool oledRefreshNeeded = true;

static const char* getPageTitle(int page) {
  switch (page) {
    case 0: return "MONITORIZARE";
    case 1: return "ENERGIE";
    case 2: return "RETEA WiFi";
    case 3: return "SISTEM";
    case 4: return "MODULE";
    default: return "UNKNOWN";
  }
}

static int signalBarsFromRssi(int rssi) {
  if (rssi >= -60) return 4;
  if (rssi >= -70) return 3;
  if (rssi >= -80) return 2;
  if (rssi >= -90) return 1;
  return 0;
}

static void drawBatteryTopBar(int percent) {
  display.setCursor(72, 0);

  if (percent > 0) {
    display.print(percent);
    display.print("%");
  } else {
    display.print("--");
  }

  display.drawRect(105, 0, 20, 8, WHITE);
  display.fillRect(125, 2, 2, 4, WHITE);

  if (percent > 0) {
    int fill = map(percent, 0, 100, 0, 20);
    display.fillRect(105, 0, fill, 8, WHITE);
  }
}

static void drawSignalBarsCompact(int x, int y, int rssi, bool connected) {
  int bars = connected ? signalBarsFromRssi(rssi) : 0;

  for (int i = 0; i < 4; i++) {
    int barX = x + i * 3;
    int barH = 2 + i * 2;
    int barY = y + (8 - barH);

    if (i < bars) {
      display.fillRect(barX, barY, 2, barH, WHITE);
    } else {
      display.drawRect(barX, barY, 2, barH, WHITE);
    }
  }
}

static void drawHeader(const char* wifiText, int batteryPercent, int rssi, bool wifiConnected) {
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print(wifiText);

  drawSignalBarsCompact(54, 0, rssi, wifiConnected);
  drawBatteryTopBar(batteryPercent);
}

static void drawBar(int x, int y, int w, int h, int percent) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  display.drawRect(x, y, w, h, WHITE);
  int fillW = map(percent, 0, 100, 0, w);
  if (fillW > 0) {
    display.fillRect(x, y, fillW, h, WHITE);
  }
}

void oledInit() {
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed");
    return;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10, 25);
  display.println("OLED READY");
  display.display();

  Serial.println("OLED init ok");
}

void oledUpdate() {
  HubState& hub = getHubState();
  OledState& oled = getOledState();
  TelemetryState& t = getTelemetryState();

  ComponentState& wifi = getWifiComponentState();
  ComponentState& ina = getIna219State();
  ComponentState& ntc = getNtcState();
  ComponentState& bmi = getBmi160State();
  ComponentState& rtc = getRtcState();
  ComponentState& oledComp = getOledComponentState();

  bool wifiConnected = wifiIsConnected();
  int rssi = wifiGetRSSI();

  bool hasIna = ina.status == "online";
  bool hasTemp = ntc.status == "online" && t.temperatureC > 0.0f;

  display.clearDisplay();
  display.setTextSize(1);

  drawHeader(wifiConnected ? "WiFi:OK" : "WiFi:..", t.batteryPercent, rssi, wifiConnected);

  switch (oled.currentPage) {
    case 0: { // MONITORIZARE
      display.setCursor(18, 12);
      display.print("MONITORIZARE");
      display.drawFastHLine(0, 22, 128, WHITE);

      display.setCursor(0, 30);
      display.print("TEMP: ");

      if (hasTemp) {
        display.print(t.temperatureC, 1);
        display.print("C");

        int tempPercent = constrain(map((int)(t.temperatureC * 10), 150, 450, 0, 100), 0, 100);
        drawBar(75, 30, 50, 7, tempPercent);
      } else {
        display.print("OFF");
        drawBar(75, 30, 50, 7, 0);
      }

      display.setCursor(0, 45);
      display.print("CPU:  ");
      if (t.cpuLoadPercent > 0) {
        display.print(t.cpuLoadPercent);
        display.print("%");
        drawBar(75, 45, 50, 7, t.cpuLoadPercent);
      } else {
        display.print("--");
        drawBar(75, 45, 50, 7, 0);
      }

      display.setCursor(0, 56);
      display.print("Bond: ");
      display.print(MODULE_MOTION_DEVICE_PRESENT ? "ONLINE" : "SEARCH");
      break;
    }

    case 1: { // ENERGIE
      display.setCursor(30, 12);
      display.print("ENERGIE");
      display.drawFastHLine(0, 22, 128, WHITE);

      display.setCursor(0, 30);
      display.print("Tensiune:  ");
      if (hasIna) {
        display.print(t.voltageV, 2);
        display.println(" V");
      } else {
        display.println("OFF");
      }

      display.print("Consum:  ");
      if (hasIna) {
        display.print(t.currentmA, 1);
        display.println(" mA");
      } else {
        display.println("OFF");
      }

      display.print("Autonomie: ");
      if (hasIna && t.batteryLifeH > 0.0f) {
        display.print(t.batteryLifeH, 1);
        display.println(" h");
      } else {
        display.println("--");
      }

      drawBar(0, 58, 128, 6, t.batteryPercent > 0 ? t.batteryPercent : 0);
      break;
    }

    case 2: { // RETEA WiFi
      display.setCursor(26, 12);
      display.print("RETEA WiFi");
      display.drawFastHLine(0, 22, 128, WHITE);

      display.setCursor(0, 32);
      if (wifiConnected) {
        display.println("Status: CONECTAT");
        display.print("IP: ");
        display.println(hub.ip);
        display.print("Semnal: ");
        display.print(rssi);
        display.println(" dBm");
      } else {
        display.println("Status: OFFLINE");
        display.print("IP: --");
      }
      break;
    }

    case 3: { // SISTEM
      display.setCursor(32, 12);
      display.print("SISTEM C3");
      display.drawFastHLine(0, 22, 128, WHITE);

      display.setCursor(0, 30);
      display.print("Uptime:   ");
      display.print(millis() / 1000);
      display.println(" sec");

      display.print("RAM Lib:  ");
      display.print(ESP.getFreeHeap() / 1024);
      display.println(" KB");

      display.print("CPU Load: ");
      if (t.cpuLoadPercent > 0) {
        display.print(t.cpuLoadPercent);
        display.println(" %");
      } else {
        display.println("--");
      }
      break;
    }

    case 4: {
    display.clearDisplay();
    
    // Header discret
    display.drawFastHLine(0, 10, 128, WHITE);
    display.setCursor(0, 0);
    display.printf("R:%d P:%d", (int)roll, (int)pitch);

    // Centru Instrument
    int cx = 64, cy = 40;
    
    // Limităm Pitch-ul vizual la +/- 20 pixeli
    int pOffset = map(constrain((int)pitch, -60, 60), -60, 60, -20, 20);
    float rRad = roll * M_PI / 180.0f;

    // Calculăm capetele bării (lungime 30 pixeli)
    int lx = 30;
    int x0 = cx - cos(rRad) * lx;
    int y0 = cy - sin(rRad) * lx + pOffset;
    int x1 = cx + cos(rRad) * lx;
    int y1 = cy + sin(rRad) * lx + pOffset;

    // Desenăm "Gimbal-ul"
    display.drawCircle(cx, cy, 3, WHITE);          // Avionul fix
    display.drawLine(cx-5, cy, cx+5, cy, WHITE);   // Aripile avionului
    display.drawLine(x0, y0, x1, y1, WHITE);       // Orizontul mobil
    
    // Cadru de referință
    display.drawRect(34, 15, 60, 48, WHITE);
    
    break;
}
    
  }

  display.display();
  oledClearRefreshFlag();
}

void oledRequestRefresh() {
  oledRefreshNeeded = true;
}

bool oledNeedsRefresh() {
  return oledRefreshNeeded;
}

void oledClearRefreshFlag() {
  oledRefreshNeeded = false;
}