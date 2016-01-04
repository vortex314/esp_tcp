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

typedef union {
	uint8_t b[4];
	uint32_t addr;
	ip_addr ipAddr;
} IpAddress;

class Tcp: public Handler, public Stream {
protected:
	Wifi* _wifi;
	IpAddress _remote_ip;
	uint16_t _remote_port;
	uint16_t _local_port;
	struct espconn* _conn;
	char _host[64];
	bool _connected;
private:
	static Tcp* _first;
	Tcp* _next;
	static uint32_t _maxConnections;
//	ConnState _connState;
	CircBuf _rxd;
	CircBuf _txd;
	Bytes _buffer;
	uint32_t _connections;
	uint32_t _bytesRxd;
	uint32_t _bytesTxd;
	uint32_t _overflowTxd;

public:
	IROM Tcp(Wifi* wifi); //
	IROM Tcp(Wifi* wifi, struct espconn* conn); //
	IROM ~Tcp(); //
	IROM void logConn(const char* s, void *arg);
	void IROM loadEspconn(struct espconn* conn);

	void IROM connect();
	void IROM connect(const char* host, uint16_t port);

	void IROM disconnect();

	IROM void registerCb(struct espconn* pconn);	//
	static IROM void globalInit(Wifi* wifi, uint32_t maxConnections);
	static IROM Tcp* findTcp(struct espconn* pconn);
	static IROM Tcp* findFreeTcp(struct espconn* pconn);
	static IROM bool match(struct espconn* pconn, Tcp* pTcp); //
	void IROM reg();
	uint32_t IROM count(); //
	uint32_t IROM used(); //
	static IROM void connectCb(void* arg);	//
	IROM static void reconnectCb(void* arg, int8 err); // mqtt_tcpclient_recon_cb(void *arg, sint8 errType)
	IROM static void disconnectCb(void* arg); //
	IROM static void dnsFoundCb(const char *name, ip_addr_t *ipaddr, void *arg); //
	IROM static void recvCb(void* arg, char *pdata, unsigned short len); //
	IROM static void sendCb(void* arg); //
	IROM static void writeFinishCb(void* arg); //

	IROM void send();	//
	IROM Erc write(Bytes& bytes); //
	IROM Erc write(uint8_t b); //
	IROM Erc write(uint8_t* pb, uint32_t length); //
	IROM bool hasData(); //
	IROM bool hasSpace(); //
	IROM uint8_t read(); //
	virtual IROM bool dispatch(Msg& msg); //
	IROM bool isConnected(); //

};

class TcpServer: public Tcp {
public:
	IROM TcpServer(Wifi* wifi);
	bool IROM dispatch(Msg& msg); //
	Erc IROM config(uint32_t maxConnections, uint16_t port);
	void listen();
};

class TcpClient: public Tcp {
public:
	IROM TcpClient(Wifi* wifi);IROM bool dispatch(Msg& msg); //
	void IROM config(const char* host, uint16_t port);

};

#endif /* TCP_H_ */
