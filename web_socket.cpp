#include "config.h"
#ifdef USE_WEBSOCKETS   // whole module is compiled out unless WebSockets are enabled
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include "wifi_connection.h"
#include "esp32_clock_face.h"

WebSocketsServer wsServer = WebSocketsServer(81);

void WS_process(){
  wsServer.loop();   
}

void WS_ServerBroadcast(String msg){
  wsServer.broadcastTXT(msg);
}

static void serverEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{ 
  if(type == WStype_TEXT){
    char textMsg[length + 1];
    memcpy(textMsg, payload, length);
    textMsg[length] = '\0';
        
    DynamicJsonDocument doc(256); // Increased slightly for safety
    DeserializationError error = deserializeJson(doc, textMsg);    

    if (!error) {
      JsonObject root = doc.as<JsonObject>();
      
      if(root.containsKey("APLIST")){
        String APList = "{\"APLIST\":\"" + WIFIC_getApList() + "\"}";
        wsServer.sendTXT(num, APList);
      }

      if(root.containsKey("GETDISP")){
        wsServer.sendTXT(num, "{\"DISP\":\"" + MAIN_getDisplay() + "\"}");
      }
    }
  }   
}

void WS_init(){
  wsServer.close();
  wsServer.begin();
  wsServer.onEvent(serverEvent);
}
#endif // USE_WEBSOCKETS
