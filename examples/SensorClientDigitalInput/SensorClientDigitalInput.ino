/*
  SimpleEspNowConnectionServer

  Basic EspNowConnection Client implementation for a sensor

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
#include <EEPROM.h>
#include "SimpleEspNowConnection.h"

SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::CLIENT);

int pairingButton = 13;
int sensorButton = 12;
bool pairingMode = false;

String inputString;
String serverAddress;

String readServerAddressFromEEPROM()
{  
  EEPROM.begin(12);
  
  for (int i = 0; i < 12; ++i)
  {
    serverAddress += char(EEPROM.read(i));
  }
  
  EEPROM.end();

  Serial.println("readServerAddressFromEEPROM '"+serverAddress+"'");

  return serverAddress;
}

void writeServerAddressToEEPROM(char *serverAddress)
{
  Serial.println("writeServerAddressToEEPROM '"+String(serverAddress)+"'");

  EEPROM.begin(12);

  for (int i = 0; i < 12; ++i)
  {
    EEPROM.write(i, serverAddress[i]);
  }
 
  EEPROM.commit();
  EEPROM.end();
}

void OnSendError(uint8_t* ad)
{
  Serial.println("Sending to '"+simpleEspConnection.macToStr(ad)+"' was not possible!");  
}

void OnMessage(uint8_t* ad, const char* message)
{
  Serial.println("MESSAGE from server:"+String(message));
}

void OnNewGatewayAddress(uint8_t *ga, String ad)
{  
  writeServerAddressToEEPROM((char *)ad.c_str());

  simpleEspConnection.setServerMac(ga);

  Serial.println("Pairing mode finished...");

  pairingMode = false;
}

void setup() 
{
  Serial.begin(9600);
  Serial.println();

  simpleEspConnection.begin();
  simpleEspConnection.setPairingBlinkPort(2);  
  simpleEspConnection.onNewGatewayAddress(&OnNewGatewayAddress);    
  simpleEspConnection.onMessage(&OnMessage);  
  simpleEspConnection.onSendError(&OnSendError);  

  // make the pairing pin to input:
  pinMode(pairingButton, INPUT_PULLUP);

  // Check if button is pressed when started
  int pairingButtonState = digitalRead(pairingButton);
  
  // start pairing if button is pressed
  if(pairingButtonState == 0)
  {
    Serial.println("Pairing mode ongoing...");
    
    pairingMode = true;
    simpleEspConnection.startPairing();
  }
  else
  {
    if(!simpleEspConnection.setServerMac(readServerAddressFromEEPROM())) // set the server address which is stored in EEPROM
    {
      Serial.println("!!! Server address not valid. Please pair first !!!");
      return;
    }
    
    // make the sensor pin to input:
    pinMode(sensorButton, INPUT_PULLUP);

    String sensorButtonState = digitalRead(sensorButton) == 0 ? "SENSOR:PRESSED" : "SENSOR:UNPRESSED";

    simpleEspConnection.sendMessage((char *)sensorButtonState.c_str());
  }
}

void loop() 
{  
  if(pairingMode) // do not go to sleep if pairing mode is ongoing
    return;
    
  if(millis() < 100)  // wait half a second for message from server otherwise go to sleep
    return;

  Serial.printf("Going to sleep...I was up for %i ms...will come back in 10 seconds\n", millis()); 

  ESP.deepSleep(10 * 1000000, RF_NO_CAL); // deep sleep for 10 seconds
  delay(100);          
}
