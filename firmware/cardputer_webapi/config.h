#pragma once

// ============================================================================
// Cardputer ADV API Server — Configuration
// ============================================================================

// --- WiFi Access Point ---
#define WIFI_AP_SSID       "CardputerADV"
#define WIFI_AP_PASSWORD   "cardputer"   // min 8 chars; empty string = open AP
#define WIFI_AP_CHANNEL    1
#define WIFI_AP_MAX_CONN   4

// --- HTTP Server ---
#define HTTP_PORT          80
#define WS_MAX_CLIENTS     4

// --- GPS (ATGM336H via UART on Cap LoRa-1262) ---
#define GPS_UART_NUM       1
#define GPS_TX_PIN         13   // Cardputer → GPS
#define GPS_RX_PIN         15   // GPS → Cardputer
#define GPS_BAUD           115200

// --- LoRa (SX1262 via SPI on Cap LoRa-1262 for Cardputer ADV) ---
#define LORA_CS_PIN        5
#define LORA_MOSI_PIN      14
#define LORA_MISO_PIN      39
#define LORA_SCK_PIN       40
#define LORA_RST_PIN       3
#define LORA_BUSY_PIN      6
#define LORA_DIO1_PIN      4

// LoRa defaults
#define LORA_DEFAULT_FREQ      915.0   // MHz (US ISM band)
#define LORA_DEFAULT_BW        125.0   // kHz
#define LORA_DEFAULT_SF        7       // Spreading factor (7–12)
#define LORA_DEFAULT_CR        5       // Coding rate (5–8 = 4/5–4/8)
#define LORA_DEFAULT_SYNC      0x12    // Sync word (0x12 = private)
#define LORA_DEFAULT_POWER     10      // dBm (max 22 for SX1262)
#define LORA_DEFAULT_PREAMBLE  8

// --- IR ---
#define IR_SEND_PIN        44

// --- IMU (BMI270 via I2C, handled by M5Unified) ---
#define IMU_DEFAULT_RATE_MS  100   // ms between IMU WebSocket pushes

// --- Audio (ES8311 via I2C + I2S, handled by M5Unified) ---
#define AUDIO_DEFAULT_VOLUME  128  // 0–255

// --- Keyboard ---
// I2C on GPIO8 (SDA), GPIO9 (SCL), interrupt on GPIO11
// Handled internally by M5Cardputer library

// --- Display ---
// SPI, handled by M5GFX/M5Unified
#define DISPLAY_STATUS_UPDATE_MS  2000  // Status screen refresh interval

// --- API Authentication (Phase 4) ---
#define API_AUTH_ENABLED   false
#define API_KEY            "changeme"

// --- GPIO Safety ---
// Pins reserved by other modules — GPIO API will reject these
// GPS: 13 (TX), 15 (RX)
// LoRa SPI: 14 (MOSI), 5 (CS), 39 (MISO), 40 (SCK), 4 (DIO1), 6 (BUSY), 3 (RST)
// IR: 44
// I2C (keyboard/IMU): 8 (SDA), 9 (SCL)
// Keyboard interrupt: 11
#define GPIO_MAX_PIN       48
static const int RESERVED_PINS[] = {
    GPS_TX_PIN, GPS_RX_PIN,
    LORA_MOSI_PIN, LORA_CS_PIN, LORA_MISO_PIN, LORA_SCK_PIN,
    LORA_DIO1_PIN, LORA_BUSY_PIN, LORA_RST_PIN,
    IR_SEND_PIN,
    8, 9,  // I2C
    11     // TCA8418 keyboard interrupt
};
#define RESERVED_PIN_COUNT (sizeof(RESERVED_PINS) / sizeof(RESERVED_PINS[0]))

// --- Firmware Version ---
#define FIRMWARE_VERSION   "1.1.0"

// --- mDNS ---
#define MDNS_HOSTNAME      "cardputer"
