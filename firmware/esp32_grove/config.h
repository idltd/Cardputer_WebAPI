#pragma once

// ============================================================================
// ESP32 Grove Sensor Explorer — Configuration
// ============================================================================

// --- WiFi Access Point ---
#define WIFI_AP_SSID       "GroveExplorer"
#define WIFI_AP_PASSWORD   "grovedemo"    // min 8 chars; empty string = open AP
#define WIFI_AP_CHANNEL    6
#define WIFI_AP_MAX_CONN   4

// --- HTTP Server ---
#define HTTP_PORT          80

// --- Firmware Version ---
#define FIRMWARE_VERSION   "1.0.0"

// --- mDNS ---
#define MDNS_HOSTNAME      "grove"

// --- Grove Port Pin Defaults ---
// IMPORTANT: Must be ADC1 pins (GPIO32-39) — ADC2 pins conflict with WiFi.
// D is the primary data pin. D2 is used by sensors needing two signals
// (HC-SR04 trigger/echo, rotary encoder CLK/DT).
//
// Override at runtime via POST /api/grove/board, or change defaults here.
#define GROVE_DEFAULT_D    32
#define GROVE_DEFAULT_D2   33

// --- Board Pin Profiles ---
// Each profile maps to a Grove-style connector or equivalent header pins.
// Choosing a profile at runtime re-maps D/D2 without reflashing.
//
// All profiles must use ADC1-capable pins for analog sensor support.
// ADC2 (GPIO0,2,4,12-15,25-27) is disabled while WiFi is active.
struct BoardProfile {
    const char* id;
    const char* name;
    int d;    // primary data pin
    int d2;   // secondary data pin
};

static const BoardProfile BOARD_PROFILES[] = {
    // id              name                        D    D2
    { "devkitc",       "ESP32 DevKit C (default)", 32,  33  },
    { "nodemcu32s",    "NodeMCU-32S",              32,  33  },
    { "d1mini32",      "Wemos D1 Mini32",          32,  33  },
    { "lolin32",       "LOLIN32 / LOLIN32 Lite",   32,  33  },
    { "custom",        "Custom pins",              32,  33  },  // mutable
};
#define BOARD_PROFILE_COUNT (sizeof(BOARD_PROFILES) / sizeof(BOARD_PROFILES[0]))
#define BOARD_DEFAULT_ID    "devkitc"
