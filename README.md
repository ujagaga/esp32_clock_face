# ESP32-C6 WiFi Clock

A WiFi-connected NTP clock for the **Waveshare ESP32-C6-LCD-1.47** board. Shows
the time on the built-in LCD, or a 1-bit monochrome image uploaded to on-chip
flash, drawn in the current LED colour. No SD card. The onboard RGB LED, screen
brightness, orientation and display mode are all controllable from a built-in
web page and a matching HTTP API.

The web page shows a dark-themed gallery: a clock tile plus a thumbnail of every
uploaded image, rendered in the browser. Tap a thumbnail to display it on the
LCD (the active item is highlighted), delete one with the ✕ overlay, or upload a
new image straight from the browser: it is resized, cropped and thresholded to a
1-bpp frame client-side, then stored in flash (LittleFS). The page reads its
state (time, active display) once on load; **reload to refresh** it.

> **Branch note:** the `esp32-c6-websockets` branch implements the same features
> over a WebSocket instead — the clock and active display update live in the
> browser (no reload), giving a nicer UI. The trade-off is higher CPU usage from
> the persistent connection and per-second pushes, which may leave less headroom
> for additional features. This `main` branch uses simple load-time fetches and
> is the lighter-weight option.

## Hardware

**Board:** [Waveshare ESP32-C6-LCD-1.47](https://docs.waveshare.com/ESP32-C6-LCD-1.47)

| Feature | Detail | GPIO |
|---|---|---|
| LCD | ST7789, 1.47" IPS, 172x320 (used in 320x172 landscape) | MOSI 6, SCLK 7, CS 14, DC 15, RST 21, BL 22 |
| RGB LED | Single WS2812 (addressable) | 8 |
| Button | Onboard BOOT button (active low) | 9 |

Images live in on-chip flash (LittleFS), so no microSD card is used.

### Behaviour

- Boots into AP mode and shows the WiFi SSID / password / IP on screen.
- After WiFi is configured it connects, syncs time over NTP and shows the clock.
- Press the BOOT button to force the clock display (switches back from an image).
- Flip the screen 180 degrees from the web page ("Flip Screen").
- Backlight is capped at 50% duty by default (the panel docs warn against
  sustained full brightness); the "Flash" function briefly overrides this.
- Once connected to your WiFi, the device's own AP is switched off to save
  power after 120 s with no AP clients (configurable via `AP_AUTO_OFF_MS` in
  `config.h`), dropping to station-only mode. The timer restarts whenever an AP
  client is connected, and the AP is brought back automatically if the station
  connection is later lost, so the device never becomes unreachable.

## Software requirements

- **ESP32 Arduino core 3.3.10** (board: `esp32:esp32:esp32c6`).

### Libraries to install (Library Manager)

| Library | Used for |
|---|---|
| Adafruit GFX Library | graphics primitives / fonts (the custom LCD driver extends it) |
| Adafruit BusIO | dependency of Adafruit GFX |
| NTPClient | NTP time |
| Time | time keeping (dependency of Timezone) |
| Timezone | local time / DST |
| ArduinoJson | JSON parsing/serialization for the TCP API |
| Adafruit ST7789 | *optional* — only if you enable `USE_ADAFRUIT_ST7789` in `config.h` |

Bundled with the ESP32 core (no install needed): `WiFi`, `WebServer`,
`LittleFS`, `SPI`, `EEPROM`, `WiFiUdp`.

By default the project uses the bundled custom driver `ST7789_Custom.h`, which
handles the 172x320 panel offset and the remappable C6 SPI pins. Define
`USE_ADAFRUIT_ST7789` in `config.h` to use the Adafruit driver instead.

## Building

### Arduino IDE

1. Install the **esp32** boards package (Boards Manager) at version >= 3.3.10.
2. Install the libraries listed above (Library Manager).
3. Select board **ESP32C6 Dev Module**.
4. Set **Tools → Partition Scheme → "Huge APP (3MB No OTA/1MB SPIFFS)"** — the
   firmware is large and is flashed over serial, so OTA/SPIFFS are not needed.
5. Open `esp32_clock_face.ino` and Upload.

### arduino-cli / command line

I developped this on linux, so using shell scripts. On windows you will need to stick to Arduino IDE or build your own cmd scripts.
Requires `arduino-cli` (developed with 1.4.1) with the esp32 core and the
libraries installed. Then:

```bash
tools/build.sh            # compile only
tools/build.sh upload     # compile, then flash
```

`build.sh` defaults to the `huge_app` partition scheme
(`esp32:esp32:esp32c6:PartitionScheme=huge_app`). Overridable via environment:

```bash
FQBN=esp32:esp32:esp32c6:PartitionScheme=huge_app PORT=/dev/ttyACM0 tools/build.sh upload
```

### VS Code (IntelliSense)

The build writes a `compile_commands.json` into `.cache/`, and
`.vscode/c_cpp_properties.json` points IntelliSense at it plus the full include
path. To use it:

1. Install the **C/C++** extension (ms-vscode.cpptools).
2. Run `tools/build.sh` at least once so `.cache/compile_commands.json` exists.
3. Reload the window (**Developer: Reload Window**).

The include paths are pinned to esp32 core 3.3.10. If you upgrade the core or
add a library, rebuild and regenerate `.vscode/c_cpp_properties.json`. The
build is the source of truth — VS Code is only for editing.

## HTTP API

Base URL is the device IP (port 80). Most endpoints are **HTTP GET**; image
upload is **POST**. Action endpoints return `OK` (or `400` on bad input); list
endpoints return a `|`-separated string.

### Automation API

Stable `/api/*` endpoints, intended for scripts and home automation.

| Endpoint | Description | Example |
|---|---|---|
| `GET /api/setled?c=RRGGBB` | Set RGB LED color (6 hex digits) | `/api/setled?c=00ff00` |
| `GET /api/setbl?v=N` | Backlight brightness, `N` = 0..100 (maps to 0..50% hardware duty) | `/api/setbl?v=30` |
| `GET /api/flashbl` | Flash backlight to 100% for 500 ms, then restore | `/api/flashbl` |
| `GET /api/flipscreen` | Rotate the screen 180 degrees | `/api/flipscreen` |
| `GET /api/setdisplay?img=NAME` | Show uploaded image `NAME`, or `clock` | `/api/setdisplay?img=logo.1bpp` |
| `GET /api/imagelist` | List uploaded images | `logo.1bpp\|cat.1bpp` |

### Web UI & config

Used by the built-in web pages; not part of the automation surface.

| Endpoint | Description | Example |
|---|---|---|
| `GET /getimage?name=NAME` | Raw 1-bpp bytes of an image (used by the gallery) | `/getimage?name=logo.1bpp` |
| `POST /upload` | Upload a 1-bpp frame to flash (multipart, 320x172, 6880 bytes) | |
| `GET /delete?name=NAME` | Delete an uploaded image from flash | `/delete?name=logo.1bpp` |
| `GET /getdisplay` | Active display name (`clock` or an image) | `eyes1.bin` |
| `GET /gettime` | Current time `HH\|MM\|SS\|DD.MM` (empty until NTP-synced) | `14\|05\|23\|28.06` |
| `GET /aplist` | Result of the last WiFi scan (empty until ready) | `Home\|Office` |
| `GET /wifisave?s=SSID&p=PASS` | Save WiFi credentials and switch to station mode | `/wifisave?s=Home&p=secret` |
| `GET /` | Main web page | |
| `GET /selectap` | WiFi configuration page (starts an async scan on load) | |
| `GET /api` | API documentation page | |

WiFi scanning is non-blocking: serving `/selectap` kicks off a scan, and the page
fetches `/aplist` ~10 s later to populate the list (with a couple of retries).

Example:

```bash
curl "http://192.168.4.1/api/setled?c=ff0000"
curl "http://192.168.4.1/api/imagelist"
```

## TCP API (JSON)

A line-based JSON API runs on TCP port **333** (`TCP_API_PORT` in `config.h`),
in parallel with the HTTP server. Send one JSON object per line (`\n`-terminated);
the device replies with one JSON object per line. One client at a time.

| Command | Reply |
|---|---|
| `{"cmd":"list"}` | `{"images":"logo.1bpp\|cat.1bpp"}` |
| `{"cmd":"getdisplay"}` | `{"display":"clock"}` |
| `{"cmd":"setdisplay","img":"logo.1bpp"}` | `{"ok":true}` (use `"clock"` or omit `img` for the clock) |
| `{"cmd":"gettime"}` | `{"time":"HH\|MM\|SS\|DD.MM"}` (empty until NTP-synced) |
| `{"cmd":"setled","c":"00ff00"}` | `{"ok":true}` (6 hex digits) |
| `{"cmd":"setbl","v":50}` | `{"ok":true}` (0..100) |
| `{"cmd":"flashbl"}` | `{"ok":true}` |
| `{"cmd":"flipscreen"}` | `{"ok":true}` |
| `{"cmd":"httpserver","enable":false}` | `{"ok":true,"running":false}` — stop the HTTP server to save CPU; **refused** (`{"ok":false,"error":"http client connected","running":true}`) while a browser is connected. `"enable":true` restarts it; omit `enable` to just query `{"running":bool}`. |

```bash
printf '{"cmd":"list"}\n'                    | nc 192.168.4.1 333
printf '{"cmd":"setdisplay","img":"logo.1bpp"}\n' | nc 192.168.4.1 333
printf '{"cmd":"httpserver","enable":false}\n' | nc 192.168.4.1 333
```

Or use the bundled Python client (`tools/tcp_api.py`):

```bash
tools/tcp_api.py 192.168.4.1 list
tools/tcp_api.py 192.168.4.1 setdisplay logo.1bpp
tools/tcp_api.py 192.168.4.1 setled 00ff00
tools/tcp_api.py 192.168.4.1 httpserver off
```

## Images

Everything shown (other than the clock) is a full-screen **320x172 1-bit
monochrome** frame, drawn in the current LED colour on black, stored in on-chip
flash (LittleFS). Each frame is exactly `320 * 172 / 8 = 6880` bytes; the ~1 MB
filesystem holds ~150 of them.

Upload from the web page: click **Upload images**, pick one or more files, and
the browser scales/crops each to 320x172, thresholds it to 1-bpp and stores it
in flash (extension forced to `.1bpp`). They appear in the gallery and in
`/api/imagelist`. Delete one with its X overlay button.

The LED colour doubles as the image colour — set it from the web page or
`/api/setled`. If the LED is off (black), images fall back to white so they stay
visible.

## Scripts (`tools/`)

| Script | Description |
|---|---|
| `build.sh` | Compile (and optionally `upload`) the firmware with arduino-cli. Honors `FQBN` and `PORT` env vars. |
| `tcp_api.py` | Command-line client for the JSON-over-TCP API (`tcp_api.py <host> <command>`; `-h` for the command list). |

Image conversion for uploads is done in the browser, so no offline conversion
script is needed.

