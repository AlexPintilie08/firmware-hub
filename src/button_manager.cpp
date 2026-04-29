#include <Arduino.h>
#include "system_state.h"
#include "connection_manager.h"

#define BTN_NEXT 2
#define BTN_PREV 3

#define BUTTON_SCAN_MS 35
#define BOTH_HOLD_MS 3000
#define MODE_COOLDOWN_MS 1500

static unsigned long lastScan = 0;
static unsigned long lastAction = 0;
static unsigned long holdNextStart = 0;
static unsigned long holdPrevStart = 0;
static unsigned long bothHoldStart = 0;

static bool modeToggleDone = false;

void buttonManagerInit() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
}

void buttonManagerUpdate() {
  unsigned long now = millis();

  if (now - lastScan < BUTTON_SCAN_MS) return;
  lastScan = now;

  bool nextPressed = digitalRead(BTN_NEXT) == LOW;
  bool prevPressed = digitalRead(BTN_PREV) == LOW;

  // Ambele butoane ținute 3 secunde = comută BLE/WIFI
  if (nextPressed && prevPressed) {
    if (bothHoldStart == 0) {
      bothHoldStart = now;
      modeToggleDone = false;
    }

    if (
      !modeToggleDone &&
      now - bothHoldStart >= BOTH_HOLD_MS &&
      now - lastAction >= MODE_COOLDOWN_MS
    ) {
      connectionManagerToggleMode();

      page = 0;
      lastAction = now;
      modeToggleDone = true;

      Serial.print("Button combo switched mode to: ");
      Serial.println(connectionManagerModeName());
    }

    return;
  }

  bothHoldStart = 0;
  modeToggleDone = false;

  // NEXT
  if (nextPressed) {
    if (holdNextStart == 0) holdNextStart = now;

    unsigned long interval = (now - holdNextStart > 800) ? 150 : 320;

    if (now - lastAction > interval) {
      page++;
      if (page >= PAGE_COUNT) page = 0;
      lastAction = now;
    }
  } else {
    holdNextStart = 0;
  }

  // PREV
  if (prevPressed) {
    if (holdPrevStart == 0) holdPrevStart = now;

    unsigned long interval = (now - holdPrevStart > 800) ? 150 : 320;

    if (now - lastAction > interval) {
      page--;
      if (page < 0) page = PAGE_COUNT - 1;
      lastAction = now;
    }
  } else {
    holdPrevStart = 0;
  }
}