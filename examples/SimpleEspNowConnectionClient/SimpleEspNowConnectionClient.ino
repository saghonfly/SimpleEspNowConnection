/*
  SimpleEspNowConnectionClient

  Basic EspNowConnection Client implementation

  HOWTO Arduino IDE:
  - Prepare two ESP8266 or ESP32 based devices (eg. WeMos)
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
  Modified 14 Mai 2020
  By Erich O. Pintar

  https://github.com/saghonfly/SimpleEspNowConnection

*/


#include "SimpleEspNowConnection.h"

SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::CLIENT);

String inputString;
String serverAddress;

bool sendBigMessage()
{
  char bigMessage[] = "\
One, remember to look up at the stars and not down at your feet. Two, never give up work. Work gives you meaning and purpose and \
life is empty without it. Three, if you are lucky enough to find love,\
remember it is there and don't throw it away.\
\
A human being is a part of the whole called by us universe, \
a part limited in time and space. He experiences himself, \
his thoughts and feeling as something separated from the rest, \
a kind of optical delusion of his consciousness. \
This delusion is a kind of prison for us, restricting us to our personal \
desires and to affection for a few persons nearest to us. Our task must be \
to free ourselves from this prison by widening our circle of compassion to \
embrace all living creatures and the whole of nature in its beauty.\
\
Fantasy is escapist, and that is its glory. \
If a soldier is 1imprisioned by the enemy, don't we consider \
it his duty to escape?. . .If we value the freedom of mind and soul, \
if we're partisans of liberty, then it's our plain duty to escape, \
and to take as many people with us as we can!\
\
Fantasy is escapist, and that is its glory. \
If a soldier is 2imprisioned by the enemy, don't we consider \
it his duty to escape?. . .If we value the freedom of mind and soul, \
if we're partisans of liberty, then it's our plain duty to escape, \
and to take as many people with us as we can!\
\
das sind die letzten zeichen\
";  

 return(simpleEspConnection.sendMessage((uint8_t *)bigMessage, strlen(bigMessage)+1));  
}

void OnSendError(uint8_t* ad)
{
  Serial.println("SENDING TO '"+simpleEspConnection.macToStr(ad)+"' WAS NOT POSSIBLE!");
}

void OnMessage(uint8_t* ad, const uint8_t* message, size_t len)
{
  Serial.println("MESSAGE:"+String((char *)message));
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

  simpleEspConnection.begin(true);
  simpleEspConnection.setPairingBlinkPort(2);  

   serverAddress = "ECFABCC08CDA"; // Test if you know the server
   simpleEspConnection.setServerMac(serverAddress);
  simpleEspConnection.onNewGatewayAddress(&OnNewGatewayAddress);    
  simpleEspConnection.onSendError(&OnSendError);  
  simpleEspConnection.onMessage(&OnMessage);  
  
  Serial.println(WiFi.macAddress());  
}

void loop() 
{
  simpleEspConnection.loop();
  
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
      else if(inputString == "send")
      {
        if(!simpleEspConnection.sendMessage((uint8_t *)"This comes from the Client", 26))
        {
          Serial.println("SENDING TO '"+serverAddress+"' WAS NOT POSSIBLE!");
        }
      }
      else if(inputString == "bigsend")
      {
        if(!sendBigMessage())
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
