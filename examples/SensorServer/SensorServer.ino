/*
  SimpleEspNowConnectionServer

  Basic EspNowConnection Server implementation

  HOWTO Arduino IDE:
  - Prepare two ESP8266 based devices (eg. WeMos)
  - Start two separate instances of Arduino IDE and load 
    on the first one the 'SensorServer.ino' and
    on the second one the 'SensorClient.ino' sketch and upload 
    these to the two ESP devices.
  - Start the 'Serial Monitor' in both instances and set baud rate to 9600
  - Type 'startpair' into the edit box of both 'Serial Monitors' and hit Enter key (or press 'Send' button)
  - After devices are paired, type 'sendtest' into the edit box 
    of the 'Serial Monitor' and hit Enter key (or press 'Send' button)

  - You can use multiple clients which can be connected to one server

  Created 11 Mai 2020
  By Erich O. Pintar
  Modified 11 Mai 2020
  By Erich O. Pintar

  https://github.com/saghonfly/SimpleEspNowConnection

*/

#include "SimpleEspNowConnection.h"

static SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::SERVER);
String inputString, clientAddress;

void OnSendError(uint8_t* ad)
{
  Serial.println("Sending to '"+simpleEspConnection.macToStr(ad)+"' was not possible!");  
}

void OnMessage(uint8_t* ad, const char* message)
{
  Serial.println("Client '"+simpleEspConnection.macToStr(ad)+"' has sent me '"+String(message)+"'");
}

void OnPaired(uint8_t *ga, String ad)
{
  Serial.println("EspNowConnection : Client '"+ad+"' paired! ");

  simpleEspConnection.endPairing();  
}

void OnConnected(uint8_t *ga, String ad)
{
  Serial.println("I will send now a setup message to sensor client...");

  simpleEspConnection.sendMessage("Please reconfigure with following parameter blablabla...",ad );
}

void setup() 
{
  Serial.begin(9600);
  Serial.println();
   clientAddress = "ECFABC0CE7A2"; // Test if you know the client

  simpleEspConnection.begin();
  simpleEspConnection.setPairingBlinkPort(2);
  simpleEspConnection.onMessage(&OnMessage);  
  simpleEspConnection.onPaired(&OnPaired);  
  simpleEspConnection.onSendError(&OnSendError);  
  simpleEspConnection.onConnected(&OnConnected);  
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
        Serial.println("Pairing started...");
        simpleEspConnection.startPairing(120);
      }
            
      inputString = "";
    }
    else
    {
      inputString += inChar;
    }
  }  
}
