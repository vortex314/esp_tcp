 /*
 * Tcp.h
 *
 *  Created on: Oct 24, 2015
 *      Author: lieven
 */

#ifndef TCP_H_
#define TCP_H_
#include "Handler.h"
#include "Stream.h"
#include "CircBuf.h"
#include "Sys.h"
#include "Wifi.h"
extern "C" {
#include "espmissingincludes.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
//#include "mqtt_msg.h"
//#include "user_config.h"

}

typedef enum {

	DNS_RESOLVE,
	TCP_DISCONNECTED,
	TCP_RECONNECT_REQ,
	TCP_RECONNECT,
	TCP_CONNECTING,
	TCP_CONNECTING_ERROR,
	TCP_CONNECTED,

} ConnState;

class Tcp:  public Handler,public Stream {
private:
	Wifi* _wifi;
	uint16_t _srcPort;
	uint16_t _dstPort;
	uint8_t _dstIp[4];
	struct espconn _conn;
	char _host[64];
	ip_addr_t _ip;
//	ConnState _connState;
	CircBuf _rxd;
	CircBuf _txd;
	Bytes _buffer;
	uint32_t _connections;
	uint32_t _bytesRxd;
	uint32_t _bytesTxd;
	uint32_t _overflowTxd;
	bool _connected;
public:
	IROM Tcp(Wifi* wifi);
	IROM ~Tcp();
	void IROM config(const char* host, uint16_t port);
	void IROM connect();
	static void IROM connectCb(void *arg);
	IROM static void reconnectCb(void *arg, int8 err); // mqtt_tcpclient_recon_cb(void *arg, sint8 errType)
	IROM static void dnsFoundCb(const char *name, ip_addr_t *ipaddr, void *arg);
	IROM static void disconnectCb(void *arg);
	IROM static void recvCb(void *arg, char *pdata, unsigned short len);
	IROM static void sentCb(void *arg);
	IROM void send();
	IROM Erc write(Bytes& bytes);
	IROM Erc write(uint8_t b);
	IROM Erc write(uint8_t* pb, uint32_t length);
	IROM bool hasData();
	IROM bool hasSpace();
	IROM uint8_t read();
	IROM bool dispatch(Msg& msg);
	IROM bool isConnected();
	IROM void disconnect();
};

#endif /* TCP_H_ */
