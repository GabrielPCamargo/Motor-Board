#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>          //https://github.com/esp8266/Arduino
#endif

//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#define PIN_AP 18

   
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>


AsyncWebServer server(80); //Declara um servidor com porta 80
AsyncWebSocket ws("/ws"); //Declara um metodo para socket cliente


//Página root do HTML do servidor
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
      body {
        height: 100vh;
      }
     .flex {
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100%;
     }
    .arrows {
      font-size:50px;
      color:red;
    }
    .circularArrows {
      font-size:50px;
      color:blue;
    }
    tr {
      height: 20px;
      width: 100%;
    }
    .alternative tr{
      height: 20px;
      width: 25px;
    } 
    td {
      background-color:black;
      border-radius:25%;
      box-shadow: 5px 5px #888888;
      width: 60px;
    }
    td:active {
      transform: translate(5px,5px);
      box-shadow: none; 
    }
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
    <div class=flex>
      <table id="mainTable" style="display: inline-block;margin:auto;table-layout:fixed;text-align: center">
        <tr>
          <td ontouchstart='onTouchStartAndEnd("5")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11017;</span></td>
          <td ontouchstart='onTouchStartAndEnd("1")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#8679;</span></td>
          <td ontouchstart='onTouchStartAndEnd("6")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11016;</span></td>
        </tr>
        
        <tr>
          <td ontouchstart='onTouchStartAndEnd("3")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#8678;</span></td>
          <td></td>    
          <td ontouchstart='onTouchStartAndEnd("4")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#8680;</span></td>
        </tr>
        
        <tr>
          <td ontouchstart='onTouchStartAndEnd("7")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11019;</span></td>
          <td ontouchstart='onTouchStartAndEnd("2")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#8681;</span></td>
          <td ontouchstart='onTouchStartAndEnd("8")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11018;</span></td>
        </tr>
      
        <tr>
          <td ontouchstart='onTouchStartAndEnd("9")' ontouchend='onTouchStartAndEnd("0")'><span class="circularArrows" >&#8634;</span></td>
          <td style="background-color:white;box-shadow:none"></td>
          <td ontouchstart='onTouchStartAndEnd("10")' ontouchend='onTouchStartAndEnd("0")'><span class="circularArrows" >&#8635;</span></td>
        </tr>
      </table>
  
      <table id="mainTable" style=";display:inline-block;margin:auto;table-layout:fixed">
        <tr>
          <th>Alternative buttons</th>
        </tr>
        <tr class="alternative">
          <td ontouchstart='onTouchStartAndEnd("11")' class="arrows">&#8622;</td>
        </tr>
      </table>
    </div>
    
    <script>
      var webSocketUrl = "ws:\/\/" + window.location.hostname + "/ws";
      var websocket;
      
      function initWebSocket() 
      {
        websocket = new WebSocket(webSocketUrl);
        websocket.onopen    = function(event){};
        websocket.onclose   = function(event){setTimeout(initWebSocket, 2000);};
        websocket.onmessage = function(event){};
      }
      function onTouchStartAndEnd(value) 
      {
        websocket.send(value);
      }
          
      window.onload = initWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
    
  </body>
</html> 
)HTMLHOMEPAGE";



void handleRoot(AsyncWebServerRequest *request) //Para a raiz do servidor executa o HTML
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request)  //Para página não encontrada, escreve "arquivo não encontrada"
{
    request->send(404, "text/plain", "Arquivo não encontrado");
}


//Seleciona o tipo de evento do websocket e executa de acordo.
void onWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len){                      
  switch (type) 
  {
    case WS_EVT_CONNECT: //Caso conecte
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      //client->text(getRelayPinsStatusJson(ALL_RELAY_PINS_INDEX));
      break;
    case WS_EVT_DISCONNECT: //Caso desconecte
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA: //Caso envie dados
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);     
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}


void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();

    //wifiManager.setSTAStaticIPConfig(IPAddress(192,168,100,99), IPAddress(192,168,100,1), IPAddress(255,255,255,0));
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,254), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    
    if(WiFi.SSID() != "ESP_AP"){
      Serial.println("Não é ESP-Ap");

      server.on("/", HTTP_GET, handleRoot); //Caso o cliente entre na rota raiz, executa handleRoot
      server.onNotFound(handleNotFound);//Caso o cliente entre fora da rota raiz, executa handleNotFound
      
      ws.onEvent(onWebSocketEvent); //Quando houver uma mudança no wesocket executa a função de callback onWebSocketEvent
      server.addHandler(&ws); //Adiciona websocket ao servidor web
      
      server.begin(); //Inicia o servidor wev
      Serial.println("HTTP server started");
    }
}

void loop() {
    // put your main code here, to run repeatedly:
    if ( digitalRead(PIN_AP) == HIGH ){
      Serial.println(WiFi.status());
      WiFi.disconnect(true);
      Serial.println("Iniciando novo ap");
      esp_wifi_restore();
      delay(2000);
      ESP.restart();
    }

    if(WiFi.SSID() != "ESP_AP"){
      ws.cleanupClients(); //Limpa os clientes, caso exceda o número máximo
    }
}