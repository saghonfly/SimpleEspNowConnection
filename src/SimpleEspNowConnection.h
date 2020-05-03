#ifndef SIMPLEESPNOWCONNECTION_H
#define SIMPLEESPNOWCONNECTION_H
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Ticker.h"

extern "C" {
#include <espnow.h>
#include <user_interface.h>
}


typedef enum SimpleEspNowRole 
{
  SERVER = 0, CLIENT = 1
} SimpleEspNowRole_t;


class SimpleEspNowConnection 
{   
  public:
	typedef std::function<void(uint8_t*, const char*)> MessageFunction;	
  
    SimpleEspNowConnection(SimpleEspNowRole role);

	bool              begin();
	void              end();
	bool              setServerMac(uint8_t *mac);
	bool              setServerMac(String address);	
	bool 			  sendMessage(char *message, String address = "");

	void              onMessage(MessageFunction fn);
	
	String 			  macToStr(const uint8_t* mac);

  protected:    
	
  private:    
	SimpleEspNowRole_t    _role;
	  
	//uint8_t _broadcastMac[6] = {0x46, 0x33, 0x33, 0x33, 0x33, 0x33};  
	uint8_t _pairingMac[6] {0xCE, 0x50, 0xE3, 0x15, 0xB7, 0x34}; // MAC ADDRESS WHEN CLIENT IS LISTENING FOR PAIRING
	uint8_t _myAddress[6];
	uint8_t _serverMac[6];
	
				   
	bool initServer();
	bool initClient();	
	
	uint8_t* strToMac(const char* str);	
	
	static void onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len);

	MessageFunction					_MessageFunction = NULL;	
};

static SimpleEspNowConnection *simpleEspNowConnection = NULL;
#endif