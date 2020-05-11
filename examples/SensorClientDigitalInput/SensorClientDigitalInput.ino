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

String readServerAddressFromEEPROM()
{
  String serverAddress;
  
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

void OnMessage(uint8_t* ad, const char* message)
{
  Serial.println("MESSAGE:"+String(message));
}

void OnNewGatewayAddress(uint8_t *ga, String ad)
{  
//  Serial.println("New GatewayAddress '"+ad+"'");

  writeServerAddressToEEPROM((char *)ad.c_str());

  simpleEspConnection.setServerMac(ga);
}

void setup() 
{
  Serial.begin(9600);
  Serial.println();

  simpleEspConnection.begin();
  simpleEspConnection.setPairingBlinkPort(2);  
  simpleEspConnection.setServerMac(readServerAddressFromEEPROM()); // set the server address which is stored in EEPROM
  simpleEspConnection.onNewGatewayAddress(&OnNewGatewayAddress);    
  simpleEspConnection.onMessage(&OnMessage);  
  
  // make the pairing pin to input:
  pinMode(pairingButton, INPUT_PULLUP);

  // Check if button is pressed when started
  int pairingButtonState = digitalRead(pairingButton);
  
  // start pairing if button is pressed
  if(pairingButtonState == 0)
  {
    simpleEspConnection.startPairing();
  }
  else
  {
    // make the sensor pin to input:
    pinMode(sensorButton, INPUT_PULLUP);

    String sensorButtonState = digitalRead(sensorButton) == 0 ? "SENSOR:PRESSED" : "SENSOR:UNPRESSED";

    if(simpleEspConnection.sendMessage((char *)sensorButtonState.c_str(), 100))
    {
      Serial.println("Sensor state successfully sent");
    }
    else
    {
      Serial.println("Timeout at sent");
    }     
  }
}

void loop() 
{
  if(millis() < 500)  // wait half a second for message from server
    return;

  ESP.deepSleep(10 * 1000000, RF_NO_CAL); // deep sleep for 10 seconds
  delay(100);          
}
