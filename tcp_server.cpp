/*
 *  JSON-over-TCP API server.
 *
 *  Connect to TCP_API_PORT and send one JSON command per line (\n terminated);
 *  the server replies with one JSON object per line. Example:
 *    {"cmd":"list"}                       -> {"images":"eyes1.bin|eyes2.bin"}
 *    {"cmd":"setdisplay","img":"eyes1.bin"} -> {"ok":true}
 *    {"cmd":"setled","c":"00ff00"}        -> {"ok":true}
 *    {"cmd":"httpserver","enable":false}  -> {"ok":true,"running":false}
 *
 *  Mirrors the HTTP /api/* actions; also start/stops the HTTP server to free CPU.
 */

#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "tcp_server.h"
#include "sd_images.h"
#include "esp32_clock_face.h"
#include "gpio.h"
#include "lcd_display.h"
#include "NTPSync.h"
#include "http_server.h"

static WiFiServer tcpServer(TCP_API_PORT);
static WiFiClient tcpClient;
static String lineBuf;

void TCP_init(void){
  tcpServer.begin();
  tcpServer.setNoDelay(true);
}

// Parse one JSON command line and write back one JSON response line.
static void handleLine(const String& line){
  DynamicJsonDocument doc(256);
  DynamicJsonDocument res(256);

  if(deserializeJson(doc, line)){
    res["error"] = "bad json";
  }else{
    String cmd = doc["cmd"] | "";

    if(cmd == "list"){
      res["images"] = SDIMG_list();

    }else if(cmd == "getdisplay"){
      res["display"] = MAIN_getDisplay();

    }else if(cmd == "setdisplay"){
      String img = doc["img"] | "";
      if(img.length() == 0 || img == "clock"){
        MAIN_setDisplayClock();
      }else{
        MAIN_setDisplayImage(img);
      }
      res["ok"] = true;

    }else if(cmd == "gettime"){
      if(NTPS_hasSynced()){
        res["time"] = NTPS_getHH() + "|" + NTPS_getMM() + "|" +
                      String(NTPS_getSeconds()) + "|" + NTPS_getDate();
      }else{
        res["time"] = "";
      }

    }else if(cmd == "setled"){
      String c = doc["c"] | "";
      if(c.length() != 6){
        res["ok"] = false;
        res["error"] = "expected 6 hex digits";
      }else{
        GPIO_setLedHex((uint32_t)strtoul(c.c_str(), nullptr, 16));
        res["ok"] = true;
      }

    }else if(cmd == "setbl"){
      int v = doc["v"] | 0;
      if(v < 0){ v = 0; }
      if(v > 100){ v = 100; }
      LCD_setBacklight((uint8_t)v);
      res["ok"] = true;

    }else if(cmd == "flashbl"){
      LCD_flashBacklight();
      res["ok"] = true;

    }else if(cmd == "flipscreen"){
      LCD_rotate180();
      MAIN_displayRefresh();
      res["ok"] = true;

    }else if(cmd == "httpserver"){
      if(doc.containsKey("enable")){
        bool enable = doc["enable"];
        if(enable){
          HTTP_SERVER_start();
          res["ok"] = true;
        }else if(HTTP_SERVER_clientConnected()){
          res["ok"] = false;            // refuse: a browser is still connected
          res["error"] = "http client connected";
        }else{
          HTTP_SERVER_stop();
          res["ok"] = true;
        }
      }
      res["running"] = HTTP_SERVER_isRunning();

    }else{
      res["error"] = "unknown cmd";
    }
  }

  String out;
  serializeJson(res, out);
  out += "\n";
  tcpClient.print(out);
}

void TCP_process(void){
  // Accept a new client only when the previous one has gone (one at a time).
  if(!tcpClient || !tcpClient.connected()){
    WiFiClient next = tcpServer.available();
    if(next){
      tcpClient = next;
      lineBuf = "";
    }
  }

  if(tcpClient && tcpClient.connected()){
    while(tcpClient.available()){
      char ch = (char)tcpClient.read();
      if(ch == '\n'){
        String line = lineBuf;
        lineBuf = "";
        line.trim();
        if(line.length()){
          handleLine(line);
        }
      }else if(ch != '\r'){
        if(lineBuf.length() < 255){     // cap a single line
          lineBuf += ch;
        }
      }
    }
  }
}
