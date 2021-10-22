#include <EEPROM.h>
#define BACKWARD = 0;
#define FORWARD = 1;

void main() {
    control = digitalRead();

    if(control){
        startMotor
    }
}

void IRAM_ATTR funcao_ISR() {

    startStopMotor();

}

void IRAM_ATTR endMotor() {

    digitalWrite(relayForward, LOW);
    digitalWrite(relayBackward, LOW);
    setMotorState(!state);

}

attachInterrupt(GPIO_BOTAO, funcao_ISR, RISING);

//Interrupção para controle e uma para página web.

attachInterrupt(GPIO_REED, endMotor, RISING);


//Usar uma reedswitch com interrupção, ou seja, cada vez que o portão chegar ao limite, inicial ou final, ele desliga os reles e inverte o estado do portão.

void startStopMotor() {
    bool state = getMotorState();

    if(state == FORWARD && !motorRunning){ //FORWARD

        digitalWrite(relayForward, HIGH);
        digitalWrite(relayBackward, LOW);
        motorRunning = true;
        
    }else if(state == BACKWARD && !motorRunning){ //BACKWARD

        digitalWrite(relayForward,  LOW);
        digitalWrite(relayBackward, HIGH);
        motorRunning = true;      

    }


    digitalWrite(relayForward, LOW);
    digitalWrite(relayBackward, LOW);
    setMotorState(!state);
    motorRunning = false;

}


bool getMotorState() {
    return EEPROM[0];
}


void setMotorState(bool state) {

    if(state == FORWARD){
        EEPROM[0] = 1;
    }else if(state = BACKWARD){
        EEPROM[0] = 0;
    }
}


// To save date in eeprom we'll need at least 4bytes for unix timestamp. ainda precisa pegar o tempo no esp32
void saveInEeprom() {

    for (int index = 0 ; index < EEPROM.length() ; index++) {

    if(EEPROM[index] == 0){
        EEPROM[ index ] = firstByte;
        EEPROM[ index + 1] = secondByte;
        EEPROM[ index + 2] = 0;
    }
    

  }
}


//Possible work

for (int i = 3; i >= 0; ++i)
{
    byte b = (integervalue >> 8 * i) & 0xFF;
    send(b);
}

//Other possible working solution

#include <vector>
using namespace std;

vector<unsigned char> intToBytes(int paramInt)
{
     vector<unsigned char> arrayOfByte(4);
     for (int i = 0; i < 4; i++)
         arrayOfByte[3 - i] = (paramInt >> (i * 8));
     return arrayOfByte;
}