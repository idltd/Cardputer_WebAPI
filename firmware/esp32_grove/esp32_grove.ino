// ============================================================================
// ESP32 Grove Sensor Explorer
//
// Headless WiFi AP + REST API + SSE streaming for Grove-compatible sensors.
// Connect to "GroveExplorer" WiFi and open http://192.168.4.1/ in a browser.
//
// Libraries required (install via Arduino Library Manager):
//   - ESPAsyncWebServer + AsyncTCP  (me-no-dev)
//   - ArduinoJson (v7)              (bblanchon)
//   - DHTesp                        (beegee-tokyo)
//   - OneWire                       (Paul Stoffregen)
//   - DallasTemperature             (Miles Burton)
// ============================================================================

#include <WiFi.h>
#include <ESPmDNS.h>

#include "config.h"
#include "api_server.h"
#include "api_grove.h"
#include "api_webapp.h"

static void setupWiFiAP() {
    WiFi.mode(WIFI_AP);
    const char* pw = WIFI_AP_PASSWORD;
    if (strlen(pw) == 0) {
        WiFi.softAP(WIFI_AP_SSID, NULL, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
        Serial.printf("[WiFi] AP: %s (open)\n", WIFI_AP_SSID);
    } else {
        WiFi.softAP(WIFI_AP_SSID, pw, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
        Serial.printf("[WiFi] AP: %s (password protected)\n", WIFI_AP_SSID);
    }
    Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
}

static void setupMDNS() {
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.printf("[mDNS] http://%s.local/\n", MDNS_HOSTNAME);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n[Grove Explorer] Starting...");

    setupWiFiAP();
    setupMDNS();

    setupWebApp();   // PWA served at /
    setupGroveApi(); // Grove sensor API

    apiServer.begin();

    Serial.printf("[Grove Explorer] Ready. Connect to WiFi '%s' → http://%s/\n",
                  WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
}

void loop() {
    apiServer.loop();
    groveLoop();
}
