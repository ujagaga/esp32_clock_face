#ifndef SD_IMAGES_H
#define SD_IMAGES_H

#include <Arduino.h>

// Reads raw RGB565 (big-endian) image files from the microSD card and blits
// them to the LCD. Files must be sized to the screen (landscape: 320x172) with
// no header. Convert on a PC, e.g.:
//   ffmpeg -i in.png -vf scale=320:172 -f rawvideo -pix_fmt rgb565be out.bin

extern bool SDIMG_init(void);
extern String SDIMG_list(void);            // image file names joined with '|'
extern bool SDIMG_show(String name);       // draw the named file full screen

#endif
