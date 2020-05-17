/*
  SensorClientDigitalInput

  Basic EspNowConnection Client implementation for a sensor

  HOWTO Hardware Setup:
  - Mount two jumper/pushbuttons, one between GPIO13 and ground and one between
    GPIO12 and ground
  - Connect GPIO16 with RST
  - Take a look on "https://github.com/saghonfly/SimpleEspNowConnection/wiki/Hardware-Setup-'SensorClientDigitalInput'-Example"
  

  HOWTO Arduino IDE:
  - Prepare two ESP8266 based devices (eg. WeMos)
  - Start two separate instances of Arduino IDE and load 
    on the first one the 'SensorServer.ino' and
    on the second one the 'SensorClientDigitalInput.ino' sketch and upload 
    these to the two ESP devices.
  - Start the 'Serial Monitor' in both instances and set baud rate to 9600
  - Type 'startpair' into the edit box of the server. Hold the pairing button on the sensor and reset the device
  - After server and client are paired, you can change the sleep time of the client in the server
    by typing 'settimeout <seconds>' into the serial terminal. 
    This will than be send next time when sensor is up.
  
  - You can use multiple clients which can be connected to one server

  Created 11 Mai 2020
  By Erich O. Pintar
  Modified 17 Mai 2020
  By Erich O. Pintar

  https://github.com/saghonfly/SimpleEspNowConnection

*/
#include "SimpleEspNowConnection.h"

SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::CLIENT);

int pairingButton = 13;
int sensorButton = 12;
bool pairingMode = false;

String inputString;
String serverAddress;

int timeout = 15;

void OnSendError(uint8_t* ad)
{
  Serial.println("Sending to '"+simpleEspConnection.macToStr(ad)+"' was not possible!");  
}

void OnMessage(uint8_t* ad, const char* message)
{
  Serial.println("MESSAGE from server:"+String(message));

  if(String(message).substring(0,8) == "timeout:")
    timeout = atoi( String(message).substring(8).c_str() );

  writeConfig();   
}

void OnNewGatewayAddress(uint8_t *ga, String ad)
{  
  serverAddress = ad;

  simpleEspConnection.setServerMac(ga);

  Serial.println("Pairing mode finished...");

  pairingMode = false;

  writeConfig();
}

bool readConfig()
{
    if (!SPIFFS.exists("/setup.txt")) 
      return false;

    File configFile = SPIFFS.open("/setup.txt", "r");
    if (!configFile) 
      return false;

    for(int i=0;i<12;i++) //Read server address
      serverAddress += (char)configFile.read();

    timeout = configFile.read();    
    configFile.close();

    return true;
}

bool writeConfig()
{
    File configFile = SPIFFS.open("/setup.txt", "w");
    if (!configFile) 
      return false;

    configFile.print(serverAddress.c_str());
    configFile.write(timeout);

    configFile.close();

    return true;
}
    
void setup() 
{
  Serial.begin(9600);
  Serial.println();

  // start SPIFFS file system. Ensure, sketch is uploaded with FS support !!!
  if(!SPIFFS.begin())
    SPIFFS.format();
  
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
      Serial.println("!!! Server address not valid. Please pair first !!! (Press the pairing button during restart/reset)");
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
    
  if(millis() < 100)  // wait 100 milliseconds for message from server otherwise go to sleep
    return;

  Serial.printf("Going to sleep...I was up for %i ms...will come back in %d seconds\n", millis(), timeout); 

  ESP.deepSleep(timeout * 1000000, RF_NO_CAL); // deep sleep for 10 seconds
  delay(100);          
}
