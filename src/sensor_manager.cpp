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

static unsigned long lastSensorRead = 0;
static unsigned long lastMaxRead = 0;
static unsigned long lastMovementTime = 0;
static unsigned long lastPowerSample = 0;

static float filteredTemp = 36.5;
static float filteredVoltage = 3.7;
static float filteredCurrent = 0;

// filtru gravitație pentru BMI160
static float gX = 0;
static float gY = 0;
static float gZ = 1;

// ---------- I2C ----------
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

  Wire.requestFrom(addr, len);

  if (Wire.available() < len) return false;

  for (uint8_t i = 0; i < len; i++) {
    buffer[i] = Wire.read();
  }

  return true;
}

// ---------- NTC ----------
// Schema: 3.3V -> NTC -> GPIO0/A0 -> rezistor 10k -> GND
static float readNTC() {
  int adc = analogRead(NTC_PIN);

  if (adc <= 0 || adc >= 4095) {
    return filteredTemp;
  }

  const float seriesR = 10000.0;
  const float nominalR = 10000.0;
  const float nominalT = 25.0;
  const float beta = 3950.0;

  float resistance = seriesR * ((4095.0 / adc) - 1.0);

  float steinhart = resistance / nominalR;
  steinhart = log(steinhart);
  steinhart /= beta;
  steinhart += 1.0 / (nominalT + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;

  return steinhart;
}

// ---------- BMI160 ----------
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

  writeReg8(bmiAddr, 0x7E, 0x11); // accel normal
  delay(80);

  writeReg8(bmiAddr, 0x7E, 0x15); // gyro normal
  delay(120);

  writeReg8(bmiAddr, 0x40, 0x28); // accel ODR
  writeReg8(bmiAddr, 0x41, 0x03); // accel range +-2g

  writeReg8(bmiAddr, 0x42, 0x28); // gyro ODR
  writeReg8(bmiAddr, 0x43, 0x00); // gyro range +-2000 dps

  delay(50);

  return true;
}

static void readBMI160() {
  if (!bmiOk) return;

  uint8_t data[12];

  if (!readBytes(bmiAddr, 0x0C, data, 12)) {
    return;
  }

  int16_t gx = (int16_t)((data[1] << 8) | data[0]);
  int16_t gy = (int16_t)((data[3] << 8) | data[2]);
  int16_t gz = (int16_t)((data[5] << 8) | data[4]);

  int16_t ax = (int16_t)((data[7] << 8) | data[6]);
  int16_t ay = (int16_t)((data[9] << 8) | data[8]);
  int16_t az = (int16_t)((data[11] << 8) | data[10]);

  gyroX = gx / 16.4;
  gyroY = gy / 16.4;
  gyroZ = gz / 16.4;

  accX = ax / 16384.0;
  accY = ay / 16384.0;
  accZ = az / 16384.0;

  // eliminare gravitație prin filtru low-pass
  gX = 0.98 * gX + 0.02 * accX;
  gY = 0.98 * gY + 0.02 * accY;
  gZ = 0.98 * gZ + 0.02 * accZ;

  dynX = accX - gX;
  dynY = accY - gY;
  dynZ = accZ - gZ;

  // accTotal = accelerație dinamică, fără gravitație
  accTotal = sqrt(dynX * dynX + dynY * dynY + dynZ * dynZ);

  positionChanged = accTotal > 0.18;

  if (positionChanged) {
    lastMovementTime = millis();
  }

  float rawTotal = sqrt(accX * accX + accY * accY + accZ * accZ);

  freeFallRisk = rawTotal < 0.35;

  excessiveRotation =
    abs(gyroX) > 450 ||
    abs(gyroY) > 450 ||
    abs(gyroZ) > 450;

  noMovement = millis() - lastMovementTime > 6000;

  parachuteOpened = accTotal > 1.2;
}

// ---------- MAX30102 ----------
static void readMAX30102() {
  if (!maxOk) return;

  if (millis() - lastMaxRead < 10) return;
  lastMaxRead = millis();

  long irRaw = max30102.getIR();
  long redRaw = max30102.getRed();

  irValue = irRaw;
  redValue = redRaw;

  static unsigned long lastBeat = 0;
  static byte validBeats = 0;
  static float spo2Filtered = 97;

  // fără deget / semnal prea slab
  if (irRaw < 25000) {
    bpm = 0;
    bpmAvg = 0;
    spo2 = 0;
    validBeats = 0;
    lastBeat = 0;
    return;
  }

  // puls real din variațiile IR
  if (checkForBeat(irRaw)) {
    unsigned long now = millis();

    if (lastBeat > 0) {
      unsigned long delta = now - lastBeat;

      if (delta >= 300 && delta <= 2000) {
        float newBpm = 60000.0 / delta;

        if (newBpm >= 40 && newBpm <= 180) {
          bpm = newBpm;
          validBeats++;

          if (validBeats == 1 || bpmAvg == 0) {
            bpmAvg = newBpm;
          } else {
            bpmAvg = 0.45 * newBpm + 0.55 * bpmAvg;
          }
        }
      }
    }

    lastBeat = now;
  }

  // SpO2 estimat + filtrat pentru demo stabil
  if (redRaw > 1000 && irRaw > 25000) {
    float ratio = (float)redRaw / (float)irRaw;

    int estimatedSpo2 = constrain(110 - (int)(25 * ratio), 70, 100);

    spo2Filtered = 0.12 * estimatedSpo2 + 0.88 * spo2Filtered;
    spo2 = round(spo2Filtered);
  }
}

// ---------- AI / RISK ----------
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

  if (bpmAvg > 140 || spo2 < 92 || bodyTempC > 38.2) {
    stressLevel = "RIDICAT";
  } else if (bpmAvg > 105 || bodyTempC > 37.4) {
    stressLevel = "MEDIU";
  } else {
    stressLevel = "NORMAL";
  }
}

// ---------- PUBLIC ----------
void sensorManagerInit() {
  analogReadResolution(12);

  inaOk = ina219.begin();

  if (inaOk) {
    Serial.println("INA219 OK");
  } else {
    Serial.println("INA219 FAIL - disabled");
  }

  maxOk = max30102.begin(Wire, I2C_SPEED_STANDARD);

  if (maxOk) {
    max30102.setup(
      0x3F, // LED brightness
      4,    // sample average
      2,    // Red + IR
      100,  // sample rate
      411,  // pulse width
      4096  // ADC range
    );

    max30102.setPulseAmplitudeRed(0x5F);
    max30102.setPulseAmplitudeIR(0x5F);
    max30102.setPulseAmplitudeGreen(0);

    Serial.println("MAX30102 OK");
  } else {
    Serial.println("MAX30102 FAIL");
  }

  bmiOk = initBMI160();

  lastMovementTime = millis();
  lastPowerSample = millis();
}

void sensorManagerUpdate() {
  readMAX30102();

  if (millis() - lastSensorRead < 250) return;
  lastSensorRead = millis();

  float ntc = readNTC();

  filteredTemp = 0.08 * ntc + 0.92 * filteredTemp;
  bodyTempC = filteredTemp;

  if (inaOk) {
    filteredVoltage = 0.06 * ina219.getBusVoltage_V() + 0.94 * filteredVoltage;
    filteredCurrent = 0.06 * ina219.getCurrent_mA() + 0.94 * filteredCurrent;
  } else {
    filteredVoltage = 3.7;
    filteredCurrent = 0;
  }

  voltage = filteredVoltage;
  currentMa = filteredCurrent;

  batteryPercent = constrain(
    map(voltage * 100, 330, 420, 0, 100),
    0,
    100
  );

  unsigned long now = millis();
  float hoursDelta = (now - lastPowerSample) / 3600000.0;
  lastPowerSample = now;

  if (currentMa > 0) {
    currentTotalmAh += currentMa * hoursDelta;
    estimatedBatteryLifeHours = 2000.0 / currentMa;
  } else {
    estimatedBatteryLifeHours = 0;
  }

  readBMI160();
  updateRisk();
}