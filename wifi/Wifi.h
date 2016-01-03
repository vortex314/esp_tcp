/*
 * Wifi.h
 *
 *  Created on: Oct 24, 2015
 *      Author: lieven
 */

#ifndef WIFI_H_
#define WIFI_H_
#include "Handler.h"

 class Wifi : public Handler{
	 char _ssid[32];
	 char _pswd[64];
	 uint32_t _connections;
	 bool _connected;
	static void callback();
	static uint8_t _wifiStatus ;
public:
	IROM Wifi();
	IROM virtual ~Wifi();
	IROM void config(const char* ssid,const char* pswd);
	IROM bool dispatch(Msg& msg);
	IROM void init();
	IROM bool isConnected() const;
}
;

#endif /* WIFI_H_ */
