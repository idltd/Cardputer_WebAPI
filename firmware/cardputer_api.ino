// ============================================================================
// Cardputer ADV — Hardware API Server
//
// Exposes all Cardputer ADV hardware via REST + WebSocket over WiFi AP.
// Connect any device to the "CardputerADV" WiFi and use HTTP/WS to control
// GPS, LoRa, IR, IMU, Display, GPIO, Audio, and Keyboard.
//
// Libraries required:
//   - M5Cardputer (Board: M5Cardputer via M5Stack board manager)
//   - ESPAsyncWebServer + AsyncTCP
//   - ArduinoJson (v7)
//   - TinyGPSPlus
//   - RadioLib
//   - IRremoteESP8266
// ============================================================================

#include <M5Cardputer.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#include "config.h"
#include "api_server.h"
#include "api_system.h"
#include "api_display.h"
#include "api_gps.h"
#include "api_lora.h"
#include "api_ir.h"
#include "api_gpio.h"
#include "api_imu.h"
#include "api_audio.h"
#include "api_serial.h"

// Keyboard WebSocket
static AsyncWebSocket* wsKeyboard = nullptr;
static unsigned long lastStatusUpdate = 0;

// ── WiFi AP Setup ──────────────────────────────────────────────────────────

static void setupWiFiAP() {
    WiFi.mode(WIFI_AP);

    const char* password = WIFI_AP_PASSWORD;
    if (strlen(password) == 0) {
        WiFi.softAP(WIFI_AP_SSID, NULL, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
        Serial.printf("[WiFi] AP started: %s (open)\n", WIFI_AP_SSID);
    } else {
        WiFi.softAP(WIFI_AP_SSID, password, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
        Serial.printf("[WiFi] AP started: %s (password protected)\n", WIFI_AP_SSID);
    }

    Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
}

// ── mDNS Setup ─────────────────────────────────────────────────────────────

static void setupMDNS() {
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.printf("[mDNS] %s.local\n", MDNS_HOSTNAME);
    } else {
        Serial.println("[mDNS] Failed to start");
    }
}

// ── Keyboard Handling ──────────────────────────────────────────────────────

static void keyboardLoop() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange()) {
        if (!M5Cardputer.Keyboard.isPressed()) return;
        Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();

        // Only broadcast if there's actually a key pressed
        bool hasKey = !state.word.empty() || state.enter || state.del ||
                      state.tab || state.space;
        if (!hasKey) return;

        JsonDocument doc;
        doc["event"] = "press";

        // Characters from word vector
        if (!state.word.empty()) {
            String chars;
            for (char c : state.word) chars += c;
            doc["char"] = chars;
        }

        // All pressed keys as array
        JsonArray keys = doc["keys"].to<JsonArray>();
        for (char key : state.word) {
            keys.add(String(key));
        }

        // Special keys
        doc["enter"] = state.enter;
        doc["del"] = state.del;
        doc["tab"] = state.tab;
        doc["space"] = state.space;

        // Modifier keys
        doc["shift"] = state.shift;
        doc["ctrl"] = state.ctrl;
        doc["alt"] = state.alt;
        doc["opt"] = state.opt;
        doc["fn"] = state.fn;

        String json;
        serializeJson(doc, json);
        ApiServer::broadcast(wsKeyboard, json);
    }
}

// ── Root Page ──────────────────────────────────────────────────────────────

static void setupRootPage() {
    // GET /api/version — firmware version
    apiServer.http().on("/api/version", HTTP_GET, [](AsyncWebServerRequest* req) {
        ApiServer::restRequestCount++;
        JsonDocument doc;
        doc["firmware"] = "CardputerADV";
        doc["version"] = FIRMWARE_VERSION;
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    apiServer.http().on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        String ip = WiFi.softAPIP().toString();
        String html = "<!DOCTYPE html>"
            "<html><head><title>Cardputer ADV API</title>"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
            "<style>"
            "body{font-family:monospace;background:#111;color:#0f0;padding:20px;max-width:600px;margin:0 auto}"
            "h1{color:#0f0;border-bottom:1px solid #0f0;padding-bottom:10px}"
            "a{color:#0ff}"
            ".section{margin:20px 0}"
            ".endpoint{margin:4px 0}"
            ".method{display:inline-block;width:50px;font-weight:bold}"
            ".get{color:#0f0} .post{color:#ff0}"
            "</style></head><body>"
            "<h1>Cardputer ADV API v" FIRMWARE_VERSION "</h1>"
            "<div class=\"section\"><h2>REST Endpoints</h2>"
            "<div class=\"endpoint\"><span class=\"method get\">GET</span> <a href=\"/api/version\">/api/version</a></div>"
            "<div class=\"endpoint\"><span class=\"method get\">GET</span> <a href=\"/api/system/info\">/api/system/info</a></div>"
            "<div class=\"endpoint\"><span class=\"method get\">GET</span> <a href=\"/api/gps\">/api/gps</a></div>"
            "<div class=\"endpoint\"><span class=\"method get\">GET</span> <a href=\"/api/imu\">/api/imu</a></div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/imu/rate</div>"
            "<div class=\"endpoint\"><span class=\"method get\">GET</span> <a href=\"/api/lora/config\">/api/lora/config</a></div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/lora/config</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/lora/send</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/ir/send</div>"
            "<div class=\"endpoint\"><span class=\"method get\">GET</span> /api/gpio/{pin}</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/gpio/{pin}</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/gpio/{pin}/mode</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/display/text</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/display/clear</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/display/fill</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/audio/tone</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/audio/volume</div>"
            "<div class=\"endpoint\"><span class=\"method post\">POST</span> /api/audio/stop</div>"
            "</div>"
            "<div class=\"section\"><h2>WebSocket Streams</h2>"
            "<div class=\"endpoint\">ws://" + ip + "/ws/gps</div>"
            "<div class=\"endpoint\">ws://" + ip + "/ws/lora</div>"
            "<div class=\"endpoint\">ws://" + ip + "/ws/keyboard</div>"
            "<div class=\"endpoint\">ws://" + ip + "/ws/imu</div>"
            "</div>"
            "</body></html>";
        req->send(200, "text/html", html);
    });
}

// ── Arduino Setup ──────────────────────────────────────────────────────────

void setup() {
    // Initialize M5Cardputer (display, keyboard, IMU, speaker)
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextSize(1);

    Serial.begin(115200);
    Serial.println("\n[Cardputer ADV] Hardware API Server starting...");

    // Show boot screen
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_YELLOW);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("CardputerADV API");
    M5Cardputer.Display.println("Starting...");

    // Start WiFi AP
    setupWiFiAP();

    // Start mDNS
    setupMDNS();

    // Initialize speaker with default volume
    M5Cardputer.Speaker.setVolume(AUDIO_DEFAULT_VOLUME);

    // Register API modules
    setupRootPage();
    setupSystemApi();
    setupDisplayApi();
    setupGpioApi();
    setupAudioApi();

    // Keyboard WebSocket
    wsKeyboard = apiServer.addWebSocket("/ws/keyboard");

    // GPS (UART-based, may fail gracefully if no GPS module)
    setupGpsApi();

    // LoRa (SPI-based, may fail gracefully if no LoRa module)
    setupLoraApi();

    // IR
    setupIrApi();

    // IMU
    setupImuApi();

    // Serial command interface (USB)
    setupSerialApi();

    // Start HTTP server (must be called after all routes are registered)
    apiServer.begin();

    Serial.println("[Cardputer ADV] All systems ready!");
    Serial.printf("[Cardputer ADV] Connect to WiFi '%s' and browse to http://%s/\n",
                  WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());

    // Show status screen
    displayShowStatus(WIFI_AP_SSID, WiFi.softAPIP().toString().c_str(),
                      WiFi.softAPgetStationNum());
}

// ── Arduino Loop ───────────────────────────────────────────────────────────

void loop() {
    // Serial command processing (USB)
    serialApiLoop();

    // WebSocket cleanup
    apiServer.loop();

    // Keyboard events
    keyboardLoop();

    // GPS NMEA parsing + WebSocket broadcast
    gpsLoop();

    // LoRa received packet handling
    loraLoop();

    // (IR is fire-and-forget, no loop needed)

    // IMU WebSocket streaming
    imuLoop();

    // Periodic status display update
    if (millis() - lastStatusUpdate >= DISPLAY_STATUS_UPDATE_MS) {
        lastStatusUpdate = millis();
        displayShowStatus(WIFI_AP_SSID, WiFi.softAPIP().toString().c_str(),
                          WiFi.softAPgetStationNum());
    }
}
