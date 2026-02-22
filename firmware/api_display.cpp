#include "config.h"
#include "api_server.h"
#include "api_display.h"
#include <M5Cardputer.h>

// Parse "#RRGGBB" hex colour string to 565 format. Returns false if invalid.
static bool parseColor(const char* hexColor, uint32_t& out) {
    if (!hexColor || hexColor[0] != '#' || strlen(hexColor) != 7) return false;
    for (int i = 1; i <= 6; i++) {
        char c = hexColor[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
    }
    uint32_t rgb = strtoul(hexColor + 1, NULL, 16);
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;
    out = M5Cardputer.Display.color565(r, g, b);
    return true;
}

void setupDisplayApi() {
    // POST /api/display/text — draw text on screen
    apiServer.http().on("/api/display/text", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }

            const char* text = doc["text"] | "";
            int x = doc["x"] | 0;
            int y = doc["y"] | 0;
            int size = doc["size"] | 1;
            uint32_t color = TFT_WHITE;

            if (doc["color"].is<const char*>()) {
                if (!parseColor(doc["color"], color)) {
                    ApiServer::sendError(req, 400, "Invalid color (use #RRGGBB)");
                    return;
                }
            }

            M5Cardputer.Display.setTextSize(size);
            M5Cardputer.Display.setTextColor(color);
            M5Cardputer.Display.setCursor(x, y);
            M5Cardputer.Display.print(text);

            ApiServer::sendOk(req, "text drawn");
        }
    );

    // POST /api/display/clear — clear screen
    apiServer.http().on("/api/display/clear", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            uint32_t color = TFT_BLACK;

            if (len > 0) {
                JsonDocument doc;
                if (!deserializeJson(doc, data, len)) {
                    if (doc["color"].is<const char*>()) {
                        if (!parseColor(doc["color"], color)) {
                            ApiServer::sendError(req, 400, "Invalid color (use #RRGGBB)");
                            return;
                        }
                    }
                }
            }

            M5Cardputer.Display.fillScreen(color);
            ApiServer::sendOk(req, "display cleared");
        }
    );

    // POST /api/display/fill — fill rectangle
    apiServer.http().on("/api/display/fill", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }

            int x = doc["x"] | 0;
            int y = doc["y"] | 0;
            int w = doc["w"] | 10;
            int h = doc["h"] | 10;
            uint32_t color = TFT_WHITE;

            if (doc["color"].is<const char*>()) {
                if (!parseColor(doc["color"], color)) {
                    ApiServer::sendError(req, 400, "Invalid color (use #RRGGBB)");
                    return;
                }
            }

            M5Cardputer.Display.fillRect(x, y, w, h, color);
            ApiServer::sendOk(req, "rect filled");
        }
    );

    // GET /api/display/clear — also allow GET for convenience
    apiServer.http().on("/api/display/clear", HTTP_GET, [](AsyncWebServerRequest* req) {
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        ApiServer::sendOk(req, "display cleared");
    });

    Serial.println("[API] Display endpoints registered");
}

void displayShowStatus(const char* ssid, const char* ip, int clients) {
    auto& lcd = M5Cardputer.Display;
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_GREEN);
    lcd.setTextSize(1);

    lcd.setCursor(0, 0);
    lcd.printf("CardputerADV v%s", FIRMWARE_VERSION);
    lcd.println();
    lcd.println();
    lcd.printf("SSID: %s\n", ssid);
    lcd.printf("IP:   %s\n", ip);
    lcd.printf("Port: %d\n", HTTP_PORT);
    lcd.println();
    lcd.printf("WiFi clients: %d\n", clients);
    lcd.printf("WS clients:   %d\n", apiServer.wsClientCount());
    lcd.println();
    lcd.printf("Heap: %d / %d\n", ESP.getFreeHeap(), ESP.getHeapSize());
    lcd.printf("Up: %lus\n", millis() / 1000);
    lcd.println();
    lcd.setTextColor(TFT_CYAN);
    lcd.printf("REST: %lu  WS: %lu\n", ApiServer::restRequestCount, ApiServer::wsMessageCount);
}
