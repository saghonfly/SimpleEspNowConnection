#include "SimpleEspNowConnection.h"

SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::SERVER);

String inputString;
String clientAddress = "ECFABC0CE7A2";


void OnMessage(uint8_t* ad, const char* message)
{
  Serial.println("MESSAGE:"+String(message));
}

void OnPaired(uint8_t *ga, String ad)
{
  Serial.println("EspNowConnection : Client '"+ad+"' paired! ");
}
  
void setup() 
{
  Serial.begin(9600);

  simpleEspConnection.begin();
  simpleEspConnection.setPairingBlinkPort(2);
  simpleEspConnection.onMessage(&OnMessage);  
  simpleEspConnection.onPaired(&OnPaired);  

  Serial.println(WiFi.macAddress());    
}

void loop() 
{
  while (Serial.available()) 
  {
    char inChar = (char)Serial.read();
    if (inChar == '\n') 
    {
      Serial.println(inputString);

      if(inputString == "startpair")
      {
        simpleEspConnection.startPairing(30);
      }
      else if(inputString == "endpair")
      {
        simpleEspConnection.endPairing();
      }
      else if(inputString == "changepairingmac")
      {
        uint8_t np[] {0xCE, 0x50, 0xE3, 0x15, 0xB7, 0x33};
        
        simpleEspConnection.setPairingMac(np);
      }      
      else if(inputString == "sendtest")
      {
         simpleEspConnection.sendMessage("das kommt vom server", clientAddress);
      }
      
      inputString = "";
    }
    else
    {
      inputString += inChar;
    }
  }
}
