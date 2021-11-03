#include <EEPROM.h>

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