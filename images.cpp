/*
 *  1-bpp full-screen image source backed by the on-chip LittleFS filesystem.
 *  Every frame is 320x172, 1 bit/pixel (MSB first, 40 bytes/row -> 6880 bytes),
 *  drawn in the current LED colour on a black background. No SD card.
 */

#include <LittleFS.h>
#include <WebServer.h>
#include "config.h"
#include "images.h"
#include "lcd_display.h"
#include "gpio.h"

#define IMG_W      320
#define IMG_H      172
#define IMG_STRIDE (IMG_W / 8)          // 40 bytes/row
#define IMG_BYTES  (IMG_STRIDE * IMG_H) // 6880

static bool fsReady = false;
static String uploadErr;

String IMG_lastError(void){ return uploadErr; }

bool IMG_init(void){
  fsReady = LittleFS.begin(true);      // format on first run if needed
  return fsReady;
}

// 0x00RRGGBB -> RGB565. Falls back to white if the LED is off (black on black).
static uint16_t ledColor565(void){
  uint32_t c = GPIO_getLedHex();
  if(c == 0){
    return C_WHITE;
  }
  uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Draw a full 6880-byte 1-bpp frame to the panel, streaming a row at a time.
static void drawBits(const uint8_t* data){
  uint16_t color = ledColor565();
  uint16_t bg = LCD_getBgColor();
  static uint16_t row[IMG_W];
  LCD_imageBegin();
  for(int y = 0; y < IMG_H; y++){
    const uint8_t* rp = data + y * IMG_STRIDE;
    for(int x = 0; x < IMG_W; x++){
      row[x] = (rp[x >> 3] & (0x80 >> (x & 7))) ? color : bg;
    }
    LCD_imagePush(row, IMG_W);
  }
  LCD_imageEnd();
}

static String sanitizeName(String name){
  int slash = name.lastIndexOf('/');
  if(slash >= 0){ name = name.substring(slash + 1); }
  slash = name.lastIndexOf('\\');
  if(slash >= 0){ name = name.substring(slash + 1); }
  if(name.length() == 0 || name.length() > 32){ return ""; }
  for(unsigned i = 0; i < name.length(); i++){
    char c = name[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    if(!ok){ return ""; }
  }
  return "/" + name;
}

// Uploaded LittleFS files, joined with '|'.
String IMG_list(void){
  String result = "";
  if(fsReady){
    File root = LittleFS.open("/");
    if(root){
      for(File e = root.openNextFile(); e; e = root.openNextFile()){
        if(!e.isDirectory()){
          String n = String(e.name());
          int slash = n.lastIndexOf('/');
          if(slash >= 0){ n = n.substring(slash + 1); }
          if(n.length()){
            if(result.length()){ result += "|"; }
            result += n;
          }
        }
        e.close();
      }
      root.close();
    }
  }
  return result;
}

bool IMG_show(String name){
  if(!fsReady){ return false; }
  String path = sanitizeName(name);
  if(path.length() == 0){ return false; }
  File f = LittleFS.open(path, "r");
  if(!f){ return false; }
  if(f.size() != IMG_BYTES){ f.close(); return false; }
  static uint8_t buf[IMG_BYTES];
  f.read(buf, IMG_BYTES);
  f.close();
  drawBits(buf);
  return true;
}

bool IMG_sendRaw(String name, WebServer* server){
  if(!fsReady){ return false; }
  String path = sanitizeName(name);
  if(path.length() == 0){ return false; }
  File f = LittleFS.open(path, "r");
  if(!f){ return false; }
  server->setContentLength(f.size());
  server->send(200, "application/octet-stream", "");
  static uint8_t buf[512];
  while(true){
    int got = f.read(buf, sizeof(buf));
    if(got <= 0){ break; }
    server->sendContent((const char*)buf, got);
  }
  f.close();
  return true;
}

// --- upload ---
static File uploadFile;
static String uploadPath;
static size_t uploadBytes = 0;   // counted as written (File.size() is unreliable mid-write)

bool IMG_writeBegin(String name){
  uploadErr = "";
  uploadBytes = 0;
  if(!fsReady){ uploadErr = "filesystem not ready"; return false; }
  uploadPath = sanitizeName(name);
  if(uploadPath.length() == 0){ uploadErr = "invalid file name: '" + name + "'"; return false; }
  uploadFile = LittleFS.open(uploadPath, "w");
  if(!uploadFile){ uploadErr = "cannot open " + uploadPath + " for write"; return false; }
  return true;
}

bool IMG_writeChunk(const uint8_t* buf, size_t len){
  if(!uploadFile){ return false; }
  size_t wrote = uploadFile.write(buf, len);
  uploadBytes += wrote;
  if(wrote != len){ uploadErr = "write failed (" + String(wrote) + " of " + String(len) + ")"; return false; }
  return true;
}

bool IMG_writeEnd(void){
  if(!uploadFile){ return false; }
  uploadFile.flush();
  uploadFile.close();
  if(uploadBytes != (size_t)IMG_BYTES){
    LittleFS.remove(uploadPath);
    uploadErr = "wrong size: got " + String(uploadBytes) + " bytes, expected " + String(IMG_BYTES);
    return false;
  }
  return true;
}

void IMG_writeAbort(void){
  if(uploadFile){
    uploadFile.close();
    LittleFS.remove(uploadPath);
  }
}

bool IMG_delete(String name){
  uploadErr = "";
  if(!fsReady){ uploadErr = "filesystem not ready"; return false; }
  String path = sanitizeName(name);
  if(path.length() == 0){ uploadErr = "invalid name: '" + name + "'"; return false; }
  if(!LittleFS.exists(path)){ uploadErr = "not found: " + path; return false; }
  if(!LittleFS.remove(path)){ uploadErr = "remove failed: " + path; return false; }
  return true;
}
