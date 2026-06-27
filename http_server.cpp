/* 
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *  
 *  HTTP server which generates the web browser pages. 
 */

#include <WebServer.h>
#include <pgmspace.h>
#include "wifi_connection.h"
#include "config.h"
#include "esp32_clock_face.h"
#include "gpio.h"
#include "lcd_display.h"
#include "sd_images.h"

// --- HTML templates ---
static const char HTML_BEGIN[] PROGMEM = R"(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0">
    <title>WiFi Clock</title>
    <style>
      body { background-color: white; font-family: Arial, Helvetica, Sans-Serif; color: #000000; }
      .contain{width: 100%;}
      .center_div{margin:0 auto; max-width: 400px;position:relative;}
    </style>
  </head>
  <body>
)";

static const char HTML_END[] PROGMEM = "</body></html>";

static const char INDEX_HTML_0[] PROGMEM = R"(
<style>
  .btn_cfg{border:0;border-radius:0.3rem;color:#fff;line-height:1.4rem;font-size:0.8rem;margin:1ch;height:2rem;width:10rem;background-color:#ff3300;}
</style>
<div class="contain">
  <div class="center_div">
  <h1>WiFi Clock</h1>
)";

const char INDEX_HTML_1[] PROGMEM = R"(
  <br>
  <label for="led">LED color: </label>
  <input type="color" id="led" value="#000000" oninput="setLed(this.value);">
  <br/><br/>
  <label for="bl">Brightness: </label>
  <input type="range" id="bl" min="0" max="100" value="100" oninput="setBl(this.value);">
  <br/><br/>
  <button class="btn_cfg" type="button" onclick="fetch('/flashbl');">Flash</button>
  <br/>
  <button class="btn_cfg" type="button" onclick="fetch('/flipscreen');">Flip Screen</button>
  <br/>
  <button class="btn_cfg" type="button" onclick="location.href='/selectap';">Configure WiFi</button>
  <br/>
  <hr>
  <p id="pvlabel" style="display:none;">Preview:</p>
  <canvas id="pv" width="320" height="172" style="display:none;max-width:100%;border:1px solid #ccc;"></canvas>
  <hr>
  <a href='https://github.com/ujagaga/esp32_clock_face' target="_blank" rel="noopener noreferrer">Source code</a>
</div>
<script>
  var PV_W=320, PV_H=172;
  function onSel(v){
    fetch('/setdisplay?img=' + encodeURIComponent(v));
    var cv=document.getElementById('pv'), lb=document.getElementById('pvlabel');
    if(v==='clock'){
      cv.style.display='none'; lb.style.display='none';
      return;
    }
    fetch('/getimage?name=' + encodeURIComponent(v)).then(function(r){
      if(!r.ok){ throw new Error('not found'); }
      return r.arrayBuffer();
    }).then(function(ab){
      var bytes=new Uint8Array(ab);
      var img=cv.getContext('2d').createImageData(PV_W, PV_H);
      var d=img.data, n=PV_W*PV_H;
      for(var i=0;i<n;i++){
        var hi=bytes[2*i], lo=bytes[2*i+1];      // big-endian RGB565
        var p=(hi<<8)|lo;
        var r5=(p>>11)&0x1f, g6=(p>>5)&0x3f, b5=p&0x1f;
        d[4*i]  =(r5*255/31)|0;
        d[4*i+1]=(g6*255/63)|0;
        d[4*i+2]=(b5*255/31)|0;
        d[4*i+3]=255;
      }
      cv.getContext('2d').putImageData(img,0,0);
      cv.style.display='block'; lb.style.display='block';
    }).catch(function(){
      cv.style.display='none'; lb.style.display='none';
    });
  }
  function setLed(v){ fetch('/setled?c=' + v.substring(1)); }
  function setBl(v){ fetch('/setbl?v=' + v); }
  setInterval(function(){
    fetch('/getdisplay').then(function(r){return r.text();}).then(function(v){
      var d=document.getElementById('disp');
      if(d && d.value!==v){ d.value=v; }
    });
  }, 2000);
</script>
)";

static const char APLIST_HTML_0[] PROGMEM = R"(
<style>
  .c{text-align: center;}
  div,input{padding:5px;font-size:1em;}
  input{width:95%;}
  body{text-align: left;}
  button{width:100%;border:0;border-radius:0.3rem;color:#fff;line-height:2.4rem;font-size:1.2rem;height:40px;background-color:#1fa3ec;}
  .q{float: right;width: 64px;text-align: right;}
  .radio{width:2em;}
  #vm{width:100%;height:50vh;overflow-y:auto;margin-bottom:1em;} 
</style>
</head><body>  
  <div class="contain">
    <div class="center_div">
)";    

static const char APLIST_HTML_1[] PROGMEM = R"(   
      <h1 id='ttl'>Networks found:</h1>
      <div id='vm'>
)";  

static const char APLIST_HTML_2[] PROGMEM = R"( 
      </div>
      <form method='get' action='wifisave'>
        <button type='button' onclick='refresh();'>Rescan</button><br/><br/>
        <input id='s' name='s' length=32 placeholder='SSID (Leave blank for AP mode)'><br>      
        <input id='p' name='p' length=32 placeholder='Password'><br>        
        <br><button type='submit'>Save</button>        
      </form>      
     </div>
  </div>
<script>
  function c(l){
    document.getElementById('s').value=l.innerText||l.textContent;
    document.getElementById('p').focus();
  }
  
  var cn=new WebSocket('ws://'+location.hostname+':81/');
  cn.onopen=function(){
    cn.send('{"APLIST":""}');
  }
  cn.onmessage=function(e){
    var data=JSON.parse(e.data);
    if(data.hasOwnProperty('APLIST')){
      rsp=data.APLIST.split('|');
      document.getElementById('vm').innerHTML='';
      for(var i=0;i<(rsp.length);i++){
        document.getElementById('vm').innerHTML+='<span>'+(i+1)+": </span><a href='#p' onclick='c(this)'>" + rsp[i] + '</a><br>';
      }
      if(!document.getElementById('vm').innerHTML.replace(/\\s/g,'').length){
        document.getElementById('ttl').innerHTML='No networks found.'
      } 
    }
  };
  function refresh(){
    document.getElementById('vm').innerHTML='Please wait...'
    cn.send('{"APLIST":""}');
  } 
</script>
)";

static const char REDIRECT_HTML[] PROGMEM = R"(
<p id="tmr"></p>
<script>
  var c=6;   
  function count(){
    var tmr=document.getElementById('tmr');   
    if(c>0){
      c--;
      tmr.innerHTML="You will be redirected to home page in "+c+" seconds.";
      setTimeout('count()',1000);
    }else{
      window.location.href="/";
    }
  }
  count();
</script> 
)";

// --- Web server object ---
WebServer* webServer = nullptr;

// --- Page handlers ---
void showStartPage() { 
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(INDEX_HTML_0);
  response += "<p>Station IP: " + WIFIC_getStationIp() + "</p>";

  // Display selector: clock or one of the SD card images.
  response += "<label for='disp'>Display: </label>";
  response += "<select id='disp' onchange=\"onSel(this.value);\">";
  response += "<option value='clock'>Clock</option>";
  String imgList = SDIMG_list();
  int start = 0;
  while(start < imgList.length()){
    int sep = imgList.indexOf('|', start);
    if(sep < 0){
      sep = imgList.length();
    }
    String name = imgList.substring(start, sep);
    if(name.length() > 0){
      response += "<option value='" + name + "'>" + name + "</option>";
    }
    start = sep + 1;
  }
  response += "</select>";

  response += FPSTR(INDEX_HTML_1);
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
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(APLIST_HTML_0);  
  response += FPSTR(APLIST_HTML_1);
  response += "Please wait...";  
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

static void getDisplay(void){
  webServer->send(200, "text/plain", MAIN_getDisplay());
}

static void imageList(void){
  webServer->send(200, "text/plain", SDIMG_list());
}

static void getImage(void){
  String name = webServer->arg("name");
  if(name.length() == 0 || !SDIMG_sendRaw(name, webServer)){
    webServer->send(404, "text/plain", "Image not found");
  }
}

static void apList(void){
  webServer->send(200, "text/plain", WIFIC_getApList());
}

// --- Public functions ---
void HTTP_SERVER_process(void){
  webServer->handleClient(); 
}

void HTTP_SERVER_init(void){   
  if (webServer != nullptr) {
    delete webServer; // Clean up old one
  }
  webServer = new WebServer(80);

  webServer->on("/", HTTP_GET, showStartPage);
  webServer->on("/favicon.ico", HTTP_GET, showNotFound);
  webServer->on("/selectap", HTTP_GET, selectAP);
  webServer->on("/wifisave", HTTP_GET, saveWiFi);
  webServer->on("/setled", HTTP_GET, setLed);
  webServer->on("/flipscreen", HTTP_GET, flipScreen);
  webServer->on("/setdisplay", HTTP_GET, setDisplay);
  webServer->on("/setbl", HTTP_GET, setBacklight);
  webServer->on("/flashbl", HTTP_GET, flashBacklight);
  webServer->on("/getdisplay", HTTP_GET, getDisplay);
  webServer->on("/imagelist", HTTP_GET, imageList);
  webServer->on("/getimage", HTTP_GET, getImage);
  webServer->on("/aplist", HTTP_GET, apList);
  webServer->onNotFound(showStartPage);
  
  webServer->begin();
}
