#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "system_state.h"

#define NTC_PIN 0
#define BMI160_ADDR_1 0x68
#define BMI160_ADDR_2 0x69

Adafruit_INA219 ina219;
MAX30105 max30102;

static bool inaOk = false;
static bool maxOk = false;
static bool bmiOk = false;
static uint8_t bmiAddr = BMI160_ADDR_1;

static unsigned long lastSlowSensorRead = 0;
static unsigned long lastMotionRead = 0;
static unsigned long lastMaxRead = 0;
static unsigned long lastMovementTime = 0;
static unsigned long lastPowerSample = 0;

static float filteredTemp = 36.5;
static float filteredVoltage = 3.7;
static float filteredCurrent = 0;

const byte RATE_SIZE = 2;
static byte rates[RATE_SIZE];
static byte rateSpot = 0;
static long lastBeat = 0;

static float gX = 0;
static float gY = 0;
static float gZ = 1;

static float accOffsetX = 0;
static float accOffsetY = 0;
static float accOffsetZ = 0;

static float gyroOffsetX = 0;
static float gyroOffsetY = 0;
static float gyroOffsetZ = 0;

static float irDC = 0;
static float redDC = 0;
static float irAC = 0;
static float redAC = 0;
static float spo2Filtered = 97;

static void writeReg8(uint8_t addr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

static bool readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) return false;

  uint8_t received = Wire.requestFrom(addr, len);
  if (received < len) return false;

  for (uint8_t i = 0; i < len; i++) {
    buffer[i] = Wire.read();
  }

  return true;
}

static float readNTC() {
  int adc = analogRead(NTC_PIN);

  if (adc <= 0 || adc >= 4095) return filteredTemp;

  float r = 10000.0 * ((4095.0 / adc) - 1.0);

  float t = log(r / 10000.0);
  t /= 3950.0;
  t += 1.0 / (25.0 + 273.15);
  t = 1.0 / t;
  t -= 273.15;

  return t;
}

static bool initBMI160() {
  uint8_t chipId = 0;

  bmiAddr = BMI160_ADDR_1;

  if (!readBytes(bmiAddr, 0x00, &chipId, 1) || chipId != 0xD1) {
    bmiAddr = BMI160_ADDR_2;

    if (!readBytes(bmiAddr, 0x00, &chipId, 1) || chipId != 0xD1) {
      Serial.println("BMI160 FAIL");
      return false;
    }
  }

  Serial.print("BMI160 OK addr: 0x");
  Serial.println(bmiAddr, HEX);

  writeReg8(bmiAddr, 0x7E, 0x11);
  delay(80);

  writeReg8(bmiAddr, 0x7E, 0x15);
  delay(120);

  // ACC config: 100Hz, normal mode
  writeReg8(bmiAddr, 0x40, 0x28);
  // ACC range: +/-2g
  writeReg8(bmiAddr, 0x41, 0x03);

  // GYRO config: 100Hz, normal mode
  writeReg8(bmiAddr, 0x42, 0x28);
  // GYRO range: +/-2000 dps
  writeReg8(bmiAddr, 0x43, 0x00);

  return true;
}

static bool readBMI160Raw(float& ax, float& ay, float& az, float& gx, float& gy, float& gz) {
  if (!bmiOk) return false;

  uint8_t data[12];

  if (!readBytes(bmiAddr, 0x0C, data, 12)) return false;

  int16_t gxRaw = (int16_t)((data[1] << 8) | data[0]);
  int16_t gyRaw = (int16_t)((data[3] << 8) | data[2]);
  int16_t gzRaw = (int16_t)((data[5] << 8) | data[4]);

  int16_t axRaw = (int16_t)((data[7] << 8) | data[6]);
  int16_t ayRaw = (int16_t)((data[9] << 8) | data[8]);
  int16_t azRaw = (int16_t)((data[11] << 8) | data[10]);

  gx = gxRaw / 16.4;
  gy = gyRaw / 16.4;
  gz = gzRaw / 16.4;

  ax = axRaw / 16384.0;
  ay = ayRaw / 16384.0;
  az = azRaw / 16384.0;

  return true;
}

static void calibrateBMI160() {
  if (!bmiOk) return;

  Serial.println("BMI160 calibration start - keep board still");

  float sumAx = 0;
  float sumAy = 0;
  float sumAz = 0;
  float sumGx = 0;
  float sumGy = 0;
  float sumGz = 0;

  const int samples = 120;
  int good = 0;

  for (int i = 0; i < samples; i++) {
    float ax, ay, az, gx, gy, gz;

    if (readBMI160Raw(ax, ay, az, gx, gy, gz)) {
      sumAx += ax;
      sumAy += ay;
      sumAz += az;
      sumGx += gx;
      sumGy += gy;
      sumGz += gz;
      good++;
    }

    delay(8);
  }

  if (good <= 0) {
    Serial.println("BMI160 calibration failed");
    return;
  }

  accOffsetX = sumAx / good;
  accOffsetY = sumAy / good;
  accOffsetZ = (sumAz / good) - 1.0;

  gyroOffsetX = sumGx / good;
  gyroOffsetY = sumGy / good;
  gyroOffsetZ = sumGz / good;

  gX = 0;
  gY = 0;
  gZ = 1;

  Serial.println("BMI160 calibration done");
}

static void readBMI160() {
  if (!bmiOk) return;

  float axRaw, ayRaw, azRaw;
  float gxRaw, gyRaw, gzRaw;

  if (!readBMI160Raw(axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw)) return;

  gyroX = gxRaw - gyroOffsetX;
  gyroY = gyRaw - gyroOffsetY;
  gyroZ = gzRaw - gyroOffsetZ;

  accX = axRaw - accOffsetX;
  accY = ayRaw - accOffsetY;
  accZ = azRaw - accOffsetZ;

  // low-pass gravity estimate
  gX = 0.92 * gX + 0.08 * accX;
  gY = 0.92 * gY + 0.08 * accY;
  gZ = 0.92 * gZ + 0.08 * accZ;

  dynX = accX - gX;
  dynY = accY - gY;
  dynZ = accZ - gZ;

  float rawTotal = sqrt(accX * accX + accY * accY + accZ * accZ);
  accTotal = sqrt(dynX * dynX + dynY * dynY + dynZ * dynZ);

  positionChanged = accTotal > 0.08 || abs(gyroX) > 35 || abs(gyroY) > 35 || abs(gyroZ) > 35;

  if (positionChanged) {
    lastMovementTime = millis();
  }

  freeFallRisk = rawTotal < 0.35;

  excessiveRotation =
    abs(gyroX) > 450 ||
    abs(gyroY) > 450 ||
    abs(gyroZ) > 450;

  noMovement = millis() - lastMovementTime > 6000;

  // demo logic: opening shock usually appears as strong acceleration spike
  parachuteOpened = accTotal > 1.2;
}

static void resetHeartBuffers() {
  bpm = 0;
  bpmAvg = 0;
  spo2 = 0;
  rateSpot = 0;
  lastBeat = 0;

  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }
}

static void readMAX30102() {
  if (!maxOk) return;

  if (millis() - lastMaxRead < 8) return;
  lastMaxRead = millis();

  long irRaw = max30102.getIR();
  long redRaw = max30102.getRed();

  irValue = irRaw;
  redValue = redRaw;

  if (irRaw < 8000 || redRaw < 2000) {
    resetHeartBuffers();
    return;
  }

  if (irRaw > 260000 || redRaw > 260000) {
    return;
  }

  if (checkForBeat(irRaw)) {
    long nowTime = millis();

    if (lastBeat > 0) {
      long delta = nowTime - lastBeat;

      if (delta >= 300 && delta <= 2000) {
        float newBpm = 60000.0 / delta;

        if (newBpm >= 40 && newBpm <= 180) {
          bpm = newBpm;

          rates[rateSpot++] = (byte)newBpm;
          rateSpot %= RATE_SIZE;

          int sum = 0;
          int count = 0;

          for (byte i = 0; i < RATE_SIZE; i++) {
            if (rates[i] > 0) {
              sum += rates[i];
              count++;
            }
          }

          if (count > 0) {
            float avg = (float)sum / count;
            bpmAvg = (bpmAvg == 0) ? avg : (0.75 * avg + 0.25 * bpmAvg);
          }
        }
      }
    }

    lastBeat = nowTime;
  }

  if (irDC == 0) irDC = irRaw;
  if (redDC == 0) redDC = redRaw;

  irDC = 0.95 * irDC + 0.05 * irRaw;
  redDC = 0.95 * redDC + 0.05 * redRaw;

  float irDiff = abs(irRaw - irDC);
  float redDiff = abs(redRaw - redDC);

  irAC = 0.85 * irAC + 0.15 * irDiff;
  redAC = 0.85 * redAC + 0.15 * redDiff;

  if (irDC > 0 && redDC > 0 && irAC > 5 && redAC > 5) {
    float r = (redAC / redDC) / (irAC / irDC);
    int estimated = constrain(104 - (int)(17 * r), 90, 99);

    if (estimated >= 85 && estimated <= 100) {
      spo2Filtered = 0.25 * estimated + 0.75 * spo2Filtered;
      spo2 = round(spo2Filtered);
    }
  }
}

static void updateRisk() {
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
  if (parachuteOpened) riskScore += 5;
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

  if (bpmAvg > 140 || (spo2 > 0 && spo2 < 92) || bodyTempC > 38.2) {
    stressLevel = "RIDICAT";
  } else if (bpmAvg > 105 || bodyTempC > 37.4) {
    stressLevel = "MEDIU";
  } else {
    stressLevel = "NORMAL";
  }
}

void sensorManagerInit() {
  analogReadResolution(12);

  inaOk = ina219.begin();

  if (inaOk) Serial.println("INA219 OK");
  else Serial.println("INA219 FAIL - disabled");

  maxOk = max30102.begin(Wire, I2C_SPEED_FAST);

  if (maxOk) {
    max30102.setup(
      0x24,
      1,
      2,
      400,
      411,
      4096
    );

    max30102.setPulseAmplitudeRed(0x24);
    max30102.setPulseAmplitudeIR(0x24);
    max30102.setPulseAmplitudeGreen(0);

    Serial.println("MAX30102 OK");
  } else {
    Serial.println("MAX30102 FAIL");
  }

  bmiOk = initBMI160();

  if (bmiOk) {
    calibrateBMI160();
  }

  lastMovementTime = millis();
  lastPowerSample = millis();
}

void sensorManagerUpdate() {
  readMAX30102();

  // motion rapid: 50 Hz
  if (millis() - lastMotionRead >= 20) {
    lastMotionRead = millis();
    readBMI160();
    updateRisk();
  }

  // senzori lenți: 4 Hz
  if (millis() - lastSlowSensorRead < 250) return;
  lastSlowSensorRead = millis();

  float ntc = readNTC();

  filteredTemp = 0.1 * ntc + 0.9 * filteredTemp;
  bodyTempC = filteredTemp;

  if (inaOk) {
    float v = ina219.getBusVoltage_V();
    float i = ina219.getCurrent_mA();

    if (!isnan(v) && !isnan(i) && v > 0.5 && v < 15 && i > -5000 && i < 5000) {
      filteredVoltage = 0.08 * v + 0.92 * filteredVoltage;
      filteredCurrent = 0.08 * i + 0.92 * filteredCurrent;
    }
  }

  voltage = filteredVoltage;
  currentMa = filteredCurrent;

  batteryPercent = constrain(map(voltage * 100, 330, 420, 0, 100), 0, 100);

  unsigned long now = millis();
  float hoursDelta = (now - lastPowerSample) / 3600000.0;
  lastPowerSample = now;

  if (currentMa > 0) {
    currentTotalmAh += currentMa * hoursDelta;
    estimatedBatteryLifeHours = 2000.0 / currentMa;
  } else {
    estimatedBatteryLifeHours = 0;
  }

  updateRisk();
}