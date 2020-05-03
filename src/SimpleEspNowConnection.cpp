#include "SimpleEspNowConnection.h"

#define DEBUG

extern "C" {
#include <espnow.h>	
#include <user_interface.h>
}

SimpleEspNowConnection::SimpleEspNowConnection(EspNowRole role) 
{
	this->_role = role;
	SimpleEspNowConnection = this;
	
	WiFi.mode(WIFI_STA);

	WiFi.persistent(false);
}

bool SimpleEspNowConnection::begin()
{	
	if(this->_role == EspNowRole::SERVER)
	{
		initServer();
	}
	else
	{
		initClient();
	}
	
	return true;
}

bool SimpleEspNowConnection::sendMessage(char *message, String address)
{
	if(this->_role == EspNowRole::SERVER)
	{
		if(address.length() != 12)
			return false;
		
		uint8_t *mac = strToMac(address.c_str());

		if(mac == NULL)
			return false;
		
		SimpleEspNowConnection_DATA cd;
		
		strncpy(cd.deviceNameFrom, _deviceName, 13);
		macToStr(mac).toCharArray(cd.deviceNameTo, 13);	
		
		memset(cd.payload,0,100);
		memcpy(cd.payload, message, strlen(message));
		
	#ifdef DEBUG
		Serial.println("SimpleEspNowConnection::Will send message '"+String(cd.payload)+"' to '"+macToStr(_gatewayMac)+"'");
	#endif
		
		esp_now_send(mac, (uint8_t *)&cd, sizeof(cd));			
		
		delete mac;		
	}
	else
	{	
		SimpleEspNowConnection_DATA cd;
		
		strncpy(cd.deviceNameFrom, _deviceName, 13);
		macToStr(_gatewayMac).toCharArray(cd.deviceNameTo, 13);	
		
		memset(cd.payload,0,100);
		memcpy(cd.payload, message, strlen(message));
		
	#ifdef DEBUG
		Serial.println("SimpleEspNowConnection::Will send message '"+String(cd.payload)+"' to '"+macToStr(_gatewayMac)+"'");
	#endif
		
		esp_now_send(_gatewayMac, (uint8_t *)&cd, sizeof(cd));	
	}
	
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

bool SimpleEspNowConnection::setServerMac(uint8_t *mac)
{
	if(this->_role == EspNowRole::SERVER)
		return false;
		
	_gatewayMac[0] = mac[0];
	_gatewayMac[1] = mac[1];
	_gatewayMac[2] = mac[2];
	_gatewayMac[3] = mac[3];
	_gatewayMac[4] = mac[4];
	_gatewayMac[5] = mac[5];
	
    esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, _cryptKey, 16);
    esp_now_set_peer_key(mac, _cryptKey, 16);

	esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) 
		{
#ifdef DEBUG
			Serial.println("SimpleEspNowConnection::Sent message to "+SimpleEspNowConnection->macToStr(mac));
#endif
		});
	
#ifdef DEBUG
	Serial.println("SimpleEspNowConnection::setServerMac to "+SimpleEspNowConnection->macToStr(mac));
#endif

	return true;
}

void SimpleEspNowConnection::onNewGatewayAddress(NewGatewayAddressFunction fn)
{
	_NewGatewayAddressFunction = fn;
}

void SimpleEspNowConnection::onMessage(MessageFunction fn)
{
	_MessageFunction = fn;
}

void pairingTicker()
{
#ifdef DEBUG
    Serial.println("SimpleEspNowConnection::Pairing request sent..."+String(_pairingCounter+1)+"/"+String(_pairingMaxCount));
#endif

    esp_now_send(_broadcastMac, (uint8_t *)&_pairingData, sizeof(_pairingData));
	
	if(_pairingMaxCount > 0)
	{
		_pairingCounter++;
		if(_pairingCounter >= _pairingMaxCount)
		{
			SimpleEspNowConnection->endPairing();
		}
	}
}

void pairingTickerLED()
{	
  if(_pairingOngoing)
  {
    int state = digitalRead(_pairingGPIO);
    digitalWrite(_pairingGPIO, !state); 
  }
  else
  {
    digitalWrite(_pairingGPIO, HIGH);
    _pairingTickerBlink.detach();
  }
}

bool SimpleEspNowConnection::setPairingBlinkPort(int pairingGPIO)
{
	_pairingGPIO = pairingGPIO;
	
    pinMode(_pairingGPIO, OUTPUT);
    digitalWrite(_pairingGPIO, 1); 
	
	return true;
}

void SimpleEspNowConnection::onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len) 
{
	SimpleEspNowConnection_DATA clientData;
	memcpy(&clientData, data, len);
	#ifdef DEBUG
	Serial.println("SimpleEspNowConnection::Server message arrived from : "+String(clientData.deviceNameFrom));
	#endif	

	if(SimpleEspNowConnection->_MessageFunction)
	{
		SimpleEspNowConnection->_MessageFunction(mac, clientData.payload);
	}
}

bool SimpleEspNowConnection::startPairing(int timeoutSec)
{	
	if(_pairingOngoing) return false;
	
	if(timeoutSec > 0 && timeoutSec < 5)
		timeoutSec = 5;
	
	if(this->_role == EspNowRole::SERVER)
	{
#ifdef DEBUG
		Serial.println("SimpleEspNowConnection::Server Pairing started");
#endif	
		strcpy(_pairingData.deviceNameFrom, _deviceName);	
		strcpy(_pairingData.deviceNameTo, _deviceName);	
		
		memset(_pairingData.payload,0,100);
		memcpy(_pairingData.payload, "PAIRWITHME", 10);
		
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
		
		_pairingTicker.attach(5.0, pairingTicker);   

		if(_pairingGPIO != -1)
		{
			_pairingTickerBlink.attach(0.5, pairingTickerLED);            
		}
	}
	else
	{
		wifi_set_macaddr(STATION_IF, &_broadcastMac[0]);
		
#ifdef DEBUG
		Serial.println("SimpleEspNowConnection::Client Pairing started");
#endif	
	    esp_now_add_peer(&_broadcastMac[0], ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

		esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) 
			{
				SimpleEspNowConnection_DATA pairingData;
				memcpy(&pairingData, data, len);

				
				if((strcmp(pairingData.deviceNameFrom, pairingData.deviceNameTo) == 0) &&
				String(pairingData.payload) == "PAIRWITHME")
				{
#ifdef DEBUG
					Serial.print("SimpleEspNowConnection::Pairing arrived from ");
					Serial.print(SimpleEspNowConnection->macToStr(mac)+":'");				
					Serial.print(pairingData.payload);
					Serial.println("'");
#endif	
					if(SimpleEspNowConnection->_NewGatewayAddressFunction)
					{
						SimpleEspNowConnection->_NewGatewayAddressFunction(mac, SimpleEspNowConnection->macToStr(mac));
					}

			//		wifi_set_macaddr(SOFTAP_IF, SimpleEspNowConnection->_trigMac);
					wifi_set_macaddr(STATION_IF, SimpleEspNowConnection->_deviceMac);
					esp_now_unregister_recv_cb();
			
					esp_now_deinit();	
					SimpleEspNowConnection->begin();
				}
				else
				{
#ifdef DEBUG
					Serial.print("SimpleEspNowConnection::Something arrived from ");
					Serial.print(String(pairingData.deviceNameFrom)+":'");				
					Serial.print(pairingData.payload);
					Serial.println("'");
#endif	
				}
			}
		);
	}
	
	return true;
}

bool SimpleEspNowConnection::endPairing()
{
	_pairingOngoing = false;
    _pairingTicker.detach();

#ifdef DEBUG
	Serial.println("SimpleEspNowConnection::Server Pairing endet");
#endif	
	
	return true;
}

void SimpleEspNowConnection::end()
{
}

bool SimpleEspNowConnection::setBroadcastMac(uint8_t* broadcastMac)
{
	if( broadcastMac == NULL ) { return false; }

	memset(_broadcastMac,0,6);
	memcpy(_broadcastMac,broadcastMac, 6);
  
	return true;
}

bool SimpleEspNowConnection::setCryptKey(uint8_t* cryptKey)
{
	if( cryptKey == NULL ) { return false; }

	memset(this->_cryptKey,0,16);
	memcpy(this->_cryptKey,cryptKey, 16);
  
	return true;
}

String SimpleEspNowConnection::macToStr(const uint8_t* mac)
{
	char macAddr[12];
	
	sprintf(macAddr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);	
	
	return String(macAddr);
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
	
	for(int i=0;i<6;i++)
	{
		Serial.print(mac[i]);
		Serial.print(",");
	}

	Serial.println();

	return mac;
}

void SimpleEspNowConnection::initVariant() 
{
//  WiFi.mode(WIFI_AP);

  WiFi.macAddress(_gatewayMac);

  _gatewayMac[0] = 0xCE;
  _gatewayMac[1] = 0x50;
  _gatewayMac[2] = 0xE3;
  
  wifi_set_macaddr(STATION_IF, &_gatewayMac[0]);//new mac is set
  
  macToStr(_gatewayMac).toCharArray(_deviceName, 12);  

  memcpy(_deviceMac, _gatewayMac, 6);

#ifdef DEBUG
	Serial.println("SimpleEspNowConnection::_deviceName is "+String(_deviceName));
#endif	  
}


bool SimpleEspNowConnection::initServer()
{
	initVariant();
	
	esp_now_init();
	esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
	esp_now_add_peer(&_broadcastMac[0], ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
	esp_now_add_peer(&_trigMac[0], ESP_NOW_ROLE_CONTROLLER, 1, &_cryptKey[0], 16);
	esp_now_set_peer_key(&_trigMac[0], &_cryptKey[0], 16);

	esp_now_register_recv_cb(SimpleEspNowConnection::onReceiveData);

#ifdef DEBUG
	Serial.println("SimpleEspNowConnection::Server initialized");
#endif	
		
	return true;
}

bool SimpleEspNowConnection::initClient()
{	
	WiFi.disconnect();
	
	WiFi.macAddress(_deviceMac);
	macToStr(_deviceMac).toCharArray(_deviceName, 13);  
	wifi_set_macaddr(STATION_IF, &SimpleEspNowConnection->_deviceMac[0]);

	WiFi.begin();
	
	esp_now_init();	
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);	

	esp_now_register_recv_cb(SimpleEspNowConnection::onReceiveData);

#ifdef DEBUG
	Serial.println("SimpleEspNowConnection::Client initialized ("+String(_deviceName)+")");
#endif	

	return true;
}