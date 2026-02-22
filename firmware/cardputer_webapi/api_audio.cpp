#include "config.h"
#include "api_server.h"
#include "api_audio.h"
#include <M5Cardputer.h>

void setupAudioApi() {
    // POST /api/audio/tone — play a tone
    apiServer.http().on("/api/audio/tone", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }

            int freq = doc["freq"] | 1000;
            int duration = doc["duration"] | 500;

            if (freq < 20 || freq > 20000) {
                ApiServer::sendError(req, 400, "Frequency must be 20–20000 Hz");
                return;
            }
            if (duration < 1 || duration > 10000) {
                ApiServer::sendError(req, 400, "Duration must be 1–10000 ms");
                return;
            }

            M5Cardputer.Speaker.tone(freq, duration);

            JsonDocument resp;
            resp["status"] = "playing";
            resp["freq"] = freq;
            resp["duration"] = duration;
            String json;
            serializeJson(resp, json);
            req->send(200, "application/json", json);
        }
    );

    // POST /api/audio/volume — set speaker volume
    apiServer.http().on("/api/audio/volume", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }

            int volume = doc["volume"] | AUDIO_DEFAULT_VOLUME;
            if (volume < 0) volume = 0;
            if (volume > 255) volume = 255;

            M5Cardputer.Speaker.setVolume(volume);

            JsonDocument resp;
            resp["status"] = "set";
            resp["volume"] = volume;
            String json;
            serializeJson(resp, json);
            req->send(200, "application/json", json);
        }
    );

    // POST /api/audio/stop — stop any playing audio
    apiServer.http().on("/api/audio/stop", HTTP_POST, [](AsyncWebServerRequest* req) {
        M5Cardputer.Speaker.stop();
        ApiServer::sendOk(req, "stopped");
    });

    Serial.println("[API] Audio endpoints registered");
}
