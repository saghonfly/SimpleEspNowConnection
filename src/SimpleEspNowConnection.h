/*
  SimpleEspNowConnection.h - A simple EspNow connection and pairing class.
  Erich O. Pintar
  https://pintarweb.net
  
  Version : 1.0.4
  
  Created 04 Mai 2020
  By Erich O. Pintar
  Modified 17 Mai 2020
  By Erich O. Pintar
*/

#ifndef SIMPLEESPNOWCONNECTION_H
#define SIMPLEESPNOWCONNECTION_H
#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
extern "C" {
#include <espnow.h>
#include <user_interface.h>
}
#elif defined(ESP32)
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#endif

#include "Ticker.h"

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
	typedef std::function<void(uint8_t*)> SendErrorFunction;	
	typedef std::function<void(uint8_t*)> SendDoneFunction;	
	typedef std::function<void(void)> PairingFinishedFunction;	
  
    SimpleEspNowConnection(SimpleEspNowRole role);

	bool              begin();
	bool 			  loop();
	bool              setServerMac(uint8_t* mac);
	bool              setServerMac(String address);	
	bool              setPairingMac(uint8_t* mac);		
	bool 			  sendMessage(char* message, String address = "");
	bool              setPairingBlinkPort(int pairingGPIO, bool invers = true);
	bool 			  startPairing(int timeoutSec = 0);
	bool 			  endPairing();
	bool			  canSend();

	void              onMessage(MessageFunction fn);
	void              onNewGatewayAddress(NewGatewayAddressFunction fn);
	void 			  onPaired(PairedFunction fn);
	void 			  onConnected(ConnectedFunction fn);
	void 			  onSendError(SendErrorFunction fn);
	void 			  onSendDone(SendDoneFunction fn);
	void 			  onPairingFinished(PairingFinishedFunction fn);
	
	String 			  macToStr(const uint8_t* mac);
	uint8_t*		  getMyAddress();

  protected:    
	typedef enum SimpleEspNowMessageType
	{
	  DATA = 1, PAIR = 2, CONNECT = 3
	} SimpleEspNowMessageType_t;
		
	class DeviceMessageBuffer
	{
		protected:
			class DeviceBufferObject
			{
				public:
					DeviceBufferObject(){};
					DeviceBufferObject(uint8_t *device, int packages);
					~DeviceBufferObject();

					uint8_t devicename[6];
					uint8_t* buffer;
			};		

		public:
			DeviceMessageBuffer();
			~DeviceMessageBuffer();
			
			void createBuffer(uint8_t *device, int packages);
			void addBuffer(uint8_t *device, uint8_t *buffer, int len, int package);
			uint8_t* getBuffer(uint8_t *device);
			void deleteBuffer(uint8_t *device);


			DeviceBufferObject *dbo[20]; // buffer for 20 at the same time sending devices should be sufficent
			
	};
	
	DeviceMessageBuffer deviceMessageBuffer;

  private:    
	SimpleEspNowRole_t    _role;
	  
	uint8_t _pairingMac[6] {0xCE, 0x50, 0xE3, 0x15, 0xB7, 0x34}; // MAC ADDRESS WHEN CLIENT IS LISTENING FOR PAIRING
	uint8_t _myAddress[6];
	uint8_t _serverMac[6];
	uint8_t	_channel;
	volatile long	_lastSentTime;
				   
	bool initServer();
	bool initClient();	
	bool sendPackage(int package, int sum, char* message, String address);
	uint8_t* strToMac(const char* str);	
	
#if defined(ESP8266)
	static void onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len);
#elif defined(ESP32)
	static void onReceiveData(const uint8_t *mac, const uint8_t *data, int len);
#endif
	static void pairingTickerServer();
	static void pairingTickerClient();
	static void pairingTickerLED();
	
	
	Ticker _pairingTicker, _pairingTickerBlink;	
	volatile bool _pairingOngoing;
	volatile int _pairingCounter;
	volatile int _pairingMaxCount;
	volatile bool _openTransaction;

	int _pairingGPIO = -1;	
	int _pairingInvers = true;	

	MessageFunction					_MessageFunction = NULL;	
	NewGatewayAddressFunction 		_NewGatewayAddressFunction = NULL;	
	PairedFunction 					_PairedFunction = NULL;	
	ConnectedFunction				_ConnectedFunction = NULL;
	SendErrorFunction				_SendErrorFunction = NULL;
	SendDoneFunction				_SendDoneFunction = NULL;
	PairingFinishedFunction			_PairingFinishedFunction = NULL;	
	
#if defined(ESP32)
	esp_now_peer_info_t _serverMacPeerInfo;
	esp_now_peer_info_t _clientMacPeerInfo;
#endif	
};

static SimpleEspNowConnection *simpleEspNowConnection = NULL;
#endif
