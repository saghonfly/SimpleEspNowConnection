#ifndef SIMPLEESPNOWCONNECTION_H
#define SIMPLEESPNOWCONNECTION_H
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Ticker.h"

extern "C" {
#include <espnow.h>
#include <user_interface.h>
}

static bool _pairingOngoing = false;
static int _pairingGPIO = -1;
static Ticker _pairingTicker, _pairingTickerBlink;	
static uint8_t _broadcastMac[6] = {0x46, 0x33, 0x33, 0x33, 0x33, 0x33};  
static int _pairingCounter = 0;
static int _pairingMaxCount = 0;

typedef struct __attribute__((packed)) ESPNOWCONNECTION_DATA 
{
	char deviceNameFrom[13];
	char deviceNameTo[13];
//	String payload;
	char payload[100];
} ESPNOWCONNECTION_DATA;

typedef enum EspNowRole 
{
  SERVER = 0, CLIENT = 1
} EspNowRole_t;

static ESPNOWCONNECTION_DATA _pairingData;

class SimpleEspNowConnection 
{   
  public:
    // Initialize EspNowConnection
	typedef std::function<void(uint8_t*, String)> NewGatewayAddressFunction;	
	typedef std::function<void(uint8_t*, String)> MessageFunction;	

    SimpleEspNowConnection(EspNowRole role);
	bool setBroadcastMac(uint8_t* broadcastMac);
	bool setCryptKey(uint8_t* cryptKey);

	bool              begin();
	bool              setServerMac(uint8_t *mac);
	bool              setServerMac(String address);
	bool              setPairingBlinkPort(int pairingGPIO);
	bool 			  startPairing(int timeoutSec = 0);
	bool 			  sendMessage(char *message, String address = "");
	bool 			  endPairing();
	void              end();

	void              onNewGatewayAddress(NewGatewayAddressFunction fn);
	void              onMessage(MessageFunction fn);
	String 			  macToStr(const uint8_t* mac);

  protected:    
	NewGatewayAddressFunction 		_NewGatewayAddressFunction = NULL;
	MessageFunction					_MessageFunction = NULL;
	
  private:    
	EspNowRole_t    _role;
	
  
	char _deviceName[13];	
	uint8_t _deviceMac[6];
	uint8_t _cryptKey[16] = {0x51, 0xA0, 0xDE, 0xC5, 0x46, 0xC6, 0x77, 0xCD,
                   0x99, 0xE8, 0x61, 0xF9, 0x08, 0x77, 0x7D, 0x00};  
	uint8_t _trigMac[6] {0xCE, 0x50, 0xE3, 0x15, 0xB7, 0x33};
	uint8_t _gatewayMac[6];
	uint8_t* strToMac(const char* str);

				   
	void initVariant();
	bool initServer();
	bool initClient();	
	
	static void onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len);
};

static SimpleEspNowConnection *espNowConnection = NULL;
#endif