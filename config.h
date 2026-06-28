#ifndef CONFIG_H
#define CONFIG_H

// Target board: Waveshare ESP32-C6-LCD-1.47 (ST7789, 172x320 IPS).
// Uncomment to use the Adafruit driver instead of the bundled custom one.
// The custom driver (ST7789_Custom.h) handles the 172x320 offsets and the
// remappable ESP32-C6 SPI pins below.
// #define USE_ADAFRUIT_ST7789

#define SCREEN_W  172
#define SCREEN_H  320

#define AP_NAME_PREFIX          "LcdClk_"         // Will be appended by device MAC
#define AP_PASS                 "pass1234"

// JSON-over-TCP API server port (one line of JSON per command, see tcp_server.cpp).
#define TCP_API_PORT            (333)

// Once the station is connected, turn the AP off this long after the last AP
// client disconnects (saves power by dropping to station-only mode). The AP is
// brought back automatically if the station connection is later lost.
#define AP_AUTO_OFF_MS          (120000)

#define WIFI_PASS_EEPROM_ADDR   (0)
#define WIFI_PASS_SIZE          (32)
#define SSID_EEPROM_ADDR        (WIFI_PASS_EEPROM_ADDR + WIFI_PASS_SIZE)
#define SSID_SIZE               (32)
#define EEPROM_SIZE             (WIFI_PASS_SIZE + SSID_SIZE)

// ESP32-C6-LCD-1.47 display pins (Waveshare wiki)
#define TFT_MOSI  6
#define TFT_SCLK  7
#define TFT_CS    14
#define TFT_DC    15
#define TFT_RST   21
#define TFT_BL    22

// Backlight PWM. Docs warn against full brightness for long periods, so default
// to ~50% duty (8-bit resolution, 128/255).
#define TFT_BL_FREQ       5000
#define TFT_BL_RES_BITS   8
#define TFT_BL_DUTY       128

// Onboard addressable RGB LED (WS2812) on the Waveshare ESP32-C6-LCD-1.47.
#define RGB_LED_PIN       8

// Onboard BOOT pushbutton (active low, has external pull-up).
#define BUTTON_PIN        9

#endif
