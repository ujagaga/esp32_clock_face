#ifndef GPIO_H
#define GPIO_H

#include <Arduino.h>

// Named colors for the onboard RGB LED, packed as 0x00RRGGBB.
#define RGB_OFF     0x000000
#define RGB_RED     0xFF0000
#define RGB_GREEN   0x00FF00
#define RGB_BLUE    0x0000FF
#define RGB_WHITE   0xFFFFFF
#define RGB_YELLOW  0xFFFF00
#define RGB_CYAN    0x00FFFF
#define RGB_MAGENTA 0xFF00FF
#define RGB_ORANGE  0xFF4000

extern void GPIO_init(void);
extern void GPIO_process(void);              // poll button, call from loop()
extern void GPIO_setLedColor(uint8_t r, uint8_t g, uint8_t b);
extern void GPIO_setLedHex(uint32_t rgb);    // 0x00RRGGBB
extern uint32_t GPIO_getLedHex(void);        // last colour set, 0x00RRGGBB
extern void GPIO_setLedOff(void);

#endif
