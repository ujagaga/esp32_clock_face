#ifndef SD_IMAGES_H
#define SD_IMAGES_H

#include <Arduino.h>

class WebServer;

// Reads raw RGB565 (big-endian) image files from the microSD card and blits
// them to the LCD. Files must be sized to the screen (landscape: 320x172) with
// no header. Convert on a PC, e.g.:
//   ffmpeg -i in.png -vf scale=320:172 -f rawvideo -pix_fmt rgb565be out.bin

extern bool SDIMG_init(void);
extern String SDIMG_list(void);            // image file names joined with '|'
extern bool SDIMG_show(String name);       // draw the named file full screen
extern bool SDIMG_sendRaw(String name, WebServer* server);  // stream raw bytes to HTTP client

// Incremental write of an uploaded image to SD (call in START/WRITE/END order).
extern bool SDIMG_writeBegin(String name);
extern bool SDIMG_writeChunk(const uint8_t* buf, size_t len);
extern bool SDIMG_writeEnd(void);
extern void SDIMG_writeAbort(void);
extern String SDIMG_lastError(void);       // reason the last upload failed
extern bool SDIMG_delete(String name);     // remove the named file from SD

#endif
