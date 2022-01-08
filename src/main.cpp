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
#include <Ultrasonic.h>
#include "time.h"
#include <ArduinoJson.h>

#include <vector>

using namespace std;

vector<int> arr;


// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

unsigned int epochTime; 

unsigned int getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}



#define BACKWARD 0
#define FORWARD 1

#define OPEN_CLOSE 0
#define HISTORY 1

#define TRIGGER 13
#define ECHO 12
Ultrasonic UltrasonicSensor(TRIGGER, ECHO);
int gateWidth = 100;
unsigned long waitTime = (1000 * 60 * 10); //Tempo em milissegundos, é multiplicado por 1000 e por 60 para mostrar o tempo em minutos.
unsigned long lastOpenTime;
bool autoClose = false;

AsyncWebServer server(80); //Declara um servidor com porta 80
AsyncWebSocket ws("/ws"); //Declara um metodo para socket cliente


bool motorRunning = false;
int relayForward = 26;
int relayBackward = 25;
int GPIO_BOTAO = 4;
int GPIO_REED = 21;

int wifiPin = 23;

bool interruption = false;
bool endCourse = false;
bool debounce = false;

unsigned long saveDebounceTimeout = 0;
 
unsigned long DEBOUNCETIME = 200;


//Página root do HTML do servidor
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <meta charset="UTF-8">
    <style>
      html{
        height: 100%;
        margin: 0;
      }
      body {
        margin: 0;
      }
     .flex {
        display: flex;
        justify-content: center;
        align-items: center;
     } 
     div{
      height: auto;
      width: 100%;
    }
    div:active.controle {
      transform: translate(5px,5px);
      box-shadow: none; 
    }

    div .historico{
      padding: 2%;
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
    table, th, td {
      border: 1px solid black;
    }

    table{
      width: 100%;
    }

    @media only screen and (max-width: 600px) {
      html{
        height: 100%;
        width: 100%;
        margin: 0;
      }
      body {
        margin: 0;
        padding: 3%;
      }
      .flex{
        flex-direction: column;
      }
    }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
    <h1>Painel de controle portão eletrônico</h1>
    <div class=flex>
      
      <div ontouchstart='onTouchStartAndEnd("0")' onclick='onTouchStartAndEnd("0")' class="controle">
        <h2>Abrir/Fechar portão</h2>
        <img src="https://cdn.leroymerlin.com.br/products/controle_remoto_led_para_portao_command_preto_rcg_89057052_0002_600x600.jpg" width="250px" alt="Controle remoto digital">
      </div>
  
      <div class="historico">
        <h2>Histórico de abertura</h2>
        <table>
          <tr>
            <th>Modo</th>
            <th>Hora</th>
            <th>Data</th>
          </tr>
          <tr>
            <td>Abriu</td>
            <td>12:55</td>
            <td>24/10/2021</td>
          </tr>
          <tr>
            <td>Fechou</td>
            <td>12:56</td>
            <td>24/10/2021</td>
          </tr>
        </table>
        </table>
      </div>     
      
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


#include <EEPROM.h> //ok boomer

//eeprom addres save in the address 1;
char intToByte(int n){
    char bytes[4];

    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] = (n >> 16) & 0xFF;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;

    return bytes;

}

int byteToInt(unsigned char bytes[4]) {
    int num;


    for(int i = 0; i < 4; i++){
        num <<= 8;
        num |= bytes[i];
    }

    return num;
}

// To save date in eeprom we'll need at least 4bytes for unix timestamp. ainda precisa pegar o tempo no esp32
void saveInEeprom(unsigned char bytes[4], int type) {

    int lastAddress = EEPROM.read(1);

    lastAddress += 6;

    //Para ir até o endereço 4092 no máximo, utilizando-se de 4096 bytes
    if(lastAddress > 4086) lastAddress = 6;

    EEPROM.write(lastAddress, 255);
    EEPROM.write((lastAddress + 1), bytes[0]);
    EEPROM.write((lastAddress + 2), bytes[1]);
    EEPROM.write((lastAddress + 3), bytes[2]);
    EEPROM.write((lastAddress + 4), bytes[3]);
    EEPROM.write((lastAddress + 5), type);
    EEPROM.write(1, lastAddress);

}

void readEeprom(){

    unsigned char bytes[4] = {};
    int type[] = {};
    int times[] = {};
    int contador = 0;

    for (int index = 1 ; index < 4595 ; index++) {
        if(EEPROM.read(index) == 255){
            bytes[0] = EEPROM.read(index + 1);
            bytes[1] = EEPROM.read(index + 2);
            bytes[2] = EEPROM.read(index + 3);
            bytes[3] = EEPROM.read(index + 4);
            type[contador] = EEPROM.read(index + 5);
            contador++;

            int epochtime = byteToInt(bytes);

            arr.push_back(epochtime);

        }
    }

}


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

    epochTime = getTime();
    

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
      lastOpenTime = millis();
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
        Serial.println("default");
      break;
  }
}



void handleRoot(AsyncWebServerRequest *request) //Para a raiz do servidor executa o HTML
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleHistory(AsyncWebServerRequest *request) //Para a raiz do servidor executa o HTML
{
  //request->send_P(200, "application/json", json);
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
     Serial.printf("Pong");
    case WS_EVT_ERROR: 
    Serial.printf("Erro");
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
    pinMode(wifiPin, OUTPUT);
    pinMode(GPIO_BOTAO, INPUT);
    pinMode(GPIO_REED, INPUT);

    attachInterrupt(digitalPinToInterrupt(GPIO_BOTAO), funcao_ISR, RISING);
    attachInterrupt(digitalPinToInterrupt(GPIO_REED), endMotor, RISING);


    configTime(0, 0, ntpServer);

    if(digitalRead(wifiPin)){
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
    }
    
    if(WiFi.SSID() != "ESP_AP"){
      Serial.println("Não é ESP-Ap");

      server.on("/", HTTP_GET, handleRoot); //Caso o cliente entre na rota raiz, executa handleRoot
      server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument json(1024);

        

        json["status"] = "ok";
        json["ssid"] = WiFi.SSID();
        json["ip"] = WiFi.localIP().toString();
        serializeJson(json, *response);
        request->send(response);
      }); //Caso o cliente entre na rota raiz, executa handleRoot
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
      Serial.println("controle");
      startStopMotor();
      interruption = false;
    }
    
    if(endCourse){
      saveDebounceTimeout = millis();
      endCourse = false;
      debounce = true;
    }

    //se o tempo passado foi maior que o configurado para o debounce e o número de interrupções ocorridas é maior que ZERO (ou seja, ocorreu alguma), realiza os procedimentos
    if( (millis() - saveDebounceTimeout) > DEBOUNCETIME && digitalRead(21) && debounce){
      
      Serial.println("reed");
      bool state = getMotorState();
      digitalWrite(relayForward, LOW);
      digitalWrite(relayBackward, LOW);
      setMotorState(!state);
      motorRunning = false;
      debounce = false;
      endCourse = false;
    }

    if( (millis() - lastOpenTime) > waitTime){
      float CmDistance = UltrasonicSensor.convert(UltrasonicSensor.timing(), Ultrasonic::CM);

      Serial.print("Ultrasonic: ");
      Serial.println(CmDistance);

      if(CmDistance >= gateWidth && !autoClose){
        Serial.println("ultrassonico");
        startStopMotor();
        autoClose = true;
      }
    }
    
  
    delay(100);

}