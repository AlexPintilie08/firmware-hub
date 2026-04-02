#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

extern float roll;
extern float pitch;

void sensorsInit();
void sensorsUpdate();      // Trebuie să fie fără "Fast"
void sensorsUpdateSlow();  // Funcția pentru temperatură NTC
void sensorsSetCpuLoad(int cpuLoadPercent);

#endif