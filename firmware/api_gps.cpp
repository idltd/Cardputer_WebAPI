#include "config.h"
#include "api_server.h"
#include "api_gps.h"
#include <TinyGPS++.h>

TinyGPSPlus gps;
static HardwareSerial gpsSerial(GPS_UART_NUM);
static AsyncWebSocket* wsGps = nullptr;
static unsigned long lastGpsBroadcast = 0;
static unsigned long gpsRateMs = 1000;

String gpsToJson() {
    JsonDocument doc;
    doc["valid"] = gps.location.isValid();
    if (gps.location.isValid()) {
        doc["lat"] = gps.location.lat();
        doc["lon"] = gps.location.lng();
    }
    if (gps.altitude.isValid()) {
        doc["alt_m"] = gps.altitude.meters();
    }
    if (gps.satellites.isValid()) {
        doc["satellites"] = gps.satellites.value();
    }
    if (gps.hdop.isValid()) {
        doc["hdop"] = gps.hdop.hdop();
    }
    if (gps.speed.isValid()) {
        doc["speed_kmh"] = gps.speed.kmph();
    }
    if (gps.course.isValid()) {
        doc["course_deg"] = gps.course.deg();
    }
    if (gps.date.isValid() && gps.time.isValid()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
                 gps.date.year(), gps.date.month(), gps.date.day(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
        doc["utc"] = buf;
    }
    doc["chars_processed"] = gps.charsProcessed();
    doc["sentences_ok"] = gps.sentencesWithFix();

    String json;
    serializeJson(doc, json);
    return json;
}

void setupGpsApi() {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("[GPS] UART initialized");

    // REST endpoint
    apiServer.http().on("/api/gps", HTTP_GET, [](AsyncWebServerRequest* req) {
        ApiServer::restRequestCount++;
        req->send(200, "application/json", gpsToJson());
    });

    // POST /api/gps/rate — configure broadcast rate
    apiServer.http().on("/api/gps/rate", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }
            gpsRateMs = doc["rate_ms"] | 1000UL;
            if (gpsRateMs < 100) gpsRateMs = 100;   // minimum 100ms
            if (gpsRateMs > 10000) gpsRateMs = 10000; // maximum 10s

            JsonDocument resp;
            resp["rate_ms"] = gpsRateMs;
            resp["status"] = "configured";
            String json;
            serializeJson(resp, json);
            ApiServer::restRequestCount++;
            req->send(200, "application/json", json);
        }
    );

    // WebSocket stream
    wsGps = apiServer.addWebSocket("/ws/gps");

    Serial.println("[API] GPS endpoints registered");
}

void gpsLoop() {
    // Feed GPS parser
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }

    // Broadcast every 1 second
    if (millis() - lastGpsBroadcast >= gpsRateMs) {
        lastGpsBroadcast = millis();
        if (wsGps && wsGps->count() > 0) {
            ApiServer::broadcast(wsGps, gpsToJson());
        }
    }
}
