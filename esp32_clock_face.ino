#include "wifi_connection.h"
#include "config.h"
#include "http_server.h"
#include "NTPSync.h"
#include "lcd_display.h"
#include "gpio.h"
#include "images.h"
#include "tcp_server.h"

enum Operation {
  Init,
  WifiCredentials,
  ConnectToAp,
  ShowIp,
  ShowTime
};

static Operation state = Init;
static uint32_t stateChangedAt = 0;
static int lastSeconds = 0;
static int blikX = 0;
static int blikY = 0;
static String statusMessage = "";         /* This is set and requested from other modules. */
static String timeStringHH = "";
static String timeStringMM = "";
static bool showImage = false;             /* true: show SD image instead of clock */
static String currentImage = "";
static bool displayDirty = false;          /* force a redraw on mode/selection change */

void MAIN_setStatusMsg(String msg){
  statusMessage = msg;
}

String MAIN_getStatusMsg(void){
  return statusMessage;
}

void MAIN_setDisplayClock(void){
  showImage = false;
  displayDirty = true;
}

void MAIN_setDisplayImage(String name){
  showImage = true;
  currentImage = name;
  displayDirty = true;
}

void MAIN_displayRefresh(void){
  displayDirty = true;
}

String MAIN_getDisplay(void){
  return showImage ? currentImage : String("clock");
}

static void display_boudries()
{
  LCD_clear();
  LCD_color(C_YELLOW);  
  LCD_write("...1\n....2\n.....3\n......4\n.......5");  
}

static void display_wifi_credentials()
{
  LCD_clear();
  LCD_color(C_YELLOW);
  LCD_write("\nWiFi SSID:\n");
  LCD_color(C_WHITE);
  String message = String(WIFIC_getDeviceName());
  LCD_write(message);
  LCD_color(C_YELLOW);
  LCD_write("\nPASS:");
  LCD_color(C_WHITE);
  LCD_write(AP_PASS); 
  LCD_color(C_YELLOW);
  LCD_write("\nIP:");
  LCD_color(C_WHITE);
  LCD_write(WIFIC_getApIp());  
}


void setup(void) 
{
  /* Need to wait for background processes to complete. Otherwise trouble with gpio.*/
  delay(100);
  Serial.begin(115200);
  WIFIC_init();
  HTTP_SERVER_init();
  TCP_init();
  LCD_init();
  NTPS_init();
  GPIO_init();
  IMG_init();
}

void loop(void){
  HTTP_SERVER_process();
  TCP_process();
  GPIO_process();
  LCD_process();
  WIFIC_process();
  // Only sync NTP while the clock is shown. Timekeeping continues regardless
  // (NTPClient advances from millis between syncs); we just skip the network
  // poll when an image is displayed and the clock isn't visible.
  if(WIFIC_stationConnected() && !showImage){
    NTPS_process();
  }

  // State machine
  switch(state){
    case Init:
    {
      // display_boudries();
      state = WifiCredentials;
      stateChangedAt = millis();
    }break;

    case WifiCredentials:
    {
      if((millis() - stateChangedAt) > 5){
        display_wifi_credentials();
        state = ConnectToAp;
        stateChangedAt = millis();
      }      
    }break;

    case ConnectToAp:
    {
      if((millis() - stateChangedAt) > 5000){
        LCD_clear();
        LCD_write("\nWaiting for WiFi,\nNTP sync..."); 
        String stationIp = WIFIC_getStationIp();
        state = ShowIp;
        stateChangedAt = millis();        
      }   
    }break;

    case ShowIp:
    {
      String stationIp = WIFIC_getStationIp();
      if((stationIp.length() > 1)){
        LCD_color(C_YELLOW);
        LCD_write("\nConnected IP:\n");
        LCD_color(C_WHITE);
        LCD_write(stationIp);            
        state = ShowTime;
        stateChangedAt = millis();  
      }    
    }break;

    default:
    {
      if(showImage){
        if(displayDirty){
          IMG_show(currentImage);
          displayDirty = false;
        }
        break;
      }

      if(displayDirty){
        // Returning from image mode to the clock; force a full redraw.
        LCD_clear();
        timeStringHH = "";
        timeStringMM = "";
        lastSeconds = -1;
        displayDirty = false;
      }

      if((millis() - stateChangedAt) > 5000){
        if(NTPS_hasSynced()){
          String hh = NTPS_getHH();
          String mm = NTPS_getMM();

          if(!hh.equals(timeStringHH) || !mm.equals(timeStringMM)){
            timeStringHH = hh;
            timeStringMM = mm;

            LCD_clear(); 
            LCD_color(C_YELLOW);
            LCD_setFont(Font12pt);
            LCD_write("\n\n\n\n  ");  
            LCD_setFont(Font24pt);  
            LCD_textSize(2);
            LCD_write(timeStringHH);
            LCD_textSize(1);
            blikX = LCD_getX();
            blikY = LCD_getY();
            LCD_write(" ");
            LCD_textSize(2);
            LCD_write(timeStringMM);

            // Date in small font, centered under the time.
            int dateY = LCD_getY() + 56;
            LCD_setFont(Font18pt);     
            LCD_textSize(1);     
            LCD_color(C_BLUE);
            LCD_writeCentered(NTPS_getDate(), dateY);
          }

          int seconds = NTPS_getSeconds();
          if(lastSeconds != seconds){
            lastSeconds = seconds;
            bool blinkOn = (lastSeconds % 2) == 0;
            // Restore the large font/position used for the time before redrawing the colon.
            LCD_setFont(Font24pt);
            LCD_textSize(1);
            LCD_setCursor(blikX, blikY - 12);

            if(blinkOn){
              LCD_color(LCD_getFgColor());
              LCD_write(":");
            }else{
              LCD_color(LCD_getBgColor());
              LCD_write(":");
            }
          }
        }
      }
    }break;
  }

}

