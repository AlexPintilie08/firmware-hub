#include <Arduino.h>
#include <WiFi.h>
#include "system_state.h"

static unsigned long lastCpuUpdate = 0;

void cpuTask() {
  if (millis() - lastCpuUpdate < 1000) return;
  lastCpuUpdate = millis();

  cpuLoad = 10;

  if (WiFi.status() == WL_CONNECTED) cpuLoad += 8;
  if (bpmAvg > 0) cpuLoad += 6;
  if (accTotal > 0.1) cpuLoad += 5;
  if (riskScore > 30) cpuLoad += 4;

  cpuLoad += millis() % 7;
  cpuLoad = constrain(cpuLoad, 0, 100);
}