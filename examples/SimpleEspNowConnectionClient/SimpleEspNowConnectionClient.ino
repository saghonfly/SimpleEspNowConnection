/*
  SimpleEspNowConnectionClient

  Basic EspNowConnection Client implementation

  HOWTO Arduino IDE:
  - Prepare two ESP8266 based devices (eg. WeMos)
  - Start two separate instances of Arduino IDE and load 
    on the first one the 'SimpleEspNowConnectionServer.ino' and
    on the second one the 'SimpleEspNowConnectionClient.ino' sketch and upload 
    these to the two ESP devices.
  - Start the 'Serial Monitor' in both instances and set baud rate to 9600
  - Type 'startpair' into the edit box of both 'Serial Monitors' and hit Enter key (or press 'Send' button)
  - After devices are paired, type 'sendtest' into the edit box 
    of the 'Serial Monitor' and hit Enter key (or press 'Send' button)

  - You can use multiple clients which can be connected to one server

  Created 04 Mai 2020
  By Erich O. Pintar
  Modified 12 Mai 2020
  By Erich O. Pintar

  https://github.com/saghonfly/SimpleEspNowConnection

*/


#include "SimpleEspNowConnection.h"

SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::CLIENT);

String inputString;
String serverAddress;


void OnSendError(uint8_t* ad)
{
  Serial.println("SENDING TO '"+simpleEspConnection.macToStr(ad)+"' WAS NOT POSSIBLE!");
}

void OnMessage(uint8_t* ad, const char* message)
{
  Serial.println("MESSAGE:"+String(message));
}

void OnNewGatewayAddress(uint8_t *ga, String ad)
{  
  Serial.println("New GatewayAddress '"+ad+"'");

  simpleEspConnection.setServerMac(ga);
}

void setup() 
{
  Serial.begin(9600);
  Serial.println();

  simpleEspConnection.begin();
//  simpleEspConnection.setPairingBlinkPort(2);  

 //  serverAddress = "ECFABCC08CDA"; // Test if you know the server
 //  simpleEspConnection.setServerMac(serverAddress);
  simpleEspConnection.onNewGatewayAddress(&OnNewGatewayAddress);    
  simpleEspConnection.onSendError(&OnSendError);  
  simpleEspConnection.onMessage(&OnMessage);  
  
  Serial.println(WiFi.macAddress());  
}

void loop() 
{
  yield();
  
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
        if(!simpleEspConnection.sendMessage("This comes from the Client"))
        {
          Serial.println("SENDING TO '"+serverAddress+"' WAS NOT POSSIBLE!");
        }
      }
      
      inputString = "";
    }
    else
    {
      inputString += inChar;
    }
  }
}
