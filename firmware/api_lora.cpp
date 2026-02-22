#include "config.h"
#include "api_server.h"
#include "api_lora.h"
#include <RadioLib.h>

// SX1262 instance using custom SPI pins
static SPIClass loraSpi(HSPI);
static SX1262 lora = new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, loraSpi);

static AsyncWebSocket* wsLora = nullptr;
bool loraReady = false;

// Current config (mutable)
static float loraFreq   = LORA_DEFAULT_FREQ;
static float loraBw     = LORA_DEFAULT_BW;
static int   loraSf     = LORA_DEFAULT_SF;
static int   loraCr     = LORA_DEFAULT_CR;
static int   loraPower  = LORA_DEFAULT_POWER;
static int   loraSyncWord = LORA_DEFAULT_SYNC;

// Flag for received packet (set from ISR)
static volatile bool loraPacketReceived = false;
static portMUX_TYPE loraMux = portMUX_INITIALIZER_UNLOCKED;

#if defined(ESP32)
IRAM_ATTR
#endif
static void loraISR() {
    loraPacketReceived = true;
}

String configToJson() {
    JsonDocument doc;
    doc["frequency_mhz"] = loraFreq;
    doc["bandwidth_khz"] = loraBw;
    doc["spreading_factor"] = loraSf;
    doc["coding_rate"] = loraCr;
    doc["tx_power_dbm"] = loraPower;
    doc["sync_word"] = loraSyncWord;
    doc["ready"] = loraReady;

    String json;
    serializeJson(doc, json);
    return json;
}

void setupLoraApi() {
    loraSpi.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);

    Serial.print("[LoRa] Initializing SX1262... ");
    int state = lora.begin(loraFreq, loraBw, loraSf, loraCr, loraSyncWord, loraPower, LORA_DEFAULT_PREAMBLE);
    if (state == RADIOLIB_ERR_NONE) {
        loraReady = true;
        Serial.println("OK");

        // Set up receive interrupt
        lora.setDio1Action(loraISR);
        lora.startReceive();
    } else {
        Serial.printf("FAILED (code %d)\n", state);
        Serial.println("[LoRa] Check pin configuration and DIP switches");
    }

    // GET /api/lora/config
    apiServer.http().on("/api/lora/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        ApiServer::restRequestCount++;
        req->send(200, "application/json", configToJson());
    });

    // POST /api/lora/config — reconfigure LoRa parameters
    apiServer.http().on("/api/lora/config", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!loraReady) {
                ApiServer::sendError(req, 503, "LoRa not initialized");
                return;
            }

            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }

            bool changed = false;
            int result;

            if (doc["frequency_mhz"].is<float>()) {
                float freq = doc["frequency_mhz"];
                if (freq < 137.0 || freq > 1020.0) {
                    ApiServer::sendError(req, 400, "Frequency must be 137-1020 MHz");
                    return;
                }
                result = lora.setFrequency(freq);
                if (result != RADIOLIB_ERR_NONE) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "setFrequency failed (code %d)", result);
                    ApiServer::sendError(req, 500, msg);
                    return;
                }
                loraFreq = freq;
                changed = true;
            }
            if (doc["bandwidth_khz"].is<float>()) {
                float bw = doc["bandwidth_khz"];
                result = lora.setBandwidth(bw);
                if (result != RADIOLIB_ERR_NONE) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "setBandwidth failed (code %d)", result);
                    ApiServer::sendError(req, 500, msg);
                    return;
                }
                loraBw = bw;
                changed = true;
            }
            if (doc["spreading_factor"].is<int>()) {
                int sf = doc["spreading_factor"];
                if (sf < 7 || sf > 12) {
                    ApiServer::sendError(req, 400, "Spreading factor must be 7-12");
                    return;
                }
                result = lora.setSpreadingFactor(sf);
                if (result != RADIOLIB_ERR_NONE) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "setSpreadingFactor failed (code %d)", result);
                    ApiServer::sendError(req, 500, msg);
                    return;
                }
                loraSf = sf;
                changed = true;
            }
            if (doc["coding_rate"].is<int>()) {
                int cr = doc["coding_rate"];
                if (cr < 5 || cr > 8) {
                    ApiServer::sendError(req, 400, "Coding rate must be 5-8");
                    return;
                }
                result = lora.setCodingRate(cr);
                if (result != RADIOLIB_ERR_NONE) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "setCodingRate failed (code %d)", result);
                    ApiServer::sendError(req, 500, msg);
                    return;
                }
                loraCr = cr;
                changed = true;
            }
            if (doc["tx_power_dbm"].is<int>()) {
                int pwr = doc["tx_power_dbm"];
                if (pwr < -9 || pwr > 22) {
                    ApiServer::sendError(req, 400, "TX power must be -9 to 22 dBm");
                    return;
                }
                result = lora.setOutputPower(pwr);
                if (result != RADIOLIB_ERR_NONE) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "setOutputPower failed (code %d)", result);
                    ApiServer::sendError(req, 500, msg);
                    return;
                }
                loraPower = pwr;
                changed = true;
            }
            if (doc["sync_word"].is<int>()) {
                int sw = doc["sync_word"];
                result = lora.setSyncWord(sw);
                if (result != RADIOLIB_ERR_NONE) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "setSyncWord failed (code %d)", result);
                    ApiServer::sendError(req, 500, msg);
                    return;
                }
                loraSyncWord = sw;
                changed = true;
            }

            if (changed) {
                lora.startReceive(); // re-enter receive mode
            }

            ApiServer::restRequestCount++;
            req->send(200, "application/json", configToJson());
        }
    );

    // POST /api/lora/send — transmit a LoRa packet
    apiServer.http().on("/api/lora/send", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!loraReady) {
                ApiServer::sendError(req, 503, "LoRa not initialized");
                return;
            }

            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }

            const char* payload = doc["data"] | "";
            bool isHex = doc["hex"] | false;
            int state;
            int byteCount = 0;

            if (isHex) {
                // Parse hex string to bytes
                String hexStr = payload;
                if (hexStr.length() == 0 || hexStr.length() % 2 != 0) {
                    ApiServer::sendError(req, 400, "Hex string must have even length");
                    return;
                }
                int byteLen = hexStr.length() / 2;
                uint8_t* bytes = new (std::nothrow) uint8_t[byteLen];
                if (!bytes) {
                    ApiServer::sendError(req, 500, "Memory allocation failed");
                    return;
                }
                for (int i = 0; i < byteLen; i++) {
                    bytes[i] = strtoul(hexStr.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
                }
                state = lora.transmit(bytes, byteLen);
                byteCount = byteLen;
                delete[] bytes;
            } else {
                state = lora.transmit(payload);
                byteCount = strlen(payload);
            }

            // Return to receive mode
            lora.startReceive();

            if (state == RADIOLIB_ERR_NONE) {
                JsonDocument resp;
                resp["status"] = "sent";
                resp["bytes"] = byteCount;
                String json;
                serializeJson(resp, json);
                ApiServer::restRequestCount++;
                req->send(200, "application/json", json);
            } else {
                char msg[64];
                snprintf(msg, sizeof(msg), "Transmit failed (code %d)", state);
                ApiServer::sendError(req, 500, msg);
            }
        }
    );

    // WebSocket for received packets
    wsLora = apiServer.addWebSocket("/ws/lora");

    Serial.println("[API] LoRa endpoints registered");
}

void loraLoop() {
    if (!loraReady) return;

    // Atomically check and clear the flag
    bool received = false;
    portENTER_CRITICAL(&loraMux);
    if (loraPacketReceived) {
        received = true;
        loraPacketReceived = false;
    }
    portEXIT_CRITICAL(&loraMux);

    if (!received) return;

    String data;
    int state = lora.readData(data);
    if (state == RADIOLIB_ERR_NONE && data.length() > 0) {
        JsonDocument doc;
        doc["data"] = data;
        doc["rssi"] = lora.getRSSI();
        doc["snr"] = lora.getSNR();
        doc["freq_error"] = lora.getFrequencyError();
        doc["length"] = data.length();

        // Also provide hex representation
        String hex;
        for (size_t i = 0; i < data.length(); i++) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", (uint8_t)data[i]);
            hex += buf;
        }
        doc["hex"] = hex;

        String json;
        serializeJson(doc, json);
        ApiServer::broadcast(wsLora, json);

        Serial.printf("[LoRa] RX: %d bytes, RSSI: %.1f, SNR: %.1f\n",
                      data.length(), lora.getRSSI(), lora.getSNR());
    }

    lora.startReceive();
}
