#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

void sensorsInit();
void sensorsUpdate();
void sensorsSetCpuLoad(int cpuLoadPercent);
#endif