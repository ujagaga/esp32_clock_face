#ifndef IMAGES_H
#define IMAGES_H

#include <Arduino.h>

class WebServer;

// Full-screen 320x172 1-bpp frames stored in the on-chip LittleFS filesystem
// and drawn in the current LED colour on black. No SD card.
//
// Frame format: row-major, 1 bit/pixel, MSB first, 40 bytes/row -> 6880 bytes.

extern bool IMG_init(void);
extern String IMG_list(void);              // selectable names joined with '|'
extern bool IMG_show(String name);         // draw the named frame full screen
extern bool IMG_sendRaw(String name, WebServer* server);  // stream raw 1-bpp bytes

// Incremental upload of a 1-bpp frame to LittleFS (START/WRITE/END order).
extern bool IMG_writeBegin(String name);
extern bool IMG_writeChunk(const uint8_t* buf, size_t len);
extern bool IMG_writeEnd(void);
extern void IMG_writeAbort(void);
extern String IMG_lastError(void);
extern bool IMG_delete(String name);       // delete an uploaded frame

#endif
