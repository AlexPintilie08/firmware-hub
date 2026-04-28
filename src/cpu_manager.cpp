#include <Arduino.h>
#include <WiFi.h>
#include "system_state.h"

static unsigned long lastCpuUpdate = 0;

void cpuTask() {
  unsigned long now = millis();

  if (now - lastCpuUpdate < 1000) return;
  lastCpuUpdate = now;

  cpuLoad = 12;

  if (WiFi.status() == WL_CONNECTED) cpuLoad += 8;
  if (bpmAvg > 0) cpuLoad += 6;
  if (accTotal > 0.1) cpuLoad += 5;

  cpuLoad += millis() % 8;
  cpuLoad = constrain(cpuLoad, 0, 100);
}