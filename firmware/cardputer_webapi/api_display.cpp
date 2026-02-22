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
    const int W = lcd.width();   // 240
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextSize(1);

    // ── Header bar ───────────────────────────────────────────────────────────
    lcd.fillRect(0, 0, W, 13, lcd.color565(0, 110, 0));
    lcd.setTextColor(TFT_BLACK);
    lcd.setCursor(3, 3);
    lcd.printf("CardputerADV v%s", FIRMWARE_VERSION);
    lcd.drawLine(0, 13, W, 13, lcd.color565(0, 200, 80));

    // ── Network ──────────────────────────────────────────────────────────────
    lcd.setTextColor(TFT_YELLOW);
    lcd.setCursor(2, 16);
    lcd.printf("SSID: %s", ssid);

    lcd.setTextColor(TFT_CYAN);
    lcd.setCursor(2, 26);
    lcd.printf("IP: %s  :%d", ip, HTTP_PORT);

    lcd.setTextColor(TFT_WHITE);
    lcd.setCursor(2, 36);
    unsigned long upSec = millis() / 1000;
    if (upSec < 60) {
        lcd.printf("Up: %lus", upSec);
    } else if (upSec < 3600) {
        lcd.printf("Up: %lum %02lus", upSec / 60, upSec % 60);
    } else {
        lcd.printf("Up: %luh %02lum", upSec / 3600, (upSec % 3600) / 60);
    }

    lcd.drawLine(0, 46, W, 46, lcd.color565(40, 40, 40));

    // ── Activity ─────────────────────────────────────────────────────────────
    lcd.setTextColor(TFT_GREEN);
    lcd.setCursor(2, 49);
    lcd.printf("WiFi: %d  WS: %d clients", clients, apiServer.wsClientCount());

    lcd.setCursor(2, 59);
    lcd.printf("REST: %lu  WS msgs: %lu", ApiServer::restRequestCount, ApiServer::wsMessageCount);

    lcd.drawLine(0, 69, W, 69, lcd.color565(40, 40, 40));

    // ── Heap bar ─────────────────────────────────────────────────────────────
    uint32_t heapFree  = ESP.getFreeHeap();
    uint32_t heapTotal = ESP.getHeapSize();
    lcd.setTextColor(TFT_WHITE);
    lcd.setCursor(2, 72);
    lcd.printf("Heap: %dK / %dK free", heapFree / 1024, heapTotal / 1024);

    int barW   = W - 6;
    int fillW  = (heapTotal > 0) ? (int)((float)heapFree / heapTotal * barW) : 0;
    lcd.drawRect(3, 82, barW, 8, lcd.color565(60, 60, 60));
    lcd.fillRect(4, 83, fillW - 2, 6, lcd.color565(0, 200, 100));

    lcd.drawLine(0, 93, W, 93, lcd.color565(40, 40, 40));

    // ── Access URL ───────────────────────────────────────────────────────────
    lcd.setTextColor(TFT_CYAN);
    lcd.setCursor(2, 96);
    lcd.printf("http://%s/", ip);

    lcd.setTextColor(lcd.color565(0, 200, 100));
    lcd.setCursor(2, 106);
    lcd.print("Open in browser to control");

    lcd.drawLine(0, 117, W, 117, lcd.color565(40, 40, 40));

    // ── Footer ───────────────────────────────────────────────────────────────
    lcd.setTextColor(lcd.color565(90, 90, 90));
    lcd.setCursor(2, 120);
    lcd.printf("mDNS: %s.local", MDNS_HOSTNAME);
}
