# Cardputer ADV

M5Stack Cardputer firmware + browser PWA that turns the Cardputer into a tethered hardware peripheral. The firmware creates a WiFi access point and exposes every hardware feature through a clean JSON API (HTTP REST + WebSocket streaming + SSE). The PWA is the reference client — connect any phone, tablet or laptop and control everything through a browser.

```
firmware/
  cardputer_webapi/   ← Arduino sketch (ESP32-S3, M5Stack Cardputer)
  install-libraries.bat  ← Dependency installer for contributors (arduino-cli)
pwa/                  ← Browser PWA reference client
```

---

## Hardware

- [M5Stack Cardputer](https://shop.m5stack.com/products/m5stack-cardputer-kit-w-m5stamps3) (M5StampS3, ESP32-S3)
- Optional: [Cap LoRa 1262 Unit](https://shop.m5stack.com/products/lora-unit-1262-868-915mhz) — adds GPS + LoRa radio
- Optional: Any Grove-compatible sensor on the Cardputer's Grove Port A (yellow socket on the body)

---

## Installing

### Option A — Pre-built binary (easiest)

Download the latest `.bin` from the [Releases page](../../releases) and flash it:

- **M5Launcher users:** copy the `.bin` to your SD card and install from the launcher menu — done.
- **First time / no launcher:** use [M5Burner](https://docs.m5stack.com/en/download) (GUI) or esptool:
  ```
  esptool.py --port COM3 write_flash 0x0 cardputer_webapi_v1.3.0.merged.bin
  ```
  Replace `COM3` with your port (`/dev/ttyUSB0` on Linux/Mac).

### Option B — Build from source (Arduino IDE)

Standard Arduino workflow — no extra tools needed beyond the IDE itself.

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)

2. Add the M5Stack board package — open **File → Preferences**, add this URL to *Additional boards manager URLs*:
   ```
   https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
   ```
   Then **Tools → Board → Boards Manager**, search `M5Stack`, install.

3. Install libraries — **Tools → Manage Libraries**, search and install each:
   - `M5Cardputer`
   - `ArduinoJson` (install v7)
   - `TinyGPSPlus`
   - `RadioLib`
   - `IRremote`
   - `DHTesp`
   - `OneWire`
   - `DallasTemperature`

4. Install the two libraries not in the Library Manager — download as zips and use **Sketch → Include Library → Add .ZIP Library**:
   - [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer/archive/refs/heads/master.zip)
   - [AsyncTCP](https://github.com/me-no-dev/AsyncTCP/archive/refs/heads/master.zip)

5. Open `firmware/cardputer_webapi/cardputer_webapi.ino`, select **Tools → Board → M5Stack → M5Stack-Cardputer**, set **Partition Scheme → No OTA**, and click **Upload**.

### Option C — arduino-cli (contributors / CI)

```bat
firmware\install-libraries.bat   ← installs everything in one shot
arduino-cli compile --profile default firmware\cardputer_webapi
```

**Partition scheme:** `no_ota` — compatible with [bmorcelli M5Launcher](https://github.com/bmorcelli/M5Stick-Launcher). Binary fits at ~1.4 MB (well within the launcher's 4.9 MB app partition).

### Dependencies

| Library | Source | Purpose |
|---------|--------|---------|
| M5Cardputer | M5Stack board package | Display, keyboard, IMU, speaker |
| ESPAsyncWebServer | [GitHub (me-no-dev)](https://github.com/me-no-dev/ESPAsyncWebServer) | Async HTTP + WebSocket + SSE server |
| AsyncTCP | [GitHub (me-no-dev)](https://github.com/me-no-dev/AsyncTCP) | Async TCP layer for ESPAsyncWebServer |
| ArduinoJson v7 | Arduino Library Manager | JSON serialisation |
| TinyGPSPlus | Arduino Library Manager | NMEA sentence parsing |
| RadioLib | Arduino Library Manager | SX1262 LoRa radio driver |
| IRremote | Arduino Library Manager | IR signal transmission (TinyIRSender) |
| DHTesp | Arduino Library Manager | Grove: DHT11 temperature + humidity |
| OneWire | Arduino Library Manager | Grove: DS18B20 1-Wire bus |
| DallasTemperature | Arduino Library Manager | Grove: DS18B20 temperature sensor |

---

## Hardware Exposed

| Module | Transport | Capability |
|--------|-----------|------------|
| GPS (ATGM336H) | REST + WebSocket | Position, altitude, speed, satellites, fix time |
| LoRa (SX1262) | REST + WebSocket | Configurable radio TX/RX — frequency, BW, SF, power |
| IR Transmitter | REST | NEC / NEC Extended / Onkyo protocols |
| IMU (BMI270) | REST + WebSocket | 3-axis accelerometer + 3-axis gyroscope |
| Keyboard | WebSocket | Key events with full modifier state |
| Display (ST7789) | REST | Text, filled rectangles, clear, colour control |
| Speaker (ES8311) | REST | Tone generation, volume control |
| GPIO | REST | Digital read/write, pin mode configuration |
| **Grove Port A** | **REST + SSE** | **Pluggable sensor interface — DHT11, DS18B20, HC-SR04, rotary encoder, digital/analog/PWM** |
| System | REST | Chip model, memory, flash, WiFi clients, uptime |

---

## API Reference

Live clickable listing: `http://192.168.4.1/info`

### REST Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/version` | Firmware name and version |
| GET | `/api/system/info` | Chip, memory, flash, WiFi, uptime |
| GET | `/api/gps` | Current GPS fix |
| POST | `/api/gps/rate` | Set NMEA update rate |
| GET | `/api/imu` | Current IMU reading (accel + gyro) |
| POST | `/api/imu/rate` | Set IMU streaming interval |
| GET | `/api/lora/config` | Current LoRa radio configuration |
| POST | `/api/lora/config` | Set frequency, bandwidth, SF, TX power |
| POST | `/api/lora/send` | Transmit a LoRa packet |
| POST | `/api/ir/send` | Send IR command (NEC / NEC Extended / Onkyo) |
| GET | `/api/gpio/{pin}` | Read digital pin state |
| POST | `/api/gpio/{pin}` | Write digital pin state |
| POST | `/api/gpio/{pin}/mode` | Set pin mode (INPUT / OUTPUT / INPUT_PULLUP / INPUT_PULLDOWN) |
| POST | `/api/display/text` | Draw text at x/y with colour and size |
| POST | `/api/display/clear` | Clear display to colour |
| POST | `/api/display/fill` | Draw filled rectangle |
| POST | `/api/audio/tone` | Play tone at frequency/duration |
| POST | `/api/audio/volume` | Set speaker volume (0–255) |
| POST | `/api/audio/stop` | Stop audio playback |
| GET | `/api/grove/sensors` | Sensor catalogue with voltage safety info |
| GET | `/api/grove/config` | Current pins and active sensor |
| POST | `/api/grove/configure` | Arm a sensor type on the Grove pins |
| GET | `/api/grove/read` | Single reading from the active sensor |
| POST | `/api/grove/write` | Write output value (digital\_out or pwm\_out) |
| POST | `/api/grove/rotary/reset` | Zero the rotary encoder step count |

### WebSocket Streams

| Endpoint | Data |
|----------|------|
| `ws://192.168.4.1/ws/gps` | GPS position updates |
| `ws://192.168.4.1/ws/imu` | IMU accelerometer + gyro at configurable rate |
| `ws://192.168.4.1/ws/lora` | Received LoRa packets |
| `ws://192.168.4.1/ws/keyboard` | Key press events with modifier state |

### SSE Streams

| Endpoint | Data |
|----------|------|
| `http://192.168.4.1/api/grove/stream` | Grove sensor readings at configurable interval (default 500 ms) |

### Grove Sensor Types

| Sensor ID | Name | VCC | GPIO Safe | Notes |
|-----------|------|-----|-----------|-------|
| `digital_in` | Digital Input | 3.3V | ✓ | Signal on D pin |
| `digital_out` | Digital Output | 3.3V | ✓ | D drives HIGH/LOW |
| `analog_in` | Analog Input | 3.3V | ✓ | 12-bit ADC, 0–3.3V |
| `pwm_out` | PWM Output | 3.3V | ✓ | 8-bit duty, configurable frequency |
| `dht11` | DHT11 Temp & Humidity | 3.3V–5V | ✓ | Data on D |
| `ds18b20` | DS18B20 Temperature | 3.0V–5.5V | ✓ | 1-Wire on D; needs 4.7 kΩ pull-up |
| `hcsr04` | HC-SR04 Ultrasonic | **5V** | ⚠ | Use HC-SR04P (3.3V) or voltage divider on Echo |
| `rotary` | Rotary Encoder | 3.3V–5V | ✓ | CLK on D, DT on D2 |

Grove Port A on the Cardputer: **D = GPIO1** (yellow wire), **D2 = GPIO2** (white wire).

---

## Architecture

`firmware/cardputer_webapi/` — Arduino sketch, one source file per hardware module:

| File | Module |
|------|--------|
| `api_gps.*` | GPS UART parsing + WebSocket stream |
| `api_lora.*` | SX1262 LoRa TX/RX + WebSocket stream |
| `api_ir.*` | IR transmitter (TinyIRSender) |
| `api_imu.*` | BMI270 IMU + WebSocket stream |
| `api_audio.*` | ES8311 speaker tone + volume |
| `api_display.*` | ST7789 display drawing |
| `api_gpio.*` | Raw GPIO read/write/mode |
| `api_grove.*` | Grove Port A — pluggable sensor drivers + SSE stream |
| `api_system.*` | Chip/memory/WiFi system info |
| `api_serial.*` | USB serial JSON command interface (mirrors HTTP API) |
| `api_server.*` | ESPAsyncWebServer routing, REST + WebSocket helpers |
| `api_webapp.*` | All PWA files embedded as C string literals; serves at `/` |
| `config.h` | WiFi AP credentials, pin assignments, version |
| `sketch.yaml` | arduino-cli build profile + library list |

`pwa/` — vanilla JavaScript, ES6 modules, no build step:

| File | Tab |
|------|-----|
| `js/app.js` | Tab router + WiFi connect/disconnect |
| `js/api.js` | `CardputerAPI` class (fetch + WebSocket wrapper) |
| `js/tab-system.js` | System info |
| `js/tab-gps.js` | GPS trail map |
| `js/tab-lora.js` | LoRa TX/RX |
| `js/tab-imu.js` | IMU visualisation |
| `js/tab-keyboard.js` | Keyboard monitor |
| `js/tab-ir.js` | IR remote |
| `js/tab-display.js` | Display drawing |
| `js/tab-gpio.js` | GPIO control |
| `js/tab-audio.js` | Tone generator |
| `js/tab-grove.js` | Grove sensor explorer |
| `js/tab-record.js` | Microphone recorder |

The PWA is embedded in the firmware via `api_webapp.cpp` — no LittleFS upload or separate server needed. `pwa/` is the source of truth; after editing run the sketch through the build to regenerate the embedded version.

---

## PWA Notes

### Accessing the app

Open `http://192.168.4.1/` while connected to the `CardputerADV` AP, or open `pwa/index.html` from a local HTTP server and enter the IP manually.

### Microphone recording

Browsers block microphone access over plain HTTP. To enable it on the Cardputer AP:

- **Chrome / Android:** `chrome://flags/#unsafely-treat-insecure-origin-as-secure` → add `http://192.168.4.1`
- **Firefox:** `about:config` → `media.devices.insecure.enabled = true`

The Record tab shows these instructions when it detects an insecure context.

---

## Current State

**v1.3.0** — 10 hardware modules, 11 PWA tabs, full Grove sensor support added.

- All hardware modules implemented and tested
- Web app embedded in firmware — flash once, no separate upload step
- Grove Port A: sensor catalogue, SSE streaming, voltage safety warnings
- Display shows a live status dashboard
- Managed via [bmorcelli M5Launcher](https://github.com/bmorcelli/M5Stick-Launcher)

## Roadmap

- Additional Grove I2C sensor drivers (SHT31, BMP280, colour sensor, etc.)
- Power management / sleep modes
- Purpose-built native apps (Android via PoPA, desktop Python scripts) consuming the API
- Possible integration into the Home PWA catalogue
