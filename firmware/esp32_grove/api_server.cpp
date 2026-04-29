#include "config.h"
#include "api_server.h"

unsigned long ApiServer::restRequestCount = 0;

ApiServer apiServer;

void ApiServer::begin() {
    _server.begin();
    Serial.println("[API] HTTP server started on port " + String(HTTP_PORT));
}

void ApiServer::loop() {
    // ESPAsyncWebServer is fully async — nothing to poll
}

void ApiServer::sendError(AsyncWebServerRequest* req, int code, const char* msg) {
    JsonDocument doc;
    doc["error"] = msg;
    String json;
    serializeJson(doc, json);
    req->send(code, "application/json", json);
}

void ApiServer::sendOk(AsyncWebServerRequest* req, const char* msg) {
    JsonDocument doc;
    doc["status"] = msg;
    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
}
