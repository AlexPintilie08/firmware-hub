#ifndef OLED_MANAGER_H
#define OLED_MANAGER_H

#include <Arduino.h>

void oledInit();
void oledUpdate();
void oledRequestRefresh();
bool oledNeedsRefresh();
void oledClearRefreshFlag();

#endif