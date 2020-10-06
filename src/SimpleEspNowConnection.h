/*
  SimpleEspNowConnection.h - A simple EspNow connection and pairing class.
  Erich O. Pintar
  https://pintarweb.net
  
  Version : 1.2.0
  
  Created 04 Mai 2020
  By Erich O. Pintar
  Modified 06 Oct 2020
  By Erich O. Pintar
*/

#ifndef SIMPLEESPNOWCONNECTION_H
#define SIMPLEESPNOWCONNECTION_H
#include <Arduino.h>

#define DISABLE_BROWNOUT

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

#if defined(DISABLE_BROWNOUT)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#endif
#endif

#include "Ticker.h"

#define MaxBufferSize 50

typedef enum SimpleEspNowRole 
{
  SERVER = 0, CLIENT = 1
} SimpleEspNowRole_t;

class SimpleEspNowConnection 
{   
  public:
	typedef std::function<void(uint8_t*, const uint8_t*, size_t len)> MessageFunction;	
	typedef std::function<void(uint8_t*, String)> NewGatewayAddressFunction;	
	typedef std::function<void(uint8_t*, String)> PairedFunction;	
	typedef std::function<void(uint8_t*, String)> ConnectedFunction;	
	typedef std::function<void(uint8_t*)> SendErrorFunction;	
	typedef std::function<void(uint8_t*)> SendDoneFunction;	
	typedef std::function<void(void)> PairingFinishedFunction;	
  
    SimpleEspNowConnection(SimpleEspNowRole role);

	bool              begin();
	bool              loop();
	bool              isSendBufferEmpty();
	bool              setServerMac(uint8_t* mac);
	bool              setServerMac(String address);	
	bool              setPairingMac(uint8_t* mac);		
	bool 			  sendMessage(uint8_t* message, size_t len, String address = "");
	bool 			  sendMessage(char* message, String address = "");
	bool 			  sendMessageOld(uint8_t* message, String address = "");
	bool              setPairingBlinkPort(int pairingGPIO, bool invers = true);
	bool 			  startPairing(int timeoutSec = 0);
	bool 			  endPairing();

	void              onMessage(MessageFunction fn);
	void              onNewGatewayAddress(NewGatewayAddressFunction fn);
	void 			  onPaired(PairedFunction fn);
	void 			  onConnected(ConnectedFunction fn);
	void 			  onSendError(SendErrorFunction fn);
	void 			  onSendDone(SendDoneFunction fn);
	void 			  onPairingFinished(PairingFinishedFunction fn);
	
	String 			  macToStr(const uint8_t* mac);
	String 			  myAddress;

  protected:    
	typedef enum SimpleEspNowMessageType
	{
	  DATA = 1, PAIR = 2, CONNECT = 3
	} SimpleEspNowMessageType_t;
	
	class DeviceMessageBuffer
	{
		public:
			class DeviceBufferObject
			{
				public:
					DeviceBufferObject();
					DeviceBufferObject(long id, int counter, int packages, const uint8_t *device);
					DeviceBufferObject(long id, int counter, int packages, const uint8_t *device, const uint8_t* message, size_t len);
					~DeviceBufferObject();

					long _id;
					uint8_t _device[6];
					uint8_t _message[235];
					size_t _len;
					int _counter;
					int _packages;
			};		

			DeviceMessageBuffer();
			~DeviceMessageBuffer();
			
			bool createBuffer(const uint8_t *device, const uint8_t* message, size_t len);
			bool createBuffer(const uint8_t *device, long id, int packages);
			void addBuffer(const uint8_t *device, long id, uint8_t *buffer, size_t len, int package);
			uint8_t* getBuffer(const uint8_t *device, long id, int packages, size_t len);
			size_t getBufferSize(const uint8_t *device, long id, int packages);
			SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject* getNextBuffer();
			bool isSendBufferEmpty();
			bool deleteBuffer(SimpleEspNowConnection::DeviceMessageBuffer::DeviceBufferObject* dbo);
			bool deleteBuffer(const uint8_t *device, long id);

			DeviceBufferObject *_dbo[MaxBufferSize]; // buffer for messages			
	};
	
	DeviceMessageBuffer deviceSendMessageBuffer;
	DeviceMessageBuffer deviceReceiveMessageBuffer;
	
  private:    
	SimpleEspNowRole_t    _role;
	  
	uint8_t _pairingMac[6] {0xCE, 0x50, 0xE3, 0x15, 0xB7, 0x34}; // MAC ADDRESS WHEN CLIENT IS LISTENING FOR PAIRING
	uint8_t _myAddress[6];
	uint8_t _serverMac[6];
	uint8_t	_channel;	
	volatile long	_lastSentTime;
				   
	bool initServer();
	bool initClient();	
	void prepareSendPackages(uint8_t* message, size_t len, String address);
	bool sendPackage(long id, int package, int sum, uint8_t* message, size_t messagelen, uint8_t* address);
	
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
	bool _supportLooping;
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
