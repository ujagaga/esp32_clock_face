#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
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

// Stream a raw RGB565 file to an HTTP client as application/octet-stream using
// chunked transfer. The browser decodes the bytes into a canvas.
bool SDIMG_sendRaw(String name, WebServer* server){
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

  server->setContentLength(f.size());
  server->send(200, "application/octet-stream", "");

  static uint8_t buf[512];
  while(true){
    LCD_busRelease();
    int got = f.read(buf, sizeof(buf));
    LCD_busAcquire();
    if(got <= 0){
      break;
    }
    server->sendContent((const char*)buf, got);
  }

  f.close();
  return true;
}

// --- Incremental upload to SD ---
static File uploadFile;
static String uploadPath;
static String uploadErr;

String SDIMG_lastError(void){
  return uploadErr;
}

// Sanitize to a safe root-level "<name>.bin" path. Returns "" if invalid.
static String sanitizeName(String name){
  int slash = name.lastIndexOf('/');
  if(slash >= 0){
    name = name.substring(slash + 1);
  }
  slash = name.lastIndexOf('\\');
  if(slash >= 0){
    name = name.substring(slash + 1);
  }
  if(name.length() == 0 || name.length() > 32){
    return "";
  }
  for(unsigned i = 0; i < name.length(); i++){
    char c = name[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    if(!ok){
      return "";
    }
  }
  if(!name.endsWith(".bin") && !name.endsWith(".BIN")){
    name += ".bin";
  }
  return "/" + name;
}

// The SD bus is held (LCD released) for the whole upload: begin->chunks->end.
// Re-acquiring the LCD bus between chunks re-latches SPI on the C6 and silently
// corrupts the in-progress SD write. See [[c6-spi-mode-latch]].
bool SDIMG_writeBegin(String name){
  uploadErr = "";
  if(!cardReady){
    uploadErr = "SD card not ready";
    return false;
  }
  uploadPath = sanitizeName(name);
  if(uploadPath.length() == 0){
    uploadErr = "Invalid file name: '" + name + "'";
    return false;
  }
  LCD_busRelease();
  if(SD.exists(uploadPath)){
    SD.remove(uploadPath);
  }
  uploadFile = SD.open(uploadPath, FILE_WRITE);
  if(!uploadFile){
    LCD_busAcquire();
    uploadErr = "Cannot open " + uploadPath + " for write";
    return false;
  }
  return true;
}

bool SDIMG_writeChunk(const uint8_t* buf, size_t len){
  if(!uploadFile){
    return false;
  }
  size_t wrote = uploadFile.write(buf, len);
  if(wrote != len){
    uploadErr = "SD write failed (wrote " + String(wrote) + " of " + String(len) + ")";
    return false;
  }
  return true;
}

bool SDIMG_writeEnd(void){
  if(!uploadFile){
    return false;
  }
  uploadFile.flush();
  size_t size = uploadFile.size();
  uploadFile.close();
  size_t expected = (size_t)(SCREEN_W * SCREEN_H * 2);
  bool sizeOk = (size == expected);
  if(!sizeOk){
    SD.remove(uploadPath);
    uploadErr = "Wrong size: got " + String(size) + " bytes, expected " + String(expected);
  }
  LCD_busAcquire();
  return sizeOk;
}

bool SDIMG_delete(String name){
  if(!cardReady){
    return false;
  }
  String path = sanitizeName(name);
  if(path.length() == 0){
    return false;
  }
  LCD_busRelease();
  bool ok = SD.exists(path) && SD.remove(path);
  LCD_busAcquire();
  return ok;
}

void SDIMG_writeAbort(void){
  if(uploadFile){
    uploadFile.close();
    SD.remove(uploadPath);
    LCD_busAcquire();
  }
}
