#include <Arduino.h>
#include "system_state.h"
#include "connection_manager.h"

#define BTN_NEXT 2
#define BTN_PREV 3

#define BUTTON_SCAN_MS 35
#define BOTH_HOLD_MS 3000
#define PAGE_REPEAT_START_MS 800
#define PAGE_REPEAT_FAST_MS 150
#define PAGE_REPEAT_SLOW_MS 320
#define MODE_COOLDOWN_MS 1500

static unsigned long lastScan = 0;
static unsigned long lastPageAction = 0;
static unsigned long lastModeAction = 0;

static unsigned long holdNextStart = 0;
static unsigned long holdPrevStart = 0;
static unsigned long bothHoldStart = 0;

static bool modeToggleDone = false;

void buttonManagerInit() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  lastScan = millis();
  lastPageAction = millis();
  lastModeAction = 0;

  holdNextStart = 0;
  holdPrevStart = 0;
  bothHoldStart = 0;

  modeToggleDone = false;

  Serial.println("Buttons ready");
}

void buttonManagerUpdate() {
  unsigned long now = millis();

  if (now - lastScan < BUTTON_SCAN_MS) return;
  lastScan = now;

  bool nextPressed = digitalRead(BTN_NEXT) == LOW;
  bool prevPressed = digitalRead(BTN_PREV) == LOW;

  /*
    Ambele butoane ținute 3 secunde:
    - comută BLE/WIFI
    - nu schimbă pagina
    - declanșează o singură dată până eliberezi butoanele
  */
  if (nextPressed && prevPressed) {
    holdNextStart = 0;
    holdPrevStart = 0;

    if (bothHoldStart == 0) {
      bothHoldStart = now;
      modeToggleDone = false;
      Serial.println("Button combo hold started");
    }

    bool holdReached = now - bothHoldStart >= BOTH_HOLD_MS;
    bool cooldownOk = now - lastModeAction >= MODE_COOLDOWN_MS;

    if (!modeToggleDone && holdReached && cooldownOk) {
      connectionManagerToggleMode();

      page = 0;
      lastModeAction = now;
      modeToggleDone = true;

      Serial.print("Connection mode switched to: ");
      Serial.println(connectionManagerModeName());
    }

    return;
  }

  bothHoldStart = 0;
  modeToggleDone = false;

  // NEXT page
  if (nextPressed) {
    if (holdNextStart == 0) {
      holdNextStart = now;
    }

    unsigned long heldMs = now - holdNextStart;
    unsigned long interval =
      heldMs > PAGE_REPEAT_START_MS ? PAGE_REPEAT_FAST_MS : PAGE_REPEAT_SLOW_MS;

    if (now - lastPageAction >= interval) {
      page++;
      if (page >= PAGE_COUNT) page = 0;

      lastPageAction = now;
    }
  } else {
    holdNextStart = 0;
  }

  // PREV page
  if (prevPressed) {
    if (holdPrevStart == 0) {
      holdPrevStart = now;
    }

    unsigned long heldMs = now - holdPrevStart;
    unsigned long interval =
      heldMs > PAGE_REPEAT_START_MS ? PAGE_REPEAT_FAST_MS : PAGE_REPEAT_SLOW_MS;

    if (now - lastPageAction >= interval) {
      page--;
      if (page < 0) page = PAGE_COUNT - 1;

      lastPageAction = now;
    }
  } else {
    holdPrevStart = 0;
  }
}