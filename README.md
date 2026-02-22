# Cardputer ADV

M5Stack Cardputer firmware + browser PWA reference client. The firmware turns the Cardputer into a tethered peripheral, exposing all its hardware via a WiFi JSON API. The PWA is the reference client that proves the API.

```
Hardware/cardputer/
  firmware/
    cardputer_webapi/   ← Arduino sketch (ESP32-S3, M5Stack Cardputer)
    build/              ← Compiled binaries (gitignored except this note)
  pwa/                  ← Browser PWA reference client
```

---

## Firmware (`firmware/cardputer_webapi/`)

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

### Building

Uses **arduino-cli** with the M5Stack board package:

```bat
arduino-cli compile ^
  --fqbn "m5stack:esp32:m5stack_cardputer:PartitionScheme=no_ota" ^
  --build-path "firmware/build/m5stack.esp32.m5stack_cardputer" ^
  firmware/cardputer_webapi
```

Required libraries: M5Cardputer, ESPAsyncWebServer, AsyncTCP, ArduinoJson (v7), TinyGPSPlus, RadioLib, IRremote

**Partition scheme:** `no_ota` (2MB app limit) — compatible with the bmorcelli M5Launcher, which uses its own custom partition table on the device and installs only the app binary. The compiled binary fits comfortably at ~1.4MB (65% of the 2MB compile limit; 29% of the launcher's 4.9MB app partition).

**Output binary:** `firmware/build/m5stack.esp32.m5stack_cardputer/cardputer_webapi_vX.Y.Z.bin` — this is the file to give to the launcher.

### Installing

- **Via M5Launcher (bmorcelli):** copy `cardputer_webapi_vX.Y.Z.bin` to the SD card and install from the launcher menu.
- **Bare USB flash (first time / no launcher):** use `cardputer_webapi.ino.merged.bin` with esptool at offset 0x0.

### Architecture

`firmware/cardputer_webapi/` — Arduino sketch + C++ source, one file per hardware module:
- `api_gps.*`, `api_lora.*`, `api_ir.*`, `api_imu.*`, `api_audio.*`, `api_display.*`, `api_gpio.*`, `api_serial.*`, `api_system.*`
- `api_server.*` — ESPAsyncWebServer routing REST + WebSocket
- `api_webapp.*` — all PWA files embedded as C string literals; serves the web app at `/`
- `config.h` — WiFi AP credentials, server port, version

---

## PWA Reference Client (`pwa/`)

### What It Does

A browser PWA that controls every hardware feature of the Cardputer over WiFi. Proves the firmware API is stable before building native apps for specific use cases.

10 tabs: System info, GPS (trail map), LoRa TX/RX, IMU visualisation, Keyboard monitoring, IR remote, Display drawing, GPIO control, Audio tone generation, Microphone recording.

### Tech Stack

Vanilla JavaScript (ES6 modules), no build step, no dependencies. PWA with service worker for offline support.

### How to Use

1. Flash the firmware to your M5Stack Cardputer
2. Connect to `CardputerADV` WiFi AP (password: `cardputer`)
   > **Phone users:** disable mobile data first — mobile data takes priority over WiFi networks that have no internet, so requests to the Cardputer will be silently routed away and fail.
3. Navigate to `http://192.168.4.1/` — the web app is served directly from the firmware, no separate server needed
4. The app auto-connects to the Cardputer when opened from the device

Alternatively, open `pwa/index.html` from the local filesystem or any HTTP server and enter the Cardputer's IP (`192.168.4.1`) manually.

The API endpoint listing (for development/debugging) is at `http://192.168.4.1/info`.

### Microphone Recording

The browser blocks microphone access over plain HTTP. To enable recording when connected to the Cardputer AP:

- **Chrome/Android:** open `chrome://flags/#unsafely-treat-insecure-origin-as-secure`, add `http://192.168.4.1`, relaunch Chrome
- **Firefox:** open `about:config`, set `media.devices.insecure.enabled` to `true`

The Record tab shows these instructions automatically when it detects an insecure context.

---

## Current State

v1.2.0 — firmware and reference client working. All 9 hardware modules implemented across 10 PWA tabs. Web app embedded directly in firmware (no LittleFS upload step needed). Display shows a live dashboard. Managed via the [bmorcelli M5Launcher](https://github.com/bmorcelli/M5Stick-Launcher).

## Where It's Heading

- Additional sensor modules as hardware is tested
- Power management / sleep modes
- Purpose-built native apps (Android via PoPA, desktop scripts) for specific use cases once the API is proven stable
- Possible integration into the Home catalogue for easy launch
