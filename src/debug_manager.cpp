#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include "debug_manager.h"

static unsigned long lastDebugPrint = 0;

void debugPrintResetReason() {
  esp_reset_reason_t reason = esp_reset_reason();

  Serial.println();
  Serial.println("===== RESET DEBUG =====");

  Serial.print("Code: ");
  Serial.println((int)reason);

  Serial.print("Reason: ");

  switch (reason) {
    case ESP_RST_POWERON:   Serial.println("POWERON"); break;
    case ESP_RST_EXT:       Serial.println("EXTERNAL"); break;
    case ESP_RST_SW:        Serial.println("SOFTWARE"); break;
    case ESP_RST_PANIC:     Serial.println("PANIC / CRASH"); break;
    case ESP_RST_INT_WDT:   Serial.println("INT WDT"); break;
    case ESP_RST_TASK_WDT:  Serial.println("TASK WDT"); break;
    case ESP_RST_WDT:       Serial.println("WDT"); break;
    case ESP_RST_DEEPSLEEP: Serial.println("DEEPSLEEP"); break;
    case ESP_RST_BROWNOUT:  Serial.println("BROWNOUT"); break;
    default:                Serial.println("UNKNOWN"); break;
  }

  Serial.println("========================");
}

void debugPrintCheckpoint(const char* message) {
  Serial.print("[CHECKPOINT] ");
  Serial.println(message);
}

void debugManagerInit() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("===== SYSTEM BOOT =====");

  debugPrintResetReason();

  Serial.println("System starting...");
}

void debugManagerUpdate() {
  if (millis() - lastDebugPrint < 3000) return;
  lastDebugPrint = millis();

  Serial.print("Alive | uptime=");
  Serial.print(millis() / 1000);
  Serial.print("s | heap=");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" | wifi=");
  Serial.println(WiFi.status());
}