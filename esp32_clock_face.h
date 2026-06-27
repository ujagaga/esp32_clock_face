#ifndef ESP_CLOCK_FACE_H
#define ESP_CLOCK_FACE_H

#include <Arduino.h>

extern void MAIN_setStatusMsg(String msg);
extern String MAIN_getStatusMsg(void);
extern void MAIN_setDisplayClock(void);
extern void MAIN_setDisplayImage(String name);
extern void MAIN_displayRefresh(void);
extern String MAIN_getDisplay(void);

#endif
