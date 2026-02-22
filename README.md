# Cardputer ADV

M5Stack Cardputer firmware + browser PWA reference client. The firmware turns the Cardputer into a tethered peripheral, exposing all its hardware via a WiFi JSON API. The PWA is the reference client that proves the API.

```
Hardware/cardputer/
  firmware/    ← Arduino sketch (ESP32-S3, M5Stack Cardputer)
  pwa/         ← Browser PWA reference client
```

---

## Firmware (`firmware/`)

### What It Does

**Cardputer ADV turns the Cardputer into a tethered peripheral.** It creates a WiFi access point and serves every hardware feature through a clean JSON API (HTTP REST + WebSocket streaming). Any device on the network — phone, tablet, laptop, Raspberry Pi — can control the hardware through a bigger screen.

### Hardware Exposed

| Module | Interface | Capability |
|--------|-----------|------------|
| GPS (AT6668) | REST + WebSocket | Position, altitude, speed, satellites, time |
| LoRa (SX1262) | REST + WebSocket | Configurable radio TX/RX at 915 MHz |
| IR Transmitter | REST | NEC / NEC Extended / Onkyo protocols |
| IMU (BMI270) | REST + WebSocket | 3-axis accelerometer + 3-axis gyroscope |
| Keyboard | WebSocket | Key events with modifier state |
| Display (ST7789) | REST | Text, rectangles, clear, colour control |
| Speaker (ES8311) | REST | Tone generation, volume control |
| GPIO | REST | Digital read/write, pin mode configuration |
| System | REST | Chip info, memory, uptime, WiFi clients |

### Architecture

`firmware/` — Arduino sketch + C++ source, one file per hardware module:
- `api_gps.*`, `api_lora.*`, `api_ir.*`, `api_imu.*`, `api_audio.*`, `api_display.*`, `api_gpio.*`, `api_serial.*`, `api_system.*`
- `api_server.*` — ESPAsyncWebServer routing REST + WebSocket
- `config.h` — WiFi AP credentials, server port

### Building

Requires Arduino IDE or CLI with:
- Board: M5Stack Cardputer (ESP32-S3)
- Libraries: M5Cardputer, ESPAsyncWebServer, AsyncTCP, ArduinoJson, TinyGPSPlus, RadioLib, IRremoteESP8266

Flash via USB first time; subsequent updates via OTA.

---

## PWA Reference Client (`pwa/`)

### What It Does

A browser PWA that controls every hardware feature of the Cardputer over WiFi. Proves the firmware API is stable before building native apps for specific use cases.

9 hardware tabs: System info, GPS (trail map), LoRa TX/RX, IMU visualisation, Keyboard monitoring, IR remote, Display drawing, GPIO control, Audio tone generation.

### Tech Stack

Vanilla JavaScript (ES6 modules), no build step, no dependencies. PWA with service worker for offline support.

### How to Use

1. Flash the firmware to your M5Stack Cardputer
2. Connect to `CardputerADV` WiFi AP (password: `cardputer`)
3. Open `pwa/index.html` in a browser (or serve it from any HTTP server)
4. Enter the Cardputer's IP (default: `192.168.4.1`) and click Connect

---

## Current State

Working firmware and reference client. All 9 hardware modules implemented and tested.

## Where It's Heading

- Additional sensor modules as hardware is tested
- Power management / sleep modes
- Purpose-built native apps (Android via PoPA, desktop scripts) for specific use cases once the API is proven stable
- Possible integration into the Home catalogue for easy launch
