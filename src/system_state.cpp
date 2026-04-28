#include "system_state.h"

// HEALTH
float bodyTempC = 36.5;
float bpm = 0;
float bpmAvg = 0;
int spo2 = 0;
long irValue = 0;
long redValue = 0;

// POWER
float voltage = 3.7;
float currentMa = 0;
float currentTotalmAh = 0;
int batteryPercent = 0;
float estimatedBatteryLifeHours = 0;

// MOTION
float accX = 0;
float accY = 0;
float accZ = 0;

float gyroX = 0;
float gyroY = 0;
float gyroZ = 0;
float dynX = 0;
float dynY = 0;
float dynZ = 0;
float accTotal = 0;

bool parachuteOpened = false;
bool positionChanged = false;
bool freeFallRisk = false;
bool excessiveRotation = false;
bool noMovement = false;

// AI / STATUS
String stressLevel = "NORMAL";
String alertLevel = "SAFE";
String prediction = "normal";
int riskScore = 0;

// SYSTEM
int cpuLoad = 0;

// UI
int page = 0;
int PAGE_COUNT = 10;