#include "SimpleEspNowConnection.h"

#define DEBUG


SimpleEspNowConnection::SimpleEspNowConnection(SimpleEspNowRole role) 
{
	this->_role = role;
	simpleEspNowConnection = this;
	WiFi.persistent(false);
	
	WiFi.mode(WIFI_STA);
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

bool SimpleEspNowConnection::sendMessage(char *message, String address)
{
	if(_role == SimpleEspNowRole::SERVER)
	{
		if(address.length() != 12)
			return false;

		uint8_t *mac = strToMac(address.c_str());

		esp_now_send(mac, (uint8_t *) message, strlen(message));

		delete mac;
	}
	else
	{
		esp_now_send(_serverMac, (uint8_t *) message, strlen(message));
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
	if(simpleEspNowConnection->_MessageFunction)
	{
		char buffer[len+1];
		buffer[len] = 0;
		
		memcpy(buffer, data, len);
		
		simpleEspNowConnection->_MessageFunction(mac, buffer);
	}
	
#ifdef DEBUG
	Serial.println("SimpleEspNowConnection::message arrived from : "+simpleEspNowConnection->macToStr(mac));
#endif	

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
	if(this->_role == SimpleEspNowRole::SERVER)
		return false;
	
	memcpy(_serverMac, mac, 6);
	
#ifdef DEBUG
	Serial.println("EspNowConnection::setServerMac to "+simpleEspNowConnection->macToStr(_serverMac));
#endif

	return true;
}

void SimpleEspNowConnection::onMessage(MessageFunction fn)
{
	_MessageFunction = fn;
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