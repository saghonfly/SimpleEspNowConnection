/*
  SimpleEspNowConnection.cpp - A simple EspNow connection and pairing class.
  Erich O. Pintar
  https://pintarweb.net
  
  Version : 1.0.3
  
  Created 04 Mai 2020
  By Erich O. Pintar
  Modified 15 Mai 2020
  By Erich O. Pintar
*/

#include "SimpleEspNowConnection.h"

#define DEBUG

SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject::DeviceBufferObject()
{
}

SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject::DeviceBufferObject(long id, int counter, int packages, uint8_t *device, char* message, int len)
{
	_id = id;
	memcpy(_device, device, 6);
	memcpy(_message, message, len);
	_len = len;
	_counter = counter;
	_packages = packages;	
}

SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject::DeviceBufferObject(long id, int counter, int packages, uint8_t *device)
{
	_id = id;
	memcpy(_device, device, 6);
	memset(_message, '\0', 235);
	_len = 0;
	_counter = counter;
	_packages = packages;
}

SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject::~DeviceBufferObject()
{
}

SimpleEspNowConnection::DeviceMessageBuffer::DeviceMessageBuffer()
{
    for(int i = 0; i<MaxBufferSize; i++)
		_dbo[i] = NULL;
}

SimpleEspNowConnection::DeviceMessageBuffer::~DeviceMessageBuffer()
{
}

SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject* SimpleEspNowConnection::DeviceMessageBuffer::getNextBuffer()
{
    for(int i = 0; i<MaxBufferSize; i++)
    {
		if(_dbo[i] != NULL)
		{
			return _dbo[i];
		}
	}
	
	return NULL;
}

bool SimpleEspNowConnection::DeviceMessageBuffer::createBuffer(uint8_t *device, long id, int packages)
{
	int counter = 0;

    for(int i = 0; i<MaxBufferSize; i++)
    {
		if(_dbo[i] == NULL)
		{
			_dbo[i] = new DeviceBufferObject(id, counter+1, packages, device);

			counter++;
			
			if(counter >= packages)
				break;
		}
    }		

	return true;
}

bool SimpleEspNowConnection::DeviceMessageBuffer::createBuffer(uint8_t *device, char* message)
{		
	int packages = strlen(message) / 235 + 1;
	int messagelen;
	int counter = 0;
	long id = millis();

    for(int i = 0; i<MaxBufferSize; i++)
    {
		if(_dbo[i] == NULL)
		{
			messagelen = strlen(message+(counter*235)) > 235 ? 235 : strlen(message+(counter*235));		

			_dbo[i] = new DeviceBufferObject(id, counter+1, packages, device, message+(counter*235), messagelen);
			
			counter++;
			
			if(counter >= packages)
				break;
		}
    }	

	return true;
}

void SimpleEspNowConnection::DeviceMessageBuffer::addBuffer(uint8_t *device, long id, uint8_t *buffer, int len, int package)
{
    for(int i = 0;i<MaxBufferSize; i++)
    {
      if(_dbo[i] != NULL && memcmp(_dbo[i]->_device, device, 6) == 0 && 
		 _dbo[i]->_counter == package+1 &&
		 _dbo[i]->_id == id)
		{
			memcpy(_dbo[i]->_message, buffer, len);
			_dbo[i]->_len = len;
		
		Serial.printf("memcpy len = %d\n", _dbo[i]->_len);

		
			break;
		}
		
	}	
	
}

char* SimpleEspNowConnection::DeviceMessageBuffer::getBuffer(uint8_t *device, int packages)
{
	//String b = "";
	char bu[packages*235+1];
	memset(bu, '\0', packages*235+1);
	
	int counter = 0;
	
    for(int i = 0;i<MaxBufferSize; i++)
    {
      if(_dbo[i] != NULL && memcmp(_dbo[i]->_device, device, 6) == 0)
      {
	//	b += String(_dbo[i]->_message);
		memcpy(bu+(counter*235), _dbo[i]->_message, _dbo[i]->_len);
				
		counter++;
		
		if(counter == packages)
			break;
      }
    }	
	
	return bu;
}

bool SimpleEspNowConnection::DeviceMessageBuffer::deleteBuffer(uint8_t *device)
{
    for(int i = 0;i<MaxBufferSize; i++)
    {
      if(_dbo[i] != NULL && memcmp(_dbo[i]->_device, device, 6) == 0)
      {
        delete _dbo[i];
        _dbo[i] = NULL;
      }
    }	
}

bool SimpleEspNowConnection::DeviceMessageBuffer::deleteBuffer(SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject* dbo)
{
    for(int i = 0; i<MaxBufferSize; i++)
    {
		if(_dbo[i] == dbo)
		{
			delete _dbo[i];
			_dbo[i] = NULL;
			
#ifdef DEBUG
			Serial.printf("dbo deleted = %d\n", i);
#endif			
			return true;
		}
	}
	
	return false;
}

SimpleEspNowConnection::SimpleEspNowConnection(SimpleEspNowRole role) 
{
	this->_role = role;
	simpleEspNowConnection = this;
	this->_pairingOngoing = false;
	memset(_serverMac,0,6);
	_openTransaction = false;
	_channel = 3;
	_lastSentTime = millis();
}

bool SimpleEspNowConnection::begin(bool supportLooping)
{	
	_supportLooping = supportLooping;

	WiFi.mode(WIFI_STA);	

	WiFi.persistent(false);
	WiFi.macAddress(_myAddress);	

	if (esp_now_init() != 0) 
	{
		Serial.println("Error initializing ESP-NOW");
		return false;
	}
	
#if defined(ESP8266)
	esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
#endif	

	esp_now_register_recv_cb(SimpleEspNowConnection::onReceiveData);

	
#if defined(ESP8266)
	esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) 
#elif defined(ESP32)		
	esp_now_register_send_cb([] (const uint8_t *mac, esp_now_send_status_t sendStatus)
#endif	
	{//this is the function that is called to send data
#ifdef DEBUG
	  Serial.printf("--- send_cb, send done, status = %i\n", sendStatus);
#endif	
		simpleEspNowConnection->_lastSentTime = millis();
		simpleEspNowConnection->_openTransaction = false;

		if(memcmp(mac, simpleEspNowConnection->_pairingMac, 6) != 0)
		{
			if(sendStatus != 0 && simpleEspNowConnection->_SendErrorFunction != NULL)
			{
			  simpleEspNowConnection->_SendErrorFunction((uint8_t*)mac);
			}
			if(sendStatus == 0 && simpleEspNowConnection->_SendDoneFunction != NULL)
			{
			  simpleEspNowConnection->_SendDoneFunction((uint8_t*)mac);
			}	 
		}		
	});
	
	if(this->_role == SimpleEspNowRole::SERVER)
	{
		initServer();
	}
	else
	{
		initClient();
	}
	
	return true;
}

bool SimpleEspNowConnection::setPairingBlinkPort(int pairingGPIO, bool invers)
{
	_pairingGPIO = pairingGPIO;
	_pairingInvers = invers;
	
    pinMode(_pairingGPIO, OUTPUT);
    digitalWrite(_pairingGPIO, _pairingInvers); 
	
	return true;
}

void SimpleEspNowConnection::pairingTickerServer()
{
#ifdef DEBUG
    Serial.println("EspNowConnection::Pairing request sent..."+
		String(simpleEspNowConnection->_pairingCounter+1)+"/"+
		String(simpleEspNowConnection->_pairingMaxCount));
#endif

	char sendMessage[13];
	long id = millis();
	
	sendMessage[0] = SimpleEspNowMessageType::PAIR;	// Type of message
	sendMessage[1] = 1;	// 1st package
	sendMessage[2] = 1;	// from 1 package. WIll be enhanced in one of the next versions
	memcpy(sendMessage+3, &id, 4);	

	memcpy(sendMessage+7, simpleEspNowConnection->_myAddress, 6);

#if defined(ESP32)
	memcpy(&simpleEspNowConnection->_clientMacPeerInfo.peer_addr, simpleEspNowConnection->_pairingMac, 6);
	esp_now_add_peer(&simpleEspNowConnection->_clientMacPeerInfo);
#endif


    esp_now_send(simpleEspNowConnection->_pairingMac, 
		(uint8_t *)sendMessage, 
		9);
	
#if defined(ESP32)
	esp_now_del_peer(simpleEspNowConnection->_pairingMac);
#endif
	
	if(simpleEspNowConnection->_pairingMaxCount > 0)
	{
		simpleEspNowConnection->_pairingCounter++;
		if(simpleEspNowConnection->_pairingCounter >= simpleEspNowConnection->_pairingMaxCount)
		{
			simpleEspNowConnection->endPairing();
		}
	}
}

void SimpleEspNowConnection::pairingTickerClient()
{
    digitalWrite(simpleEspNowConnection->_pairingGPIO, simpleEspNowConnection->_pairingInvers);
	simpleEspNowConnection->endPairing();
    simpleEspNowConnection->_pairingTickerBlink.detach();
	
#if defined(ESP8266)
    wifi_set_macaddr(STATION_IF, &simpleEspNowConnection->_myAddress[0]);
#elif defined(ESP32)	
	esp_wifi_set_mac(ESP_IF_WIFI_STA, &simpleEspNowConnection->_myAddress[0]);
#endif	

#ifdef DEBUG
  Serial.println("MAC set to : "+WiFi.macAddress());  
#endif
}

void SimpleEspNowConnection::pairingTickerLED()
{	
  if(simpleEspNowConnection->_pairingOngoing)
  {
    int state = digitalRead(simpleEspNowConnection->_pairingGPIO);
    digitalWrite(simpleEspNowConnection->_pairingGPIO, !state); 
  }
  else
  {
    digitalWrite(simpleEspNowConnection->_pairingGPIO, simpleEspNowConnection->_pairingInvers);
    simpleEspNowConnection->_pairingTickerBlink.detach();
  }
}

bool SimpleEspNowConnection::startPairing(int timeoutSec)
{	
	if(_pairingOngoing) return false;
	
	if(timeoutSec > 0 && timeoutSec < 5)
		timeoutSec = 5;
	
	if(this->_role == SimpleEspNowRole::SERVER)
	{
#ifdef DEBUG
		Serial.println("SimpleEspNowConnection::Server Pairing started");
#endif	

		_pairingOngoing = true;
		_pairingCounter = 0;
		if(timeoutSec > 0)
		{
			_pairingMaxCount = timeoutSec / 5;
		}
		else
		{
			_pairingMaxCount = 0;
		}

		_pairingTicker.attach(5.0, SimpleEspNowConnection::pairingTickerServer);   
		
		if(_pairingGPIO != -1)
		{
			_pairingTickerBlink.attach(0.5, pairingTickerLED);            
		}
	}
	else
	{
#ifdef DEBUG
		Serial.println("SimpleEspNowConnection::Client Pairing started");
#endif			
		
		_pairingOngoing = true;

#if defined(ESP8266)
		wifi_set_macaddr(STATION_IF, &_pairingMac[0]);
#elif defined(ESP32)	
		esp_wifi_set_mac(ESP_IF_WIFI_STA, &_pairingMac[0]);
#endif	

#ifdef DEBUG
		Serial.println("MAC set to : "+WiFi.macAddress());  
#endif

		if(timeoutSec == 0)
			timeoutSec = 30;
		else if(timeoutSec < 10)
			timeoutSec = 10;
		else if(timeoutSec > 120)
			timeoutSec = 120;

		_pairingTicker.attach(timeoutSec, SimpleEspNowConnection::pairingTickerClient);   

		if(_pairingGPIO != -1)
		{
			_pairingTickerBlink.attach(0.5, pairingTickerLED);            
		}
	}
	
	return true;
}

bool SimpleEspNowConnection::endPairing()
{
	_pairingOngoing = false;
    _pairingTicker.detach();
	_pairingTickerBlink.detach();

	if(_pairingGPIO != -1)
		digitalWrite(_pairingGPIO, _pairingInvers);
	
	
#ifdef DEBUG
	if(_role == SimpleEspNowRole::SERVER)
		Serial.println("EspNowConnection::Server Pairing endet");
	else
		Serial.println("EspNowConnection::Client Pairing endet");		
#endif	

	if(_PairingFinishedFunction != NULL)
	{
		_PairingFinishedFunction();
	}
	
	return true;
}

void SimpleEspNowConnection::prepareSendPackages(char* message, String address)
{
	uint8_t *mac = strToMac(address.c_str());
			
	deviceSendMessageBuffer.createBuffer(mac, message);	
}

bool SimpleEspNowConnection::sendMessage(char* message, String address)
{
	if( (_role == SimpleEspNowRole::SERVER && address.length() != 12 ) ||
		(_role == SimpleEspNowRole::CLIENT && _serverMac[0] == 0 ))
	{
		return false;
	}

	int packages = strlen(message) / 235 + 1;

	if(!_supportLooping && packages > 1)
		return false;

#ifdef DEBUG
	Serial.printf("Number of bytes %d, number of packages %d\n", strlen(message), packages);
#endif
	
	if(!_supportLooping)
		return sendMessageOld(message, address);
	
	if(_role == SimpleEspNowRole::CLIENT)
		address = macToStr(_serverMac);
	
	prepareSendPackages(message, address);
	
	return true;
}

bool SimpleEspNowConnection::sendPackage(long id, int package, int sum, char* message, int messagelen, uint8_t* address)
{
	char sendMessage[messagelen+7];

    Serial.printf("sendPackage begin %d\n", messagelen);
	sendMessage[0] = SimpleEspNowMessageType::DATA;	// Type of message
	sendMessage[1] = package;	
	sendMessage[2] = sum;	
	memcpy(sendMessage+3, &id, 4);	
	memcpy(sendMessage+7, message, messagelen);
	
	if(_role == SimpleEspNowRole::SERVER)
	{
#if defined(ESP32)
		memcpy(&simpleEspNowConnection->_clientMacPeerInfo.peer_addr, address, 6);
		esp_now_add_peer(&simpleEspNowConnection->_clientMacPeerInfo);
#elif defined(ESP8266)		
		esp_now_add_peer(address, ESP_NOW_ROLE_COMBO, simpleEspNowConnection->_channel, NULL, 0);
#endif
		esp_now_send(address, (uint8_t *) sendMessage, messagelen+7);
		_openTransaction = true;

		esp_now_del_peer(address);

		delete address;
	}
	else
	{		
		_openTransaction = true;
		esp_now_send(address, (uint8_t *) sendMessage, messagelen+7);
	}
		
	return true;
}

bool SimpleEspNowConnection::sendMessageOld(char* message, String address)
{
	if( (_role == SimpleEspNowRole::SERVER && address.length() != 12 ) ||
		(_role == SimpleEspNowRole::CLIENT && _serverMac[0] == 0 ) ||
		strlen(message) > 235)
	{
		return false;
	}

	char sendMessage[strlen(message)+7];
	long id = millis();

	sendMessage[0] = SimpleEspNowMessageType::DATA;	// Type of message
	sendMessage[1] = 1;	// 1st package
	sendMessage[2] = 1;	// from 1 package. WIll be enhanced in one of the next versions
	memcpy(sendMessage+3, &id, 4);	
	//sendMessage[strlen(message)+11] = 0;
	
	memcpy(sendMessage+7, message, strlen(message));

#if defined(ESP8266)
	esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) 
#elif defined(ESP32)		
	esp_now_register_send_cb([] (const uint8_t *mac, esp_now_send_status_t sendStatus)
#endif	
	{//this is the function that is called to send data
#ifdef DEBUG
	  Serial.printf("send_cb, send done, status = %i\n", sendStatus);
#endif	
	  esp_now_unregister_send_cb();

	  if(sendStatus != 0 && simpleEspNowConnection->_SendErrorFunction != NULL)
	  {
		  simpleEspNowConnection->_SendErrorFunction((uint8_t*)mac);
	  }
	  if(sendStatus == 0 && simpleEspNowConnection->_SendDoneFunction != NULL)
	  {
		  simpleEspNowConnection->_SendDoneFunction((uint8_t*)mac);
	  }	  	  
	});

	
	if(_role == SimpleEspNowRole::SERVER)
	{
		uint8_t *mac = strToMac(address.c_str());

#if defined(ESP32)
		memcpy(&simpleEspNowConnection->_clientMacPeerInfo.peer_addr, mac, 6);
		esp_now_add_peer(&simpleEspNowConnection->_clientMacPeerInfo);
#endif
		esp_now_send(mac, (uint8_t *) sendMessage, strlen(sendMessage));
#if defined(ESP32)
		esp_now_del_peer(mac);
#endif

		delete mac;
	}
	else
	{		
		esp_now_send(_serverMac, (uint8_t *) sendMessage, strlen(sendMessage));
	}
	
	return true;
}
uint8_t* SimpleEspNowConnection::strToMac(const char* str)
{
	if(strlen(str) != 12)
		return NULL;
	
	uint8_t* mac = new uint8_t[6];
	
	char buffer[2];
	
	for(int i = 0; i<6; i++)
	{
		strncpy ( buffer, str+i*2, 2 );
		
		mac[i] = strtol(buffer, NULL, 16);
	}
	
	return mac;
}

String SimpleEspNowConnection::macToStr(const uint8_t* mac)
{
	char macAddr[13];
	macAddr[12] = 0;
	
	sprintf(macAddr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);		

	return String(macAddr);
}


#if defined(ESP8266)
void SimpleEspNowConnection::onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len) 
#elif defined(ESP32)
void SimpleEspNowConnection::onReceiveData(const uint8_t *mac, const uint8_t *data, int len)
#endif
{
	if(len <= 7)
		return;
	
	char buffer[len-6];
	buffer[len-7] = 0;
	long id;
	
	memcpy(buffer, data+7, len-7);
	memcpy(&id, data+3, 4);
	
	if(simpleEspNowConnection->_role == SimpleEspNowRole::CLIENT &&
		simpleEspNowConnection->_pairingOngoing)
	{
		if(data[0] == SimpleEspNowMessageType::PAIR)			
		{
			if(simpleEspNowConnection->_NewGatewayAddressFunction)
			{
#if defined(ESP8266)
				wifi_set_macaddr(STATION_IF, &simpleEspNowConnection->_myAddress[0]);
#elif defined(ESP32)
				esp_wifi_set_mac(ESP_IF_WIFI_STA, &simpleEspNowConnection->_myAddress[0]);
#endif				
				simpleEspNowConnection->endPairing();
				simpleEspNowConnection->_NewGatewayAddressFunction((uint8_t *)mac, String(simpleEspNowConnection->macToStr((uint8_t *)buffer)));
				
				char sendMessage[13];
				long id = millis();

				sendMessage[0] = SimpleEspNowMessageType::PAIR;	// Type of message
				sendMessage[1] = 1;	// 1st package
				sendMessage[2] = 1;	// from 1 package. Will be enhanced in one of the next versions
				memcpy(sendMessage+3, &id, 4);	
				
				memcpy(sendMessage+7, simpleEspNowConnection->_myAddress, 6);
				
				esp_now_send(mac, (uint8_t *) sendMessage, strlen(sendMessage));
			}
		}
	}
	else
	{
		if(data[0] == SimpleEspNowMessageType::DATA && simpleEspNowConnection->_MessageFunction)
		{
#ifdef DEBUG
			Serial.printf("Package %d of %d packages\n", data[1], data[2]);
#endif			

			if(data[1] == 1 && data[2] > 1)  // prepare memory for this device
			{
				simpleEspNowConnection->deviceReceiveMessageBuffer.createBuffer(mac, id, data[2]);				
				simpleEspNowConnection->deviceReceiveMessageBuffer.addBuffer(mac, id, (uint8_t *)buffer, len-7, data[1]-1);
			}
			else if(data[2] > 1)
			{
				simpleEspNowConnection->deviceReceiveMessageBuffer.addBuffer(mac, id, (uint8_t *)buffer, len-7, data[1]-1);
			}

			if(data[1] == data[2])
			{
				if(data[2] > 1)
				{
					simpleEspNowConnection->_MessageFunction((uint8_t *)mac, simpleEspNowConnection->deviceReceiveMessageBuffer.getBuffer(mac, data[2]));
					simpleEspNowConnection->deviceReceiveMessageBuffer.deleteBuffer(mac);
				}
				else					
					simpleEspNowConnection->_MessageFunction((uint8_t *)mac, buffer);
			}
		}
		if(simpleEspNowConnection->_PairedFunction)
		{		
			if(data[0] == SimpleEspNowMessageType::PAIR)			
				simpleEspNowConnection->_PairedFunction((uint8_t *)mac, String(simpleEspNowConnection->macToStr((uint8_t *)buffer)));			
		}
		if(simpleEspNowConnection->_ConnectedFunction)
		{
			if(data[0] == SimpleEspNowMessageType::CONNECT)
				simpleEspNowConnection->_ConnectedFunction((uint8_t *)mac, String(simpleEspNowConnection->macToStr((uint8_t *)buffer)));							
		}
		
#ifdef DEBUG
		Serial.println("SimpleEspNowConnection::message arrived from : "+simpleEspNowConnection->macToStr((uint8_t *)mac));
#endif	
	}
}

bool SimpleEspNowConnection::setPairingMac(uint8_t* mac)
{
	memcpy(_pairingMac, mac, 6);
	
	return true;
}

bool SimpleEspNowConnection::setServerMac(String address)
{
	if(address.length() != 12)
		return false;
	
	uint8_t *mac = strToMac(address.c_str());

	if(mac == NULL)
		return false;
	
	setServerMac(mac);
	
	delete mac;
	
	return true;
}

bool SimpleEspNowConnection::setServerMac(uint8_t* mac)
{
	if(this->_role == SimpleEspNowRole::SERVER)
		return false;
	
	memcpy(_serverMac, mac, 6);
	
#ifdef DEBUG
	Serial.println("EspNowConnection::setServerMac to "+simpleEspNowConnection->macToStr(_serverMac));
#endif

	char sendMessage[13];
	long id = millis();
	
	sendMessage[0] = SimpleEspNowMessageType::CONNECT;	// Type of message
	sendMessage[1] = 1;	// 1st package
	sendMessage[2] = 1;	// from 1 package. WIll be enhanced in one of the next versions
	memcpy(sendMessage+3, &id, 4);	

	sendMessage[14] = 0;
	
	memcpy(sendMessage+7, simpleEspNowConnection->_myAddress, 6);

#if defined(ESP32)
	memcpy(&simpleEspNowConnection->_serverMacPeerInfo.peer_addr, _serverMac, 6);
	esp_now_add_peer(&simpleEspNowConnection->_serverMacPeerInfo);
#endif
	
	esp_now_send(mac, (uint8_t *) sendMessage, strlen(sendMessage));
	
	return true;
}

void SimpleEspNowConnection::onPaired(PairedFunction fn)
{
	_PairedFunction = fn;
}

void SimpleEspNowConnection::onMessage(MessageFunction fn)
{
	_MessageFunction = fn;
}

void SimpleEspNowConnection::onNewGatewayAddress(NewGatewayAddressFunction fn)
{
	_NewGatewayAddressFunction = fn;
}

void SimpleEspNowConnection::onConnected(ConnectedFunction fn)
{
	_ConnectedFunction = fn;
}

void SimpleEspNowConnection::onPairingFinished(PairingFinishedFunction fn)
{
	_PairingFinishedFunction = fn;
}

void SimpleEspNowConnection::onSendError(SendErrorFunction fn)
{
	_SendErrorFunction = fn;
}

void SimpleEspNowConnection::onSendDone(SendDoneFunction fn)
{
	_SendDoneFunction = fn;
}

void SimpleEspNowConnection::loop()
{
	SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject *dbo = deviceSendMessageBuffer.getNextBuffer();

	if(dbo == NULL || simpleEspNowConnection->_openTransaction)
		return;
	
	sendPackage(dbo->_id, dbo->_counter, dbo->_packages, dbo->_message, dbo->_len, dbo->_device);
	
	deviceSendMessageBuffer.deleteBuffer(dbo);
}

bool SimpleEspNowConnection::initServer()
{
	
#ifdef DEBUG
        Serial.println("SimpleEspNowConnection: Server initialized");
#endif
		
	return true;
}

bool SimpleEspNowConnection::initClient()
{	

#ifdef DEBUG
        Serial.println("SimpleEspNowConnection: Client initialized");
#endif

	return true;
}
