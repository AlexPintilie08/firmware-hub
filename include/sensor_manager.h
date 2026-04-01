#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

extern float roll;
extern float pitch;

void sensorsInit();
// Funcție separată pentru INA219 și Temperatura (apelată o dată pe secundă)
void sensorsUpdateSlow();
// Funcție pentru calculul giroscopului (apelată rapid în bucla principală)
void sensorsUpdateFast();
void sensorsSetCpuLoad(int cpuLoadPercent);

#endif