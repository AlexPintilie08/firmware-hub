#pragma once
#include <Arduino.h>

// HEALTH
extern float bodyTempC;
extern float bpm;
extern float bpmAvg;
extern int spo2;
extern long irValue;
extern long redValue;

// POWER
extern float voltage;
extern float currentMa;
extern float currentTotalmAh;
extern int batteryPercent;
extern float estimatedBatteryLifeHours;

// MOTION
extern float accX, accY, accZ;
extern float gyroX, gyroY, gyroZ;
extern float accTotal;
extern float dynX;
extern float dynY;
extern float dynZ;
extern bool parachuteOpened;
extern bool positionChanged;
extern bool freeFallRisk;
extern bool excessiveRotation;
extern bool noMovement;

// AI / STATUS
extern String stressLevel;
extern String alertLevel;
extern String prediction;
extern int riskScore;

// SYSTEM
extern int cpuLoad;

// UI
extern int page;
extern int PAGE_COUNT;