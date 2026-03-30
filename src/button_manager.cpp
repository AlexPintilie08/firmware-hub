#include "button_manager.h"
#include "system_state.h"
#include "oled_manager.h"
#include "hub_config.h"

#define BTN_NEXT BTN_NEXT_PIN
#define BTN_PREV BTN_PREV_PIN

static bool lastNextState = HIGH;
static bool lastPrevState = HIGH;

static unsigned long nextHoldStart = 0;
static unsigned long prevHoldStart = 0;
static unsigned long lastNextRepeat = 0;
static unsigned long lastPrevRepeat = 0;

static const unsigned long debounceMs = 40;
static const unsigned long holdDelayMs = 700;   // mai lent la hold
static const unsigned long repeatMs = 350;      // mai lent la auto-scroll

static void updatePageTitle(OledState& oled) {
  switch (oled.currentPage) {
    case 0: oled.pageTitle = "HOME"; break;
    case 1: oled.pageTitle = "TEMP"; break;
    case 2: oled.pageTitle = "POWER"; break;
    case 3: oled.pageTitle = "NET"; break;
    case 4: oled.pageTitle = "MOD"; break;
    default: oled.pageTitle = "UNK"; break;
  }
}

static void goNextPage() {
  OledState& oled = getOledState();

  oled.currentPage = (oled.currentPage + 1) % OLED_PAGE_COUNT;
  updatePageTitle(oled);
  oled.lastActionSource = "button_next";

  addLog("OLED page changed: " + oled.pageTitle + " via NEXT");
  oledRequestRefresh();
}

static void goPrevPage() {
  OledState& oled = getOledState();

  oled.currentPage--;
  if (oled.currentPage < 0) {
    oled.currentPage = OLED_PAGE_COUNT - 1;
  }

  updatePageTitle(oled);
  oled.lastActionSource = "button_prev";

  addLog("OLED page changed: " + oled.pageTitle + " via PREV");
  oledRequestRefresh();
}

void buttonsInit() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
}

void buttonsUpdate() {
  unsigned long now = millis();

  bool nextState = digitalRead(BTN_NEXT);
  bool prevState = digitalRead(BTN_PREV);

  // NEXT - apăsare inițială
  if (lastNextState == HIGH && nextState == LOW) {
    goNextPage();
    nextHoldStart = now;
    lastNextRepeat = now;
    delay(debounceMs);
  }

  // NEXT - ținut apăsat
  if (nextState == LOW) {
    if (now - nextHoldStart >= holdDelayMs && now - lastNextRepeat >= repeatMs) {
      goNextPage();
      lastNextRepeat = now;
    }
  } else {
    nextHoldStart = 0;
    lastNextRepeat = 0;
  }

  // PREV - apăsare inițială
  if (lastPrevState == HIGH && prevState == LOW) {
    goPrevPage();
    prevHoldStart = now;
    lastPrevRepeat = now;
    delay(debounceMs);
  }

  // PREV - ținut apăsat
  if (prevState == LOW) {
    if (now - prevHoldStart >= holdDelayMs && now - lastPrevRepeat >= repeatMs) {
      goPrevPage();
      lastPrevRepeat = now;
    }
  } else {
    prevHoldStart = 0;
    lastPrevRepeat = 0;
  }

  lastNextState = nextState;
  lastPrevState = prevState;
}