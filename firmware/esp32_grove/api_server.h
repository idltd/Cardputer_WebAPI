#pragma once

#include "config.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class ApiServer {
public:
    void begin();
    void loop();

    AsyncWebServer& http() { return _server; }

    static void sendError(AsyncWebServerRequest* req, int code, const char* msg);
    static void sendOk(AsyncWebServerRequest* req, const char* msg = "ok");

    static unsigned long restRequestCount;

private:
    AsyncWebServer _server{HTTP_PORT};
};

extern ApiServer apiServer;
