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
#include <EEPROM.h>


#define BACKWARD 0
#define FORWARD 1

#define OPEN_CLOSE 0
#define HISTORY 1


AsyncWebServer server(80); //Declara um servidor com porta 80
AsyncWebSocket ws("/ws"); //Declara um metodo para socket cliente


bool motorRunning = false;
int relayForward = 26;
int relayBackward = 25;
int GPIO_BOTAO = 4;
int GPIO_REED = 21;

bool interruption = false;
bool endCourse = false;


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
            <th>Acionamentos</th>
          </tr>
          <tr class="alternative">
            <td ontouchstart='onTouchStartAndEnd("0")' class="arrows">Abrir/fechar portão</td>
          </tr>
      </table>
  
      <table id="mainTable" style="display:inline-block;margin:auto;table-layout:fixed">
        <tr>
          <th>Alternative buttons</th>
        </tr>
        <tr class="alternative">
          <td ontouchstart='onTouchStartAndEnd("1")' class="arrows">Histórico</td>
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

//pega o valor de estado da eeprom
bool getMotorState() {
  return EEPROM.read(0);
}


//Salva estado de abertura como um bool na eprrom
void setMotorState(bool state) {

    if(state == FORWARD){
      EEPROM.write(0, FORWARD);
    }else if(state == BACKWARD){
      EEPROM.write(0, BACKWARD);
    }

    EEPROM.commit();
}


//Usar uma reedswitch com interrupção, ou seja, cada vez que o portão chegar ao limite, inicial ou final, ele desliga os reles e inverte o estado do portão.

void startStopMotor() {
    bool state = getMotorState();
    Serial.println("startStopMotor");
    Serial.println(state);

    if(state == FORWARD && !motorRunning){ //FORWARD

        digitalWrite(relayForward, HIGH);
        digitalWrite(relayBackward, LOW);
        motorRunning = true;
        
    }else if(state == BACKWARD && !motorRunning){ //BACKWARD

        digitalWrite(relayForward,  LOW);
        digitalWrite(relayBackward, HIGH);
        motorRunning = true;      

    }else{
      digitalWrite(relayForward, LOW);
      digitalWrite(relayBackward, LOW);
      setMotorState(!state);
      motorRunning = false;
      Serial.println(getMotorState());
    }

}


void processAction(String inputValue)
{
  Serial.printf("Got value as %s %d\n", inputValue.c_str(), inputValue.toInt());

  switch(inputValue.toInt())
  {

    case OPEN_CLOSE:
    Serial.println("oppen-Close");

      startStopMotor();
      break;
  
    case HISTORY:
      
       
      break;
    
  
    default:
        
      break;
  }
}





void handleRoot(AsyncWebServerRequest *request) //Para a raiz do servidor executa o HTML
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleHistory(AsyncWebServerRequest *request) //Para a raiz do servidor executa o HTML
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
        processAction(myData.c_str());       
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}


void IRAM_ATTR funcao_ISR() {

  interruption = true;

}

void IRAM_ATTR endMotor() {
  endCourse = true;
}




void setup() {
    // put your setup code here, to run once:
    


//Interrupção para controle e uma para página web.

    

    EEPROM.begin(1);


    Serial.begin(9600);

    pinMode(relayForward, OUTPUT);
    pinMode(relayBackward, OUTPUT);
    pinMode(GPIO_BOTAO, INPUT);
    pinMode(GPIO_REED, INPUT);

    attachInterrupt(digitalPinToInterrupt(GPIO_BOTAO), funcao_ISR, RISING);
    attachInterrupt(digitalPinToInterrupt(GPIO_REED), endMotor, RISING);
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
      server.on("/history", HTTP_GET, handleHistory); //Caso o cliente entre na rota raiz, executa handleRoot
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

    if(interruption){ 
      startStopMotor();
      interruption = false;
    }
    
    if(endCourse){ 
      bool state = getMotorState();
      digitalWrite(relayForward, LOW);
      digitalWrite(relayBackward, LOW);
      setMotorState(!state);
      motorRunning = false;
      endCourse = false;
    }

}