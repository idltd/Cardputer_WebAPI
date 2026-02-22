#include "config.h"
#include "api_server.h"
#include "api_ir.h"

// TinyIRSender is a lightweight, self-contained IR sender
#include <TinyIRSender.hpp>

void setupIrApi() {
    // POST /api/ir/send — send IR code
    apiServer.http().on("/api/ir/send", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }

            const char* protocol = doc["protocol"] | "nec";
            uint16_t address = 0;
            uint8_t command = 0;
            int repeats = doc["repeats"] | 0;

            // Parse address and command
            if (doc["address"].is<const char*>()) {
                address = strtoul(doc["address"].as<const char*>(), NULL, 16);
            } else {
                address = doc["address"] | (uint16_t)0;
            }

            if (doc["command"].is<const char*>()) {
                command = strtoul(doc["command"].as<const char*>(), NULL, 16);
            } else {
                command = doc["command"] | (uint8_t)0;
            }

            if (strcasecmp(protocol, "nec") == 0) {
                sendNEC(IR_SEND_PIN, address, command, repeats);
            } else if (strcasecmp(protocol, "nec_ext") == 0) {
                sendExtendedNEC(IR_SEND_PIN, address, command, repeats);
            } else if (strcasecmp(protocol, "onkyo") == 0 || strcasecmp(protocol, "nec16") == 0) {
                sendONKYO(IR_SEND_PIN, address, command, repeats);
            } else {
                ApiServer::sendError(req, 400, "Supported protocols: nec, nec_ext, onkyo/nec16");
                return;
            }

            JsonDocument resp;
            resp["status"] = "sent";
            resp["protocol"] = protocol;
            resp["address"] = address;
            resp["command"] = command;
            resp["repeats"] = repeats;
            String json;
            serializeJson(resp, json);
            ApiServer::restRequestCount++;
            req->send(200, "application/json", json);
        }
    );

    Serial.println("[API] IR endpoints registered");
}

bool irSend(const char* protocol, uint16_t address, uint8_t command, int repeats) {
    if (strcasecmp(protocol, "nec") == 0) {
        sendNEC(IR_SEND_PIN, address, command, repeats);
    } else if (strcasecmp(protocol, "nec_ext") == 0) {
        sendExtendedNEC(IR_SEND_PIN, address, command, repeats);
    } else if (strcasecmp(protocol, "onkyo") == 0 || strcasecmp(protocol, "nec16") == 0) {
        sendONKYO(IR_SEND_PIN, address, command, repeats);
    } else {
        return false;
    }
    return true;
}
