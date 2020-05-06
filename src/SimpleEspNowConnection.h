/*
  SimpleEspNowConnection.h - A simple EspNow connection and pairing class.
  Erich O. Pintar
  https://pintarweb.net
*/

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
	typedef std::function<void(uint8_t*, String)> NewGatewayAddressFunction;	
	typedef std::function<void(uint8_t*, String)> PairedFunction;	
	typedef std::function<void(uint8_t*, String)> ConnectedFunction;	
  
    SimpleEspNowConnection(SimpleEspNowRole role);

	bool              begin();
	bool              setServerMac(uint8_t* mac);
	bool              setServerMac(String address);	
	bool              setPairingMac(uint8_t* mac);		
	bool 			  sendMessage(char* message, String address = "");
	bool              setPairingBlinkPort(int pairingGPIO, bool invers = true);
	bool 			  startPairing(int timeoutSec = 0);
	bool 			  endPairing();

	void              onMessage(MessageFunction fn);
	void              onNewGatewayAddress(NewGatewayAddressFunction fn);
	void 			  onPaired(PairedFunction fn);
	void 			  onConnected(ConnectedFunction fn);
	
	String 			  macToStr(const uint8_t* mac);

  protected:    
	typedef enum SimpleEspNowMessageType
	{
	  DATA = 1, PAIR = 2, CONNECT = 3
	} SimpleEspNowMessageType_t;
	
  private:    
	SimpleEspNowRole_t    _role;
	  
	uint8_t _pairingMac[6] {0xCE, 0x50, 0xE3, 0x15, 0xB7, 0x34}; // MAC ADDRESS WHEN CLIENT IS LISTENING FOR PAIRING
	uint8_t _myAddress[6];
	uint8_t _serverMac[6];
	
				   
	bool initServer();
	bool initClient();	
	
	uint8_t* strToMac(const char* str);	
	
	static void onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len);
	static void pairingTickerServer();
	static void pairingTickerClient();
	static void pairingTickerLED();
	
	Ticker _pairingTicker, _pairingTickerBlink;	
	volatile bool _pairingOngoing;
	volatile int _pairingCounter;
	volatile int _pairingMaxCount;

	int _pairingGPIO = -1;	
	int _pairingInvers = true;	

	MessageFunction					_MessageFunction = NULL;	
	NewGatewayAddressFunction 		_NewGatewayAddressFunction = NULL;	
	PairedFunction 					_PairedFunction = NULL;	
	ConnectedFunction				_ConnectedFunction = NULL;
};

static SimpleEspNowConnection *simpleEspNowConnection = NULL;
#endif
