# ESP32-C6 WiFi Clock

A WiFi-connected NTP clock for the **Waveshare ESP32-C6-LCD-1.47** board. Shows
the time on the built-in LCD, or a still image loaded from a microSD card. The
onboard RGB LED, screen brightness, orientation and display mode are all
controllable from a built-in web page and a matching HTTP API.

The web page shows a dark-themed gallery: a live clock tile plus a thumbnail of
every image on the SD card, rendered in the browser. Tap a thumbnail to display
it on the LCD (selected item is highlighted), delete one with the ✕ overlay, or
upload a new image straight from the browser — it is resized, cropped and
converted to RGB565 client-side, then streamed to the card. Time and the active
display are pushed to the page over a WebSocket (no polling).

## Hardware

**Board:** [Waveshare ESP32-C6-LCD-1.47](https://docs.waveshare.com/ESP32-C6-LCD-1.47)

| Feature | Detail | GPIO |
|---|---|---|
| LCD | ST7789, 1.47" IPS, 172x320 (used in 320x172 landscape) | MOSI 6, SCLK 7, CS 14, DC 15, RST 21, BL 22 |
| RGB LED | Single WS2812 (addressable) | 8 |
| Button | Onboard BOOT button (active low) | 9 |
| microSD | TF card slot, shares the LCD SPI bus | MISO 5, MOSI 6, SCLK 7, CS 4 |

The microSD card and the LCD share one SPI bus. Because of an ESP32-C6 quirk
(`SPI.setDataMode()`/`setFrequency()` don't latch to hardware, but the SD
library's transactions do), the LCD's SPI config is re-latched via a
`beginTransaction()` pair after every SD access — see `ST7789_Custom::busAcquire()`.

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
- A microSD card formatted **FAT32** (MBR partition table; exFAT is not
  supported by the Arduino `SD` library).

### Libraries to install (Library Manager)

| Library | Used for |
|---|---|
| Adafruit GFX Library | graphics primitives / fonts (the custom LCD driver extends it) |
| Adafruit BusIO | dependency of Adafruit GFX |
| ArduinoJson | WebSocket messages |
| NTPClient | NTP time |
| Time | time keeping (dependency of Timezone) |
| Timezone | local time / DST |
| WebSockets (by Markus Sattler) | WebSocket (WiFi scan, time + active-display push) |
| Adafruit ST7789 | *optional* — only if you enable `USE_ADAFRUIT_ST7789` in `config.h` |

Bundled with the ESP32 core (no install needed): `WiFi`, `WebServer`, `SD`,
`SPI`, `EEPROM`, `WiFiUdp`.

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

| Endpoint | Description | Example |
|---|---|---|
| `GET /api/setled?c=RRGGBB` | Set RGB LED color (6 hex digits) | `/api/setled?c=00ff00` |
| `GET /api/setbl?v=N` | Backlight brightness, `N` = 0..100 (maps to 0..50% hardware duty) | `/api/setbl?v=30` |
| `GET /api/flashbl` | Flash backlight to 100% for 500 ms, then restore | `/api/flashbl` |
| `GET /api/flipscreen` | Rotate the screen 180 degrees | `/api/flipscreen` |
| `GET /api/setdisplay?img=NAME` | Show SD image `NAME`, or `clock` for the clock | `/api/setdisplay?img=eyes1.bin` |
| `GET /api/imagelist` | List `*.bin` images on the SD card | `eyes1.bin\|logo.bin` |
| `GET /getimage?name=NAME` | Raw RGB565 bytes of an image (used by the gallery) | `/getimage?name=eyes1.bin` |
| `POST /upload` | Upload an image (multipart, raw RGB565 `.bin`, must be 320x172) | |
| `GET /delete?name=NAME` | Delete an image from the SD card | `/delete?name=eyes1.bin` |
| `GET /aplist` | Scan and list nearby WiFi networks (blocking scan) | `Home\|Office` |
| `GET /wifisave?s=SSID&p=PASS` | Save WiFi credentials and switch to station mode | `/wifisave?s=Home&p=secret` |
| `GET /` | Main web page | |
| `GET /selectap` | WiFi configuration page | |

The WebSocket server (port 81) carries: `{"APLIST":""}` → network list,
`{"GETDISP":""}` → current display name, and server-pushed `{"TIME":"HH|MM|SS|DD.MM"}`
(once per second) and `{"DISP":"name"}` (on change).

Example:

```bash
curl "http://192.168.4.1/api/setled?c=ff0000"
curl "http://192.168.4.1/api/imagelist"
```

## Displaying images from SD

Images are stored as raw **RGB565, big-endian**, sized to the screen
(**320x172** landscape), with a `.bin` extension, in the SD card root.

Upload them straight from the web page: click **Upload images**, select one or
more files, and the browser resizes/crops each to 320x172, converts it to RGB565
and sends them to the card one at a time. They then appear in the gallery and in
`/imagelist`. Delete any image from the gallery with its ✕ overlay button.

> **Note:** All images are same file size, so deleting an image frees its clusters in the FAT, and the next upload
> reuses that free space — deleting and re-uploading churns the same space, no
> leak or manual cleanup needed.

## Scripts (`tools/`)

| Script | Description |
|---|---|
| `build.sh` | Compile (and optionally `upload`) the firmware with arduino-cli. Honors `FQBN` and `PORT` env vars. |

Image conversion is done in the browser at upload time (see *Displaying images
from SD*), so no offline conversion script is needed.

