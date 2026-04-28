#include <Arduino.h>
#include "system_state.h"

#define BTN_NEXT 2
#define BTN_PREV 3

static unsigned long lastButtonScan = 0;
static unsigned long lastAction = 0;
static unsigned long holdNextStart = 0;
static unsigned long holdPrevStart = 0;

void buttonManagerInit() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
}

void buttonManagerUpdate() {
  unsigned long now = millis();

  if (now - lastButtonScan < 40) return;
  lastButtonScan = now;

  bool nextPressed = digitalRead(BTN_NEXT) == LOW;
  bool prevPressed = digitalRead(BTN_PREV) == LOW;

  // ambele butoane = revine la pagina principala
  if (nextPressed && prevPressed) {
    if (now - lastAction > 500) {
      page = 0;
      lastAction = now;
    }
    return;
  }

  // NEXT: click + hold rapid
  if (nextPressed) {
    if (holdNextStart == 0) holdNextStart = now;

    unsigned long interval = (now - holdNextStart > 800) ? 170 : 330;

    if (now - lastAction > interval) {
      page++;
      if (page >= PAGE_COUNT) page = 0;
      lastAction = now;
    }
  } else {
    holdNextStart = 0;
  }

  // PREV: click + hold rapid
  if (prevPressed) {
    if (holdPrevStart == 0) holdPrevStart = now;

    unsigned long interval = (now - holdPrevStart > 800) ? 170 : 330;

    if (now - lastAction > interval) {
      page--;
      if (page < 0) page = PAGE_COUNT - 1;
      lastAction = now;
    }
  } else {
    holdPrevStart = 0;
  }
}