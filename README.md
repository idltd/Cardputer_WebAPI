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

### Architecture

`firmware/cardputer_webapi/` — Arduino sketch + C++ source, one file per hardware module:
- `api_gps.*`, `api_lora.*`, `api_ir.*`, `api_imu.*`, `api_audio.*`, `api_display.*`, `api_gpio.*`, `api_serial.*`, `api_system.*`
- `api_server.*` — ESPAsyncWebServer routing REST + WebSocket
- `config.h` — WiFi AP credentials, server port, version

### Building

Uses **arduino-cli** with the M5Stack board package:

```bat
arduino-cli compile ^
  --fqbn "m5stack:esp32:m5stack_cardputer:PartitionScheme=no_ota" ^
  --build-path "firmware/build/m5stack.esp32.m5stack_cardputer" ^
  firmware/cardputer_webapi
```

Required libraries: M5Cardputer, ESPAsyncWebServer, AsyncTCP, ArduinoJson (v7), TinyGPSPlus, RadioLib, IRremote, LittleFS

**Partition scheme:** `no_ota` (2MB app limit) — compatible with the bmorcelli M5Launcher, which uses its own custom partition table on the device and installs only the app binary. The compiled binary fits comfortably at ~1.4MB (65% of the 2MB compile limit; 29% of the launcher's 4.9MB app partition).

**Output binary:** `firmware/build/m5stack.esp32.m5stack_cardputer/cardputer_webapi_vX.Y.Z.bin` — this is the file to give to the launcher.

### Installing

- **Via M5Launcher (bmorcelli):** copy `cardputer_webapi_vX.Y.Z.bin` to the SD card and install from the launcher menu.
- **Bare USB flash (first time / no launcher):** use `cardputer_webapi.ino.merged.bin` with esptool at offset 0x0.

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
3. Open `pwa/index.html` in a browser (or serve it from any HTTP server)
   — or navigate to `http://192.168.4.1/app/` if LittleFS data has been uploaded (see below)
4. Enter the Cardputer's IP (default: `192.168.4.1`) and click Connect
   — if the app is served from the Cardputer itself, it auto-connects

### Serving from the Cardputer (LittleFS)

The firmware can serve the web app directly from its flash filesystem, so any phone on the WiFi AP can open it in a browser without needing a separate server.

**One-time setup:**
1. Run `firmware/cardputer_webapi/prepare-data.bat` — copies `pwa/` into `firmware/cardputer_webapi/data/`
2. In Arduino IDE: **Tools → ESP32 Sketch Data Upload** (requires the [arduino-esp32fs-plugin](https://github.com/lorol/arduino-esp32fs-plugin) or equivalent LittleFS uploader)
3. Navigate to `http://192.168.4.1/app/` — the app loads and auto-connects

`http://192.168.4.1/` shows the API listing with a prominent link to the app, or an instruction to upload the data if it hasn't been done. Re-run `prepare-data.bat` and re-upload whenever the PWA changes.

---

## Current State

v1.1.0 — firmware and reference client working. All 9 hardware modules implemented. LittleFS app serving added; 10th tab (microphone recording) added to PWA.

Installed and managed via the [bmorcelli M5Launcher](https://github.com/bmorcelli/M5Stick-Launcher).

## Where It's Heading

- Additional sensor modules as hardware is tested
- Power management / sleep modes
- Purpose-built native apps (Android via PoPA, desktop scripts) for specific use cases once the API is proven stable
- Possible integration into the Home catalogue for easy launch
