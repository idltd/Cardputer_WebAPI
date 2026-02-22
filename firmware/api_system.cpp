#include "config.h"
#include "api_server.h"
#include "api_system.h"
#include <M5Cardputer.h>
#include <WiFi.h>

void setupSystemApi() {
    apiServer.http().on("/api/system/info", HTTP_GET, [](AsyncWebServerRequest* req) {
        ApiServer::restRequestCount++;
        JsonDocument doc;

        // Chip info
        doc["chip"]["model"] = ESP.getChipModel();
        doc["chip"]["revision"] = ESP.getChipRevision();
        doc["chip"]["cores"] = ESP.getChipCores();
        doc["chip"]["freq_mhz"] = ESP.getCpuFreqMHz();

        // Memory
        doc["heap"]["free"] = ESP.getFreeHeap();
        doc["heap"]["total"] = ESP.getHeapSize();
        doc["heap"]["min_free"] = ESP.getMinFreeHeap();

        // PSRAM
        doc["psram"]["free"] = ESP.getFreePsram();
        doc["psram"]["total"] = ESP.getPsramSize();

        // System
        doc["uptime_ms"] = millis();
        doc["sdk_version"] = ESP.getSdkVersion();

        // Flash
        doc["flash"]["size"] = ESP.getFlashChipSize();
        doc["flash"]["speed"] = ESP.getFlashChipSpeed();

        // WiFi
        doc["wifi"]["ssid"] = WIFI_AP_SSID;
        doc["wifi"]["ip"] = WiFi.softAPIP().toString();
        doc["wifi"]["clients"] = WiFi.softAPgetStationNum();

        // WebSocket clients
        doc["ws_clients"] = apiServer.wsClientCount();

        // Traffic counters
        doc["traffic"]["rest_requests"] = ApiServer::restRequestCount;
        doc["traffic"]["ws_messages"] = ApiServer::wsMessageCount;

        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    Serial.println("[API] System endpoints registered");
}
