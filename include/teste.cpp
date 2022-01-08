#include <EEPROM.h>

//eeprom addres save in the address 1;

// To save date in eeprom we'll need at least 4bytes for unix timestamp. ainda precisa pegar o tempo no esp32
void saveInEeprom(unsigned char bytes[4], int type) {

    int lastAddress = EEPROM.read(1);

    lastAddress += 6;

    //Para ir até o endereço 4092 no máximo, utilizando-se de 4096 bytes
    if(lastAddress > 4086) lastAddress = 6;

    EEPROM.write(index, 255);
    EEPROM.write((index + 1), bytes[0]);
    EEPROM.write((index + 2), bytes[1]);
    EEPROM.write((index + 3), bytes[2]);
    EEPROM.write((index + 4), bytes[3]);
    EEPROM.write((index + 5), type);
    EEPROM.write(1, lastAddress);

}

void readEeprom(){

    unsigned char bytes[4];
    int type[];
    int times[];

    for (int index = 1 ; index < 4595 ; index++) {
        if(EEPROM.read(index) == 255){
            bytes[0] = EEPROM.read(index + 1);
            bytes[1] = EEPROM.read(index + 2);
            bytes[2] = EEPROM.read(index + 3);
            bytes[3] = EEPROM.read(index + 4);
            type = EEPROM.read(index + 5);

            int epochtime = byteToInt(bytes);

        }
    }

}

unsigned char intToByte[](int n){
    unsigned char bytes[4];
    unsigned long n;

    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] = (n >> 16) & 0xFF;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;

    return bytes;

}

int byteToInt(unsigned char bytes[4]) {


    for(int i = 0; i < 4; i++){
        num <<= 8;
        num |= bytes[i];
    }

    return num;
}