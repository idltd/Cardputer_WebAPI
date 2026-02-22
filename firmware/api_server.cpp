#include "config.h"
#include "api_server.h"

ApiServer apiServer;

unsigned long ApiServer::restRequestCount = 0;
unsigned long ApiServer::wsMessageCount = 0;

void ApiServer::begin() {
    // CORS headers for browser access
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, X-API-Key");

    // Handle preflight OPTIONS for all routes
    _server.onNotFound([](AsyncWebServerRequest* req) {
        if (req->method() == HTTP_OPTIONS) {
            req->send(200);
        } else {
            sendError(req, 404, "Not found");
        }
    });

    _server.begin();
    Serial.printf("[API] HTTP server started on port %d\n", HTTP_PORT);
}

void ApiServer::loop() {
    for (auto& ep : _wsEndpoints) {
        ep.ws->cleanupClients();
    }
}

AsyncWebSocket* ApiServer::addWebSocket(const char* path) {
    AsyncWebSocket* ws = new AsyncWebSocket(path);
    ws->onEvent(_onWsEvent);
    _server.addHandler(ws);
    _wsEndpoints.push_back({ws, path});
    Serial.printf("[API] WebSocket endpoint: %s\n", path);
    return ws;
}

void ApiServer::broadcast(AsyncWebSocket* ws, const String& json) {
    if (ws && ws->count() > 0) {
        wsMessageCount++;
        ws->textAll(json);
    }
}

int ApiServer::wsClientCount() {
    int count = 0;
    for (auto& ep : _wsEndpoints) {
        count += ep.ws->count();
    }
    return count;
}

void ApiServer::sendError(AsyncWebServerRequest* req, int code, const char* msg) {
    restRequestCount++;
    JsonDocument doc;
    doc["error"] = msg;
    doc["code"] = code;
    String json;
    serializeJson(doc, json);
    req->send(code, "application/json", json);
}

void ApiServer::sendOk(AsyncWebServerRequest* req, const char* msg) {
    restRequestCount++;
    JsonDocument doc;
    doc["status"] = msg;
    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
}

void ApiServer::_onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                           AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] %s client #%u connected from %s\n",
                          server->url(), client->id(),
                          client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] %s client #%u disconnected\n",
                          server->url(), client->id());
            break;
        case WS_EVT_ERROR:
            Serial.printf("[WS] %s client #%u error\n",
                          server->url(), client->id());
            break;
        case WS_EVT_DATA:
            wsMessageCount++;
            break;
        case WS_EVT_PONG:
            break;
    }
}
