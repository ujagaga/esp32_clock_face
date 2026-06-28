#include "gpio.h"
#include "config.h"
#include "esp32_clock_face.h"

// The onboard RGB LED is a single WS2812 (addressable) on RGB_LED_PIN. The
// ESP32 Arduino core drives it over RMT via neopixelWrite(), so no extra
// library is needed. This board's LED has red and green swapped versus what
// neopixelWrite() assumes, so we pass green and red in swapped order below.

void GPIO_init(void)
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  GPIO_setLedOff();
}

// Poll the BOOT button (active low). On a debounced press, force the clock.
void GPIO_process(void)
{
  static bool wasPressed = false;
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  if(pressed && !wasPressed){
    delay(20);                                 // simple debounce
    if(digitalRead(BUTTON_PIN) == LOW){
      MAIN_setDisplayClock();
      wasPressed = true;
    }
  }else if(!pressed){
    wasPressed = false;
  }
}

static uint32_t lastColor = 0;   // last colour set, 0x00RRGGBB

void GPIO_setLedColor(uint8_t r, uint8_t g, uint8_t b)
{
  lastColor = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  neopixelWrite(RGB_LED_PIN, g, r, b);
}

void GPIO_setLedHex(uint32_t rgb)
{
  GPIO_setLedColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

uint32_t GPIO_getLedHex(void)
{
  return lastColor;
}

void GPIO_setLedOff(void)
{
  GPIO_setLedColor(0, 0, 0);
}
