/* 
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *  
 *  HTTP server which generates the web browser pages. 
 */

#include <WebServer.h>
#include "wifi_connection.h"
#include "config.h"
#include "esp32_clock_face.h"
#include "gpio.h"
#include "lcd_display.h"
#include "images.h"
#include "NTPSync.h"
#include "http_ui.h"

// --- Web server object ---
WebServer* webServer = nullptr;

// --- Page handlers ---
void showStartPage() { 
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(INDEX_HTML_0);
  response += FPSTR(NAV_HTML);
  response += "<h1>WiFi Clock</h1>";
  response += "<p class='ip'>Station IP: " + WIFIC_getStationIp() + "</p>";
  response += FPSTR(INDEX_HTML_1);
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);  
}


static void showApiPage(void){
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(API_HTML_0);
  response += FPSTR(NAV_HTML);
  response += FPSTR(API_HTML_1);
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
}

static void showNotFound(void){
  webServer->send(404, "text/html; charset=iso-8859-1","<html><head> <title>404 Not Found</title></head><body><h1>Not Found</h1></body></html>"); 
}

static void showStatusPage(bool goToHome = false) {    
  String response = FPSTR(HTML_BEGIN);
  response += "<h1>Connection Status</h1><p>";
  response += MAIN_getStatusMsg() + "</p>";
  if(goToHome){
    response += FPSTR(REDIRECT_HTML);
  }
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);   
}

static void selectAP(void) {
  WIFIC_startScan();   // kick off the scan; JS fetches /aplist ~10 s later
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(APLIST_HTML_0);
  response += FPSTR(NAV_HTML);
  response += FPSTR(APLIST_HTML_1);
  response += FPSTR(APLIST_HTML_2);
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
}

static void saveWiFi(void){
  String ssid = webServer->arg("s");
  String pass = webServer->arg("p");
  
  if((ssid.length() > 63) || (pass.length() > 63)){
      MAIN_setStatusMsg("Sorry, this module can only remember SSID and a PASSWORD up to 63 bytes long.");
      showStatusPage(true); 
      return;
  } 

  String st_ssid = WIFIC_getStSSID();
  String st_pass = WIFIC_getStPass();

  if(st_ssid.equals(ssid) && st_pass.equals(pass)){
      MAIN_setStatusMsg("All parameters are already set as requested.");
      showStatusPage(true);      
      return;
  }   

  WIFIC_setStSSID(ssid);
  WIFIC_setStPass(pass);

  String http_statusMessage;

  if(ssid.length() > 3){    
    http_statusMessage = "Saving settings and connecting to SSID: " + ssid;    
  }else{       
    http_statusMessage = "No SSID selected...";    
  }

  MAIN_setStatusMsg(http_statusMessage);
  showStatusPage();

  WIFIC_stationMode();
}

static void setLed(void){
  String c = webServer->arg("c");
  if(c.length() != 6){
    webServer->send(400, "text/plain", "Expected 6 hex digits");
    return;
  }
  uint32_t rgb = (uint32_t)strtoul(c.c_str(), nullptr, 16);
  GPIO_setLedHex(rgb);
  webServer->send(200, "text/plain", "OK");
}

static void flipScreen(void){
  LCD_rotate180();
  MAIN_displayRefresh();
  webServer->send(200, "text/plain", "OK");
}

static void setBacklight(void){
  int v = webServer->arg("v").toInt();
  if(v < 0){
    v = 0;
  }
  LCD_setBacklight((uint8_t)v);
  webServer->send(200, "text/plain", "OK");
}

static void flashBacklight(void){
  LCD_flashBacklight();
  webServer->send(200, "text/plain", "OK");
}

static void setDisplay(void){
  String img = webServer->arg("img");
  if(img.length() == 0 || img == "clock"){
    MAIN_setDisplayClock();
  }else{
    MAIN_setDisplayImage(img);
  }
  webServer->send(200, "text/plain", "OK");
}

static void imageList(void){
  webServer->send(200, "text/plain", IMG_list());
}

static void getImage(void){
  String name = webServer->arg("name");
  if(name.length() == 0 || !IMG_sendRaw(name, webServer)){
    webServer->send(404, "text/plain", "Image not found");
  }
}

static void deleteImage(void){
  String name = webServer->arg("name");
  if(name.length() == 0 || !IMG_delete(name)){
    String err = IMG_lastError();
    webServer->send(400, "text/plain", err.length() ? err : "Delete failed");
    return;
  }
  if(MAIN_getDisplay() == name){
    MAIN_setDisplayClock();   // was on screen; fall back to clock
  }
  webServer->send(200, "text/plain", "OK");
}

static bool uploadOk = false;

// Streams a multipart 1-bpp frame upload straight to LittleFS, chunk by chunk.
static void uploadImage(void){
  HTTPUpload& up = webServer->upload();
  if(up.status == UPLOAD_FILE_START){
    uploadOk = IMG_writeBegin(up.filename);
  }else if(up.status == UPLOAD_FILE_WRITE){
    if(uploadOk){
      uploadOk = IMG_writeChunk(up.buf, up.currentSize);
    }
  }else if(up.status == UPLOAD_FILE_END){
    if(uploadOk){
      uploadOk = IMG_writeEnd();
    }else{
      IMG_writeAbort();   // a chunk failed: close + remove the partial file
    }
  }else{
    IMG_writeAbort();
    uploadOk = false;
  }
}

static void uploadDone(void){
  if(uploadOk){
    webServer->send(200, "text/plain", "OK");
  }else{
    String err = IMG_lastError();
    if(err.length() == 0){
      err = "Upload failed";
    }
    webServer->send(400, "text/plain", err);
  }
}

static void apList(void){
  webServer->send(200, "text/plain", WIFIC_getApList());
}

static void getDisplay(void){
  webServer->send(200, "text/plain", MAIN_getDisplay());
}

// "HH|MM|SS|DD.MM" from NTP, or "" if not synced yet.
static void getTime(void){
  if(!NTPS_hasSynced()){
    webServer->send(200, "text/plain", "");
    return;
  }
  String r = NTPS_getHH() + "|" + NTPS_getMM() + "|" +
             String(NTPS_getSeconds()) + "|" + NTPS_getDate();
  webServer->send(200, "text/plain", r);
}

static bool httpRunning = false;

// --- Public functions ---
void HTTP_SERVER_process(void){
  if(httpRunning){
    webServer->handleClient();
  }
}

// Best-effort: true while a browser holds an (keep-alive) connection. Used to
// avoid tearing down the server out from under an active web user.
bool HTTP_SERVER_clientConnected(void){
  return (webServer != nullptr) && webServer->client().connected();
}

bool HTTP_SERVER_isRunning(void){
  return httpRunning;
}

void HTTP_SERVER_start(void){
  if(webServer != nullptr && !httpRunning){
    webServer->begin();
    httpRunning = true;
  }
}

void HTTP_SERVER_stop(void){
  if(webServer != nullptr && httpRunning){
    webServer->close();
    httpRunning = false;
  }
}

void HTTP_SERVER_init(void){
  if (webServer != nullptr) {
    delete webServer; // Clean up old one
  }
  webServer = new WebServer(80);

  webServer->on("/", HTTP_GET, showStartPage);
  webServer->on("/favicon.ico", HTTP_GET, showNotFound);
  webServer->on("/selectap", HTTP_GET, selectAP);
  webServer->on("/api", HTTP_GET, showApiPage);
  webServer->on("/wifisave", HTTP_GET, saveWiFi);
  webServer->on("/api/setled", HTTP_GET, setLed);
  webServer->on("/api/flipscreen", HTTP_GET, flipScreen);
  webServer->on("/api/setdisplay", HTTP_GET, setDisplay);
  webServer->on("/api/setbl", HTTP_GET, setBacklight);
  webServer->on("/api/flashbl", HTTP_GET, flashBacklight);
  webServer->on("/api/imagelist", HTTP_GET, imageList);
  webServer->on("/getimage", HTTP_GET, getImage);
  webServer->on("/upload", HTTP_POST, uploadDone, uploadImage);
  webServer->on("/delete", HTTP_GET, deleteImage);
  webServer->on("/aplist", HTTP_GET, apList);
  webServer->on("/getdisplay", HTTP_GET, getDisplay);
  webServer->on("/gettime", HTTP_GET, getTime);
  webServer->onNotFound(showStartPage);

  webServer->begin();
  httpRunning = true;
}
