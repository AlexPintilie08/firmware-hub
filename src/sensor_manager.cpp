#include "sensor_manager.h"
#include "system_state.h"
#include "hub_config.h"

#include <Wire.h>
#include <Adafruit_INA219.h>

static Adafruit_INA219 ina219;
static bool inaOk = false;

void sensorsInit() {
  inaOk = false;

  if (MODULE_INA219_PRESENT) {
    inaOk = ina219.begin();
  }

  ComponentState& ina = getIna219State();

  if (inaOk) {
    ina.status = "online";
    ina.message = "INA219 active";
    Serial.println("INA219 init ok");
    addLog("INA219 detected");
  } else {
    ina.status = "offline";
    ina.message = "INA219 not detected";
    Serial.println("INA219 init failed");
    addLog("INA219 offline");
  }
}

void sensorsUpdate() {
  TelemetryState& t = getTelemetryState();
  ComponentState& ina = getIna219State();

  if (!inaOk) {
    ina.status = "offline";
    ina.message = "INA219 not detected";
    return;
  }

  float voltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA();

  t.voltageV = voltage;
  t.currentmA = current;

  // acumulare mAh reală în timp
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();

  if (lastUpdate != 0) {
    float hours = (now - lastUpdate) / 3600000.0f;
    t.currentTotalmAh += current * hours;
  }

  lastUpdate = now;

  // procent baterie real din tensiune
  float batteryPercentFloat =
      ((voltage - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN)) * 100.0f;

  if (batteryPercentFloat < 0.0f) batteryPercentFloat = 0.0f;
  if (batteryPercentFloat > 100.0f) batteryPercentFloat = 100.0f;

  t.batteryPercent = (int)batteryPercentFloat;

  // estimare durată viață
  if (current > 1.0f) {
    float remainingmAh = BATTERY_CAPACITY_MAH * (batteryPercentFloat / 100.0f);
    t.batteryLifeH = remainingmAh / current;
  } else {
    t.batteryLifeH = 0.0f;
  }

  // NTC încă nu există
  ComponentState& ntc = getNtcState();
  if (ntc.status != "online") {
    t.temperatureC = 0.0f;
  }

  ina.status = "online";
  ina.message = "INA219 active";
}

void sensorsSetCpuLoad(int cpuLoadPercent) {
  TelemetryState& t = getTelemetryState();
  t.cpuLoadPercent = cpuLoadPercent;
}