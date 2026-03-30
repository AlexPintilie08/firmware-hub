#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

void wifiInit();
String wifiGetIP();
int wifiGetClientCount();
bool wifiIsConnected();
int wifiGetRSSI();

#endif