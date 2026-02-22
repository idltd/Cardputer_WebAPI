#include "config.h"
#include "api_serial.h"
#include "api_server.h"
#include <M5Cardputer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include "api_ir.h"

// External references to module state (defined in their respective .cpp files)
extern TinyGPSPlus gps;
extern bool loraReady;

// Forward declarations for functions we'll call from modules
extern String gpsToJson();
extern String imuToJson();
extern String configToJson(); // LoRa config

static String serialBuffer;

// Build system info JSON (same as api_system.cpp but without req dependency)
static String systemInfoJson() {
    JsonDocument doc;
    doc["chip"]["model"] = ESP.getChipModel();
    doc["chip"]["revision"] = ESP.getChipRevision();
    doc["chip"]["cores"] = ESP.getChipCores();
    doc["chip"]["freq_mhz"] = ESP.getCpuFreqMHz();
    doc["heap"]["free"] = ESP.getFreeHeap();
    doc["heap"]["total"] = ESP.getHeapSize();
    doc["heap"]["min_free"] = ESP.getMinFreeHeap();
    doc["psram"]["free"] = ESP.getFreePsram();
    doc["psram"]["total"] = ESP.getPsramSize();
    doc["uptime_ms"] = millis();
    doc["sdk_version"] = ESP.getSdkVersion();
    doc["flash"]["size"] = ESP.getFlashChipSize();
    doc["flash"]["speed"] = ESP.getFlashChipSpeed();
    doc["wifi"]["ssid"] = WIFI_AP_SSID;
    doc["wifi"]["ip"] = WiFi.softAPIP().toString();
    doc["wifi"]["clients"] = WiFi.softAPgetStationNum();
    doc["ws_clients"] = apiServer.wsClientCount();
    doc["traffic"]["rest_requests"] = ApiServer::restRequestCount;
    doc["traffic"]["ws_messages"] = ApiServer::wsMessageCount;
    String json;
    serializeJson(doc, json);
    return json;
}

static String versionJson() {
    JsonDocument doc;
    doc["firmware"] = "CardputerADV";
    doc["version"] = FIRMWARE_VERSION;
    doc["transport"] = "serial";
    String json;
    serializeJson(doc, json);
    return json;
}

static void sendResponse(const String& json) {
    Serial.println(json);
}

static void sendError(const char* msg) {
    JsonDocument doc;
    doc["error"] = msg;
    String json;
    serializeJson(doc, json);
    Serial.println(json);
}

static void handleCommand(const String& line) {
    JsonDocument cmd;
    if (deserializeJson(cmd, line)) {
        sendError("Invalid JSON");
        return;
    }

    const char* path = cmd["path"] | "";
    const char* method = cmd["method"] | "GET";

    // GET endpoints
    if (strcasecmp(method, "GET") == 0) {
        if (strcmp(path, "/api/version") == 0) {
            sendResponse(versionJson());
        }
        else if (strcmp(path, "/api/system/info") == 0) {
            sendResponse(systemInfoJson());
        }
        else if (strcmp(path, "/api/gps") == 0) {
            sendResponse(gpsToJson());
        }
        else if (strcmp(path, "/api/imu") == 0) {
            sendResponse(imuToJson());
        }
        else if (strcmp(path, "/api/lora/config") == 0) {
            sendResponse(configToJson());
        }
        else {
            sendError("Unknown path");
        }
    }
    // POST endpoints
    else if (strcasecmp(method, "POST") == 0) {
        JsonVariant body = cmd["body"];

        if (strcmp(path, "/api/audio/tone") == 0) {
            int freq = body["freq"] | 1000;
            int duration = body["duration"] | 500;
            if (freq < 20 || freq > 20000) { sendError("Frequency must be 20-20000 Hz"); return; }
            if (duration < 1 || duration > 10000) { sendError("Duration must be 1-10000 ms"); return; }
            M5Cardputer.Speaker.tone(freq, duration);
            JsonDocument resp;
            resp["status"] = "playing";
            resp["freq"] = freq;
            resp["duration"] = duration;
            String json;
            serializeJson(resp, json);
            sendResponse(json);
        }
        else if (strcmp(path, "/api/audio/volume") == 0) {
            int vol = body["volume"] | 128;
            if (vol < 0) vol = 0;
            if (vol > 255) vol = 255;
            M5Cardputer.Speaker.setVolume(vol);
            JsonDocument resp;
            resp["status"] = "set";
            resp["volume"] = vol;
            String json;
            serializeJson(resp, json);
            sendResponse(json);
        }
        else if (strcmp(path, "/api/audio/stop") == 0) {
            M5Cardputer.Speaker.stop();
            JsonDocument resp;
            resp["status"] = "stopped";
            String json;
            serializeJson(resp, json);
            sendResponse(json);
        }
        else if (strcmp(path, "/api/display/text") == 0) {
            const char* text = body["text"] | "";
            int x = body["x"] | 0;
            int y = body["y"] | 0;
            int size = body["size"] | 1;
            M5Cardputer.Display.setTextSize(size);
            M5Cardputer.Display.setTextColor(TFT_WHITE);
            M5Cardputer.Display.setCursor(x, y);
            M5Cardputer.Display.print(text);
            JsonDocument resp;
            resp["status"] = "text drawn";
            String json;
            serializeJson(resp, json);
            sendResponse(json);
        }
        else if (strcmp(path, "/api/display/clear") == 0) {
            M5Cardputer.Display.fillScreen(TFT_BLACK);
            JsonDocument resp;
            resp["status"] = "display cleared";
            String json;
            serializeJson(resp, json);
            sendResponse(json);
        }
        else if (strcmp(path, "/api/ir/send") == 0) {
            const char* protocol = body["protocol"] | "nec";
            uint16_t address = body["address"] | (uint16_t)0;
            uint8_t command = body["command"] | (uint8_t)0;
            int repeats = body["repeats"] | 0;
            if (!irSend(protocol, address, command, repeats)) {
                sendError("Unknown IR protocol");
                return;
            }
            JsonDocument resp;
            resp["status"] = "sent";
            resp["protocol"] = protocol;
            String json;
            serializeJson(resp, json);
            sendResponse(json);
        }
        else {
            sendError("Unknown path");
        }
    }
    else {
        sendError("Unknown method (use GET or POST)");
    }
}

void setupSerialApi() {
    Serial.println("[API] Serial command interface ready");
    Serial.println("[API] Send JSON: {\"method\":\"GET\",\"path\":\"/api/version\"}");
}

void serialApiLoop() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialBuffer.length() > 0) {
                handleCommand(serialBuffer);
                serialBuffer = "";
            }
        } else {
            serialBuffer += c;
        }
    }
}
