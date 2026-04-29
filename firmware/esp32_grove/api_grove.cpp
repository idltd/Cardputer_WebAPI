#include "config.h"
#include "api_server.h"
#include "api_grove.h"
#include <Arduino.h>
#include <DHTesp.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ============================================================================
// Sensor catalogue
// ============================================================================

struct SensorDef {
    const char* id;
    const char* name;
    const char* category;
    // Voltage info
    const char* vcc;           // supply voltage for the module
    bool gpio_safe;            // true = ESP32 3.3V GPIO can connect directly
    const char* voltage_note;  // shown in PWA — explain any risk
    // Pin usage
    bool uses_d2;              // needs both D and D2
    const char* d_role;        // label for D pin (e.g. "Data", "Trigger", "CLK")
    const char* d2_role;       // label for D2 pin (null if unused)
    // What the reading returns
    const char* unit;          // primary unit string
    const char* unit2;         // secondary (e.g. humidity), null if single value
};

static const SensorDef SENSORS[] = {
    {
        "digital_in",  "Digital Input",  "Basic",
        "3.3V", true,
        "Safe. Connect signal to D. Keep signal voltage at or below 3.3V.",
        false, "Signal", nullptr, "HIGH/LOW", nullptr
    },
    {
        "digital_out", "Digital Output", "Basic",
        "3.3V", true,
        "Safe. D drives HIGH (3.3V) or LOW (0V). Do not connect to 5V logic inputs directly.",
        false, "Output", nullptr, "HIGH/LOW", nullptr
    },
    {
        "analog_in",   "Analog Input",   "Basic",
        "3.3V", true,
        "Safe. ADC range 0–3.3V, 12-bit (0–4095). Never apply more than 3.3V to this pin.",
        false, "Signal", nullptr, "0–4095", nullptr
    },
    {
        "pwm_out",     "PWM Output",     "Basic",
        "3.3V", true,
        "Safe. Outputs 3.3V PWM. Set frequency (Hz) and duty cycle (0–255).",
        false, "Output", nullptr, "duty 0–255", nullptr
    },
    {
        "dht11",       "DHT11 Temp & Humidity", "Environment",
        "3.3V–5V", true,
        "Safe. DHT11 runs on 3.3V or 5V; its data pin is open-drain so 3.3V signal is fine. Connect VCC to 3.3V.",
        false, "Data", nullptr, "°C", "%RH"
    },
    {
        "ds18b20",     "DS18B20 Temperature",   "Environment",
        "3.0V–5.5V", true,
        "Safe. DS18B20 runs on 3.3V; 1-Wire data is open-drain — compatible with 3.3V GPIO. Add 4.7kΩ pull-up on D to 3.3V.",
        false, "Data (1-Wire)", nullptr, "°C", nullptr
    },
    {
        "hcsr04",      "HC-SR04 Ultrasonic",    "Distance",
        "5V", false,
        "CAUTION: Standard HC-SR04 requires 5V VCC and its Echo pin outputs 5V — this WILL damage ESP32 GPIO. "
        "Use the HC-SR04P (3.3V version) or add a voltage divider (1kΩ + 2kΩ) on the Echo wire. "
        "Trigger pin is input-only on the sensor and safe to drive from 3.3V.",
        true, "Trigger", "Echo", "cm", nullptr
    },
    {
        "rotary",      "Rotary Encoder",         "Input",
        "3.3V–5V", true,
        "Safe if encoder module has 3.3V-compatible outputs (most KY-040 modules do). "
        "Check your module datasheet. Connect CLK→D, DT→D2. Button pin not used here.",
        true, "CLK", "DT", "steps", nullptr
    },
};
#define SENSOR_COUNT (sizeof(SENSORS) / sizeof(SENSORS[0]))

// ============================================================================
// Runtime state
// ============================================================================

static int groveD  = GROVE_DEFAULT_D;
static int groveD2 = GROVE_DEFAULT_D2;
static const SensorDef* activeSensor = nullptr;
static char activeBoardId[32] = BOARD_DEFAULT_ID;

// Per-sensor state
static DHTesp    dht;
static OneWire*  oneWire    = nullptr;
static DallasTemperature* dallas = nullptr;

// PWM (LEDC)
static const int PWM_CHANNEL = 0;
static const int PWM_FREQ_DEFAULT = 1000;
static const int PWM_RESOLUTION  = 8;   // 8-bit → 0–255
static int pwmFreq = PWM_FREQ_DEFAULT;

// Rotary encoder
static volatile int rotarySteps = 0;
static int lastCLK = LOW;

// SSE
static AsyncEventSource* groveEvents = nullptr;
static unsigned long lastStreamMs = 0;
static unsigned long streamIntervalMs = 500;

// ============================================================================
// Helpers
// ============================================================================

static const SensorDef* findSensor(const char* id) {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (strcmp(SENSORS[i].id, id) == 0) return &SENSORS[i];
    }
    return nullptr;
}

static const BoardProfile* findProfile(const char* id) {
    for (int i = 0; i < (int)BOARD_PROFILE_COUNT; i++) {
        if (strcmp(BOARD_PROFILES[i].id, id) == 0) return &BOARD_PROFILES[i];
    }
    return nullptr;
}

static void teardownSensor() {
    if (activeSensor == nullptr) return;

    if (strcmp(activeSensor->id, "pwm_out") == 0) {
        ledcDetach(groveD);
    }
    if (oneWire)  { delete oneWire;  oneWire  = nullptr; }
    if (dallas)   { delete dallas;   dallas   = nullptr; }
    activeSensor = nullptr;
}

static void configureSensor(const SensorDef* s) {
    teardownSensor();
    activeSensor = s;

    if (strcmp(s->id, "digital_in") == 0) {
        pinMode(groveD, INPUT);
    } else if (strcmp(s->id, "digital_out") == 0) {
        pinMode(groveD, OUTPUT);
        digitalWrite(groveD, LOW);
    } else if (strcmp(s->id, "analog_in") == 0) {
        pinMode(groveD, INPUT);
        analogReadResolution(12);
    } else if (strcmp(s->id, "pwm_out") == 0) {
        ledcAttach(groveD, pwmFreq, PWM_RESOLUTION);
    } else if (strcmp(s->id, "dht11") == 0) {
        dht.setup(groveD, DHTesp::DHT11);
    } else if (strcmp(s->id, "ds18b20") == 0) {
        oneWire = new OneWire(groveD);
        dallas  = new DallasTemperature(oneWire);
        dallas->begin();
    } else if (strcmp(s->id, "hcsr04") == 0) {
        pinMode(groveD,  OUTPUT);
        pinMode(groveD2, INPUT);
        digitalWrite(groveD, LOW);
    } else if (strcmp(s->id, "rotary") == 0) {
        pinMode(groveD,  INPUT_PULLUP);
        pinMode(groveD2, INPUT_PULLUP);
        rotarySteps = 0;
        lastCLK = digitalRead(groveD);
    }
}

// ============================================================================
// Reading
// ============================================================================

static String takeReading() {
    JsonDocument doc;
    if (!activeSensor) {
        doc["error"] = "No sensor configured";
        String s; serializeJson(doc, s); return s;
    }

    doc["sensor"] = activeSensor->id;
    doc["ts"] = millis();

    if (strcmp(activeSensor->id, "digital_in") == 0) {
        int v = digitalRead(groveD);
        doc["value"] = v;
        doc["label"] = v ? "HIGH" : "LOW";

    } else if (strcmp(activeSensor->id, "analog_in") == 0) {
        int raw = analogRead(groveD);
        doc["raw"]     = raw;
        doc["voltage"] = serialized(String(raw * 3.3f / 4095.0f, 3));

    } else if (strcmp(activeSensor->id, "dht11") == 0) {
        TempAndHumidity th = dht.getTempAndHumidity();
        if (dht.getStatus() == DHTesp::ERROR_NONE) {
            doc["temperature"] = serialized(String(th.temperature, 1));
            doc["humidity"]    = serialized(String(th.humidity, 1));
        } else {
            doc["error"] = dht.getStatusString();
        }

    } else if (strcmp(activeSensor->id, "ds18b20") == 0) {
        dallas->requestTemperatures();
        float t = dallas->getTempCByIndex(0);
        if (t == DEVICE_DISCONNECTED_C) {
            doc["error"] = "Sensor not found — check wiring and 4.7kΩ pull-up";
        } else {
            doc["temperature"] = serialized(String(t, 2));
        }

    } else if (strcmp(activeSensor->id, "hcsr04") == 0) {
        digitalWrite(groveD, LOW);  delayMicroseconds(2);
        digitalWrite(groveD, HIGH); delayMicroseconds(10);
        digitalWrite(groveD, LOW);
        long duration = pulseIn(groveD2, HIGH, 30000);  // 30ms timeout
        float cm = duration * 0.0343f / 2.0f;
        if (duration == 0) {
            doc["error"] = "No echo — check wiring. Standard HC-SR04 needs 5V; use HC-SR04P for 3.3V.";
        } else {
            doc["distance_cm"] = serialized(String(cm, 1));
        }

    } else if (strcmp(activeSensor->id, "rotary") == 0) {
        int clk = digitalRead(groveD);
        if (clk != lastCLK) {
            if (digitalRead(groveD2) != clk) rotarySteps++;
            else                              rotarySteps--;
            lastCLK = clk;
        }
        doc["steps"] = rotarySteps;

    } else if (strcmp(activeSensor->id, "digital_out") == 0 ||
               strcmp(activeSensor->id, "pwm_out") == 0) {
        doc["note"] = "Output sensor — use POST /api/grove/write to set value";
    }

    String result;
    serializeJson(doc, result);
    return result;
}

// ============================================================================
// API endpoints
// ============================================================================

void setupGroveApi() {
    // --- SSE stream ---
    groveEvents = new AsyncEventSource("/api/grove/stream");
    groveEvents->onConnect([](AsyncEventSourceClient* client) {
        client->send("connected", "status", millis(), 1000);
    });
    apiServer.http().addHandler(groveEvents);

    // GET /api/grove/sensors — full catalogue with voltage safety info
    apiServer.http().on("/api/grove/sensors", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        JsonArray arr = doc["sensors"].to<JsonArray>();
        for (int i = 0; i < SENSOR_COUNT; i++) {
            JsonObject o = arr.add<JsonObject>();
            o["id"]           = SENSORS[i].id;
            o["name"]         = SENSORS[i].name;
            o["category"]     = SENSORS[i].category;
            o["vcc"]          = SENSORS[i].vcc;
            o["gpio_safe"]    = SENSORS[i].gpio_safe;
            o["voltage_note"] = SENSORS[i].voltage_note;
            o["uses_d2"]      = SENSORS[i].uses_d2;
            o["d_role"]       = SENSORS[i].d_role;
            if (SENSORS[i].d2_role) o["d2_role"] = SENSORS[i].d2_role;
            o["unit"]         = SENSORS[i].unit;
            if (SENSORS[i].unit2) o["unit2"] = SENSORS[i].unit2;
        }
        String json; serializeJson(doc, json);
        ApiServer::restRequestCount++;
        req->send(200, "application/json", json);
    });

    // GET /api/grove/config — current board, pins, active sensor
    apiServer.http().on("/api/grove/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["board"]     = activeBoardId;
        doc["pin_d"]     = groveD;
        doc["pin_d2"]    = groveD2;
        doc["sensor"]    = activeSensor ? activeSensor->id : "";
        doc["stream_interval_ms"] = streamIntervalMs;
        String json; serializeJson(doc, json);
        ApiServer::restRequestCount++;
        req->send(200, "application/json", json);
    });

    // GET /api/grove/boards — available board profiles
    apiServer.http().on("/api/grove/boards", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        JsonArray arr = doc["boards"].to<JsonArray>();
        for (int i = 0; i < (int)BOARD_PROFILE_COUNT; i++) {
            JsonObject o = arr.add<JsonObject>();
            o["id"]   = BOARD_PROFILES[i].id;
            o["name"] = BOARD_PROFILES[i].name;
            o["d"]    = BOARD_PROFILES[i].d;
            o["d2"]   = BOARD_PROFILES[i].d2;
        }
        String json; serializeJson(doc, json);
        ApiServer::restRequestCount++;
        req->send(200, "application/json", json);
    });

    // POST /api/grove/board — select board profile (or set custom pins)
    apiServer.http().on("/api/grove/board", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON"); return;
            }
            const char* id = doc["board"] | "";
            if (strlen(id) == 0) {
                ApiServer::sendError(req, 400, "Missing 'board' field"); return;
            }

            if (strcmp(id, "custom") == 0) {
                int d  = doc["pin_d"]  | -1;
                int d2 = doc["pin_d2"] | -1;
                if (d < 0 || d > 39 || d2 < 0 || d2 > 39) {
                    ApiServer::sendError(req, 400, "custom board requires pin_d and pin_d2 (0-39)"); return;
                }
                teardownSensor();
                groveD  = d;
                groveD2 = d2;
            } else {
                const BoardProfile* p = findProfile(id);
                if (!p) { ApiServer::sendError(req, 404, "Unknown board profile"); return; }
                teardownSensor();
                groveD  = p->d;
                groveD2 = p->d2;
            }
            strlcpy(activeBoardId, id, sizeof(activeBoardId));

            JsonDocument resp;
            resp["board"]  = activeBoardId;
            resp["pin_d"]  = groveD;
            resp["pin_d2"] = groveD2;
            String json; serializeJson(resp, json);
            ApiServer::restRequestCount++;
            req->send(200, "application/json", json);
        }
    );

    // POST /api/grove/configure — select sensor mode
    apiServer.http().on("/api/grove/configure", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON"); return;
            }
            const char* id = doc["sensor"] | "";
            const SensorDef* s = findSensor(id);
            if (!s) { ApiServer::sendError(req, 404, "Unknown sensor type"); return; }

            if (doc.containsKey("stream_interval_ms")) {
                streamIntervalMs = max((unsigned long)100, (unsigned long)doc["stream_interval_ms"]);
            }
            if (strcmp(id, "pwm_out") == 0 && doc.containsKey("freq")) {
                pwmFreq = doc["freq"] | PWM_FREQ_DEFAULT;
            }

            configureSensor(s);

            JsonDocument resp;
            resp["sensor"]  = s->id;
            resp["name"]    = s->name;
            resp["pin_d"]   = groveD;
            resp["pin_d2"]  = groveD2;
            resp["gpio_safe"] = s->gpio_safe;
            resp["voltage_note"] = s->voltage_note;
            String json; serializeJson(resp, json);
            ApiServer::restRequestCount++;
            req->send(200, "application/json", json);
        }
    );

    // GET /api/grove/read — single reading
    apiServer.http().on("/api/grove/read", HTTP_GET, [](AsyncWebServerRequest* req) {
        ApiServer::restRequestCount++;
        req->send(200, "application/json", takeReading());
    });

    // POST /api/grove/write — set output value (digital_out or pwm_out)
    apiServer.http().on("/api/grove/write", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            if (!activeSensor) {
                ApiServer::sendError(req, 400, "No sensor configured"); return;
            }
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                ApiServer::sendError(req, 400, "Invalid JSON"); return;
            }

            if (strcmp(activeSensor->id, "digital_out") == 0) {
                int v = doc["value"] | 0;
                digitalWrite(groveD, v ? HIGH : LOW);
                ApiServer::restRequestCount++;
                ApiServer::sendOk(req, v ? "HIGH" : "LOW");

            } else if (strcmp(activeSensor->id, "pwm_out") == 0) {
                int duty = doc["duty"] | 0;
                duty = constrain(duty, 0, 255);
                ledcWrite(groveD, duty);
                ApiServer::restRequestCount++;
                JsonDocument resp;
                resp["duty"] = duty;
                String json; serializeJson(resp, json);
                req->send(200, "application/json", json);

            } else {
                ApiServer::sendError(req, 400, "Active sensor is not an output type");
            }
        }
    );

    // POST /api/grove/rotary/reset — zero the rotary encoder count
    apiServer.http().on("/api/grove/rotary/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
        rotarySteps = 0;
        ApiServer::sendOk(req, "reset");
    });

    Serial.println("[API] Grove endpoints registered");
}

// ============================================================================
// Loop — push SSE readings
// ============================================================================

void groveLoop() {
    if (!activeSensor || !groveEvents) return;
    if (groveEvents->count() == 0)     return;  // no clients

    unsigned long now = millis();
    if (now - lastStreamMs < streamIntervalMs) return;
    lastStreamMs = now;

    groveEvents->send(takeReading().c_str(), "reading", now);
}
