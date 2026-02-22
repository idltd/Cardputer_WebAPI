#include "config.h"
#include "api_server.h"
#include "api_imu.h"
#include <M5Cardputer.h>

static AsyncWebSocket* wsImu = nullptr;
static unsigned long lastImuBroadcast = 0;
static unsigned long imuRateMs = IMU_DEFAULT_RATE_MS;

String imuToJson() {
    float ax, ay, az, gx, gy, gz;
    M5.Imu.update();
    M5.Imu.getAccel(&ax, &ay, &az);
    M5.Imu.getGyro(&gx, &gy, &gz);

    JsonDocument doc;
    doc["accel"]["x"] = ax;
    doc["accel"]["y"] = ay;
    doc["accel"]["z"] = az;
    doc["gyro"]["x"] = gx;
    doc["gyro"]["y"] = gy;
    doc["gyro"]["z"] = gz;

    String json;
    serializeJson(doc, json);
    return json;
}

void setupImuApi() {
    // GET /api/imu — single reading
    apiServer.http().on("/api/imu", HTTP_GET, [](AsyncWebServerRequest* req) {
        ApiServer::restRequestCount++;
        req->send(200, "application/json", imuToJson());
    });

    // POST /api/imu/rate — configure stream rate
    apiServer.http().on("/api/imu/rate", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON");
                return;
            }
            imuRateMs = doc["rate_ms"] | IMU_DEFAULT_RATE_MS;
            if (imuRateMs < 10) imuRateMs = 10; // minimum 10ms

            JsonDocument resp;
            resp["rate_ms"] = imuRateMs;
            resp["status"] = "configured";
            String json;
            serializeJson(resp, json);
            req->send(200, "application/json", json);
        }
    );

    wsImu = apiServer.addWebSocket("/ws/imu");
    Serial.println("[API] IMU endpoints registered");
}

void imuLoop() {
    if (millis() - lastImuBroadcast >= imuRateMs) {
        lastImuBroadcast = millis();
        if (wsImu && wsImu->count() > 0) {
            ApiServer::broadcast(wsImu, imuToJson());
        }
    }
}
