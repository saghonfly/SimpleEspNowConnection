/*
  SimpleEspNowConnection.cpp - A simple EspNow connection and pairing class.
  Erich O. Pintar
  https://pintarweb.net
*/

#include "SimpleEspNowConnection.h"

//#define DEBUG

SimpleEspNowConnection::SimpleEspNowConnection(SimpleEspNowRole role) 
{
	this->_role = role;
	simpleEspNowConnection = this;
	this->_pairingOngoing = false;
	
	WiFi.mode(WIFI_STA);
	WiFi.persistent(false);
	WiFi.macAddress(_myAddress);	
}

bool SimpleEspNowConnection::begin()
{	
	if (esp_now_init() != 0) 
	{
		Serial.println("Error initializing ESP-NOW");
		return false;
	}
	
	esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
	esp_now_register_recv_cb(SimpleEspNowConnection::onReceiveData);
	
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

	char sendMessage[9];
	
	sendMessage[0] = SimpleEspNowMessageType::PAIR;	// Type of message
	sendMessage[1] = 1;	// 1st package
	sendMessage[2] = 1;	// from 1 package. WIll be enhanced in one of the next versions
	sendMessage[8] = 0;
	
	memcpy(sendMessage+2, simpleEspNowConnection->_myAddress, 6);

    esp_now_send(simpleEspNowConnection->_pairingMac, 
		(uint8_t *)sendMessage, 
		9);
	
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
	
    wifi_set_macaddr(STATION_IF, &simpleEspNowConnection->_myAddress[0]);
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

		wifi_set_macaddr(STATION_IF, &_pairingMac[0]);

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
	
	return true;
}

bool SimpleEspNowConnection::sendMessage(char* message, String address)
{
	if(strlen(message) > 140)
		return false;

	char sendMessage[strlen(message)+3];
	
	sendMessage[0] = SimpleEspNowMessageType::DATA;	// Type of message
	sendMessage[1] = 1;	// 1st package
	sendMessage[2] = 1;	// from 1 package. WIll be enhanced in one of the next versions
	sendMessage[strlen(message)+2] = 0;
	
	memcpy(sendMessage+2, message, strlen(message));
	
	if(_role == SimpleEspNowRole::SERVER)
	{
		if(address.length() != 12)
			return false;


		uint8_t *mac = strToMac(address.c_str());

		esp_now_send(mac, (uint8_t *) sendMessage, strlen(sendMessage));

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
	char macAddr[12];
	
	sprintf(macAddr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);	
	
	return String(macAddr);
}

void SimpleEspNowConnection::onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len) 
{
	if(len <= 3)
		return;
	
	char buffer[len-3];
	buffer[len-2] = 0;
	
	memcpy(buffer, data+2, len-2);
	
	if(simpleEspNowConnection->_role == SimpleEspNowRole::CLIENT &&
		simpleEspNowConnection->_pairingOngoing)
	{
		if(data[0] == SimpleEspNowMessageType::PAIR)			
		{
			if(simpleEspNowConnection->_NewGatewayAddressFunction)
			{
				wifi_set_macaddr(STATION_IF, &simpleEspNowConnection->_myAddress[0]);
				
				simpleEspNowConnection->endPairing();
				simpleEspNowConnection->_NewGatewayAddressFunction(mac, String(simpleEspNowConnection->macToStr((uint8_t *)buffer)));
				
				char sendMessage[9];
				
				sendMessage[0] = SimpleEspNowMessageType::PAIR;	// Type of message
				sendMessage[1] = 1;	// 1st package
				sendMessage[2] = 1;	// from 1 package. WIll be enhanced in one of the next versions
				sendMessage[8] = 0;
				
				memcpy(sendMessage+2, simpleEspNowConnection->_myAddress, 6);
				
				esp_now_send(mac, (uint8_t *) sendMessage, strlen(sendMessage));
			}
		}
	}
	else
	{
		if(simpleEspNowConnection->_MessageFunction)
		{
			if(data[0] == SimpleEspNowMessageType::DATA)			
				simpleEspNowConnection->_MessageFunction(mac, buffer);
		}
		if(simpleEspNowConnection->_PairedFunction)
		{		
			if(data[0] == SimpleEspNowMessageType::PAIR)			
				simpleEspNowConnection->_PairedFunction(mac, String(simpleEspNowConnection->macToStr((uint8_t *)buffer)));			
		}
		if(simpleEspNowConnection->_ConnectedFunction)
		{
			if(data[0] == SimpleEspNowMessageType::CONNECT)
				simpleEspNowConnection->_ConnectedFunction(mac, String(simpleEspNowConnection->macToStr((uint8_t *)buffer)));							
		}
		
#ifdef DEBUG
		Serial.println("SimpleEspNowConnection::message arrived from : "+simpleEspNowConnection->macToStr(mac));
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

	char sendMessage[9];
	
	sendMessage[0] = SimpleEspNowMessageType::CONNECT;	// Type of message
	sendMessage[1] = 1;	// 1st package
	sendMessage[2] = 1;	// from 1 package. WIll be enhanced in one of the next versions
	sendMessage[8] = 0;
	
	memcpy(sendMessage+2, simpleEspNowConnection->_myAddress, 6);
	
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
