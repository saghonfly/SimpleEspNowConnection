/*
  SensorClientDigitalInput

  Basic EspNowConnection Client implementation for a sensor

  HOWTO Hardware Setup:
  - Mount two jumper/pushbuttons, one between GPIO13 and ground and one between
    GPIO12 and ground
  - Connect GPIO16 with RST
  - Take a look on "https://github.com/saghonfly/SimpleEspNowConnection/wiki/Hardware-Setup-'SensorClientDigitalInput'-Example"
  

  HOWTO Arduino IDE:
  - Prepare two ESP8266 or ESP32 based devices (eg. WeMos) or mix an ESP8266 and an ESP32 device
  - Start two separate instances of Arduino IDE and load 
    on the first one the 'SensorServer.ino' and
    on the second one the 'SensorClientDigitalInput.ino' sketch and upload 
    these to the two ESP devices.
  - Start the 'Serial Monitor' in both instances and set baud rate to 9600
  - Type 'startpair' into the edit box of the SensorServer. Hold the pairing button on the SensorClientDigitalInput device and reset the device
  - After server and client are paired, you can change the sleep time of the client in the server
    by typing 'settimeout <seconds>' into the serial terminal. 
    This will than be send next time when sensor is up.
  
  - You can use multiple clients which can be connected to one server

  Created 11 Mai 2020
  By Erich O. Pintar
  Modified 06 Oct 2020
  By Erich O. Pintar

  https://github.com/saghonfly/SimpleEspNowConnection

*/
#include "SimpleEspNowConnection.h"

#include <EEPROM.h>

SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::CLIENT);

int pairingButton = 13;
//int pairingButton = 27;
int sensorButton = 12;
bool pairingMode = false;

String inputString;
uint8_t serverAddress[6];
uint8_t timeout = 10;

void OnSendError(uint8_t* ad)
{
  Serial.println("Sending to '"+simpleEspConnection.macToStr(ad)+"' was not possible!");  
}

void OnMessage(uint8_t* ad, const uint8_t* message, size_t len)
{
  Serial.printf("MESSAGE from server:%s\n", (char *)message);

  if(String((char *)message).substring(0,8) == "timeout:")
    timeout = atoi( String((char *)message).substring(8).c_str() );

  writeConfig();   
  Serial.println("New timeout written to configuration");  
}

void OnNewGatewayAddress(uint8_t *ga, String ad)
{  
  memcpy(serverAddress, ga, 6);

  simpleEspConnection.setServerMac(ga);

  Serial.println("Pairing mode finished...");

  pairingMode = false;

  writeConfig();
}

bool readConfig()
{
  for(int i=0;i<6;i++)
  {
    serverAddress[i] = EEPROM.read(i);
  }

  timeout = EEPROM.read(6);
  return true;
}

bool writeConfig()
{
  Serial.printf("Write %02X%02X%02X%02X%02X%02X to EEPROM\n", serverAddress[0],
                                                   serverAddress[1],
                                                   serverAddress[2],
                                                   serverAddress[3],
                                                   serverAddress[4],
                                                   serverAddress[5]);
                                                   
  for(int i=0;i<6;i++)
  {
    EEPROM.write(i,serverAddress[i]);
  }
  EEPROM.write(6,timeout);
  EEPROM.commit();

  return true;
}
    
void setup() 
{
  Serial.begin(9600);
  Serial.println();

  EEPROM.begin(13);
  
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
    if(!readConfig())
    {
        Serial.println("!!! Server address not valid. Please pair first !!!");
        return;
    }  

    if(!simpleEspConnection.setServerMac(serverAddress)) // set the server address which is stored in EEPROM
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
  simpleEspConnection.loop();
  
  if(pairingMode) // do not go to sleep if pairing mode is ongoing
    return;
    
  if(millis() < 100)  // wait 100 millisecond for message from server otherwise go to sleep
    return;

  if(!simpleEspConnection.isSendBufferEmpty())
    return;

  Serial.printf("Going to sleep...I was up for %i ms...will come back in %d seconds\n", millis(), timeout); 

#if defined(ESP32)
  esp_sleep_enable_timer_wakeup(timeout * 1000000);
  esp_deep_sleep_start();
#else
  ESP.deepSleep(timeout * 1000000, RF_NO_CAL); // deep sleep for 10 seconds
  delay(100);
#endif          
}
