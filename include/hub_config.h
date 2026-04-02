#ifndef HUB_CONFIG_H
#define HUB_CONFIG_H

// Device names
#define HUB_NAME "Mr Hub"
#define MOTION_NAME "Ms Motion"

// WiFi STA
#define HUB_WIFI_SSID "Net"
#define HUB_WIFI_PASSWORD "12345678"

// OLED
#define OLED_PAGE_COUNT 5

// Buttons
#define BTN_NEXT_PIN 2
#define BTN_PREV_PIN 3

// I2C pins
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Module presence
#define MODULE_WIFI_PRESENT true
#define MODULE_OLED_PRESENT true
#define MODULE_INA219_PRESENT true
#define MODULE_NTC_PRESENT true
#define MODULE_RTC_PRESENT false
#define MODULE_BMI160_PRESENT true
#define MODULE_MOTION_DEVICE_PRESENT false

// NTC PARAMETRES
#define NTC_PIN 0
#define NTC_B_COEF 3900
#define NTC_T_NOMINAL 21.0
#define NTC_R_NOMINAL 10000
#define NTC_R_FIXED 10000

// Battery
#define BATTERY_CAPACITY_MAH 2000.0f
#define BATTERY_VOLTAGE_MIN 3.0f
#define BATTERY_VOLTAGE_MAX 4.2f

#endif