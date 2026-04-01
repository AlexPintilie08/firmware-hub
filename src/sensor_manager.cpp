#include "sensor_manager.h"
#include "hub_config.h"
#include "system_state.h"
#include <Adafruit_INA219.h>
#include <BMI160Gen.h>
#include <Wire.h>

// ina
Adafruit_INA219 ina219;
static float filteredTemp = 25.0;
const float TEMP_ALFA = 0.05;

// Variabile pentru BMI160
BMI160GenClass bmi160;
const int8_t i2c_addr = 0x68;
float roll = 0, pitch = 0;
unsigned long lastMicros = 0;
static float rollOffset = 0;
static float pitchOffset = 0;
const float alpha = 0.998f;

void sensorsInit() {
    // 1. Init INA219 (existent)
    if (MODULE_INA219_PRESENT) {
        ina219.begin();
    }

    // 2. Init BMI160
    if (BMI160.begin(BMI160GenClass::I2C_MODE, 0x68)) {
        BMI160.setAccelerometerRange(2);
        BMI160.setGyroRange(250);

        // Calibrare precisă (media pe 100 de citiri)
        float sR = 0, sP = 0;
        for(int i = 0; i < 100; i++) {
            int ax, ay, az;
            BMI160.readAccelerometer(ax, ay, az);
            sR += atan2(ay, az) * 180.0 / M_PI;
            sP += atan2(-ax, sqrt((float)ay * ay + (float)az * az)) * 180.0 / M_PI;
            delay(2);
        }
        rollOffset = sR / 100.0;
        pitchOffset = sP / 100.0;
        lastMicros = micros();
        getBmi160State().status = "online";
    }
}

void sensorsUpdateSlow() {
    TelemetryState& t = getTelemetryState();
    ComponentState& ntc = getNtcState();
    ComponentState& ina = getIna219State();

    // --- LOGICA NTC ---
    if (MODULE_NTC_PRESENT) {
        int raw = analogRead(NTC_PIN);
        if (raw > 0 && raw < 4095) {
            float resistance = NTC_R_FIXED * (4095.0 / (float)raw - 1.0);
            float steinhart = log(resistance / NTC_R_NOMINAL) / NTC_B_COEF;
            steinhart += 1.0 / (NTC_T_NOMINAL + 273.15);
            float currentT = (1.0 / steinhart) - 273.15;
            filteredTemp = (TEMP_ALFA * currentT) + (1.0 - TEMP_ALFA) * filteredTemp;
            t.temperatureC = filteredTemp;
            ntc.status = "online";
        }
    }

    // --- LOGICA INA219 ---
    if (MODULE_INA219_PRESENT) {
        t.voltageV = ina219.getBusVoltage_V();
        t.currentmA = ina219.getCurrent_mA();
        
        float pct = ((t.voltageV - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN)) * 100.0f;
        t.batteryPercent = (int)constrain(pct, 0, 100);
        ina.status = "online";
    }
}

void sensorsUpdate() {
    //---- LOGICA BMI160 ---
    if (getBmi160State().status != "online") return;

    int ax, ay, az, gx, gy, gz;
    unsigned long now = micros();
    float dt = (now - lastMicros) / 1000000.0f;
    lastMicros = now;

    BMI160.readAccelerometer(ax, ay, az);
    BMI160.readGyro(gx, gy, gz);

    float accRoll = (atan2(ay, az) * 180.0 / M_PI) - rollOffset;
    float accPitch = (atan2(-ax, sqrt((float)ay * ay + (float)az * az)) * 180.0 / M_PI) - pitchOffset;

    float gyroRollRate = gx / 131.0f;
    float gyroPitchRate = gy / 131.0f;

    roll = alpha * (roll + gyroRollRate * dt) + (1.0f - alpha) * accRoll;
    pitch = alpha * (pitch + gyroPitchRate * dt) + (1.0f - alpha) * accPitch;

    // DEADZONE pentru stabilitate pe birou
    if (abs(roll) < 0.25) roll = 0;
    if (abs(pitch) < 0.25) pitch = 0;
}

void sensorsSetCpuLoad(int cpuLoadPercent) {
    getTelemetryState().cpuLoadPercent = cpuLoadPercent;
}
