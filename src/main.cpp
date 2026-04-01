#include <Arduino.h>
#include <WebSocketsServer.h>
#include "wifi_manager.h"
#include "api_server.h"
#include "system_state.h"
#include "oled_manager.h"
#include "button_manager.h"
#include "sensor_manager.h"

static unsigned long sensorTimer = 0;
static unsigned long clientTimer = 0;
static unsigned long cpuTimer = 0;
static unsigned long busyAccum = 0;

WebSocketsServer webSocket(81);

// Temporizator dedicat pentru expedierea datelor WebSockets
static unsigned long wsTimer = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    stateInit();
    wifiInit();
    stateUpdateIp(wifiGetIP());

    oledInit();
    buttonsInit();
    sensorsInit();

    oledRequestRefresh();
    webSocket.begin();
    apiServerInit();

    addLog("System state initialized");
    addLog("WiFi initialized");
    addLog("OLED initialized");
    addLog("Sensors initialized");
    addLog("API server initialized");
    Serial.println("SYSTEM READY");
}

void loop() {
    unsigned long loopStart = micros();
    unsigned long nowMs = millis();

    webSocket.loop();
    apiServerHandle();
    buttonsUpdate();

    // Actualizare ultrarapidă pentru accelerometru și giroscop
    sensorsUpdate();

    if (nowMs - wsTimer >= 30) {
        // Preluăm variabilele externe din sensor_manager.h și creăm un JSON minimal
        String wsJson = "{\"roll\":" + String(roll, 2) + ",\"pitch\":" + String(pitch, 2) + "}";
        
        // Trimitem string-ul JSON către toți clienții conectați pe portul 81
        webSocket.broadcastTXT(wsJson);
        
        wsTimer = nowMs;
    }

    // Actualizare lentă (doar 1 dată pe secundă) pt NTC/INA219
    if (nowMs - sensorTimer >= 1000) {
        sensorsUpdateSlow();
        oledRequestRefresh();
        sensorTimer = nowMs;
    }

    if (nowMs - clientTimer >= 2000) {
        stateUpdateClients(wifiGetClientCount());
        stateUpdateIp(wifiGetIP());
        oledRequestRefresh();
        clientTimer = nowMs;
    }

    unsigned long loopEnd = micros();
    busyAccum += (loopEnd - loopStart);

    if (nowMs - cpuTimer >= 1000) {
        int cpuLoad = busyAccum / 10000UL;
        if (cpuLoad > 100) cpuLoad = 100;
        sensorsSetCpuLoad(cpuLoad);
        oledRequestRefresh();
        busyAccum = 0;
        cpuTimer = nowMs;
    }

    if (oledNeedsRefresh()) {
        oledUpdate();
    }

    delay(10);
}