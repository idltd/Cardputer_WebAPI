#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>

// WebSocket client tracking per endpoint
struct WsEndpoint {
    AsyncWebSocket* ws;
    const char* path;
};

class ApiServer {
public:
    void begin();
    void loop();

    // Register a WebSocket endpoint and return its pointer
    AsyncWebSocket* addWebSocket(const char* path);

    // Broadcast JSON string to all clients on a WebSocket
    static void broadcast(AsyncWebSocket* ws, const String& json);

    // Get the underlying HTTP server for route registration
    AsyncWebServer& http() { return _server; }

    // Connected client count (across all WS endpoints)
    int wsClientCount();

    // Send a JSON error response
    static void sendError(AsyncWebServerRequest* req, int code, const char* msg);

    // Send a JSON success response with a message
    static void sendOk(AsyncWebServerRequest* req, const char* msg = "ok");

    // Traffic counters
    static unsigned long restRequestCount;
    static unsigned long wsMessageCount;

private:
    AsyncWebServer _server{HTTP_PORT};
    std::vector<WsEndpoint> _wsEndpoints;

    static void _onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                           AwsEventType type, void* arg, uint8_t* data, size_t len);
};

extern ApiServer apiServer;
