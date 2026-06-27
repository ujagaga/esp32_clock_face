#include <SD.h>
#include <SPI.h>
#include "config.h"
#include "lcd_display.h"
#include "sd_images.h"

static bool cardReady = false;

bool SDIMG_init(void){
  LCD_busRelease();
  cardReady = SD.begin(SD_CS, SPI, SD_SPI_FREQ);
  LCD_busAcquire();
  return cardReady;
}

// Return root-level *.bin file names joined with '|'.
String SDIMG_list(void){
  String result = "";
  if(!cardReady){
    return result;
  }

  LCD_busRelease();
  File root = SD.open("/");
  if(root){
    for(File entry = root.openNextFile(); entry; entry = root.openNextFile()){
      if(!entry.isDirectory()){
        String name = String(entry.name());
        if(name.endsWith(".bin") || name.endsWith(".BIN")){
          if(result.length() > 0){
            result += "|";
          }
          result += name;
        }
      }
      entry.close();
    }
    root.close();
  }
  LCD_busAcquire();

  return result;
}

// Stream a raw RGB565 (big-endian) file to the panel. Reads from SD and pushes
// to the LCD in small batches, releasing the shared bus between each.
bool SDIMG_show(String name){
  if(!cardReady){
    return false;
  }

  String path = name.startsWith("/") ? name : ("/" + name);

  LCD_busRelease();
  File f = SD.open(path);
  LCD_busAcquire();
  if(!f){
    return false;
  }

  LCD_imageBegin();

  static uint8_t raw[512];
  static uint16_t px[256];
  while(true){
    LCD_busRelease();
    int got = f.read(raw, sizeof(raw));
    if(got <= 0){
      break;
    }
    int count = got / 2;
    for(int i = 0; i < count; i++){
      px[i] = ((uint16_t)raw[2 * i] << 8) | raw[2 * i + 1];   // big-endian
    }
    LCD_imagePush(px, count);
  }

  f.close();
  LCD_imageEnd();

  return true;
}
