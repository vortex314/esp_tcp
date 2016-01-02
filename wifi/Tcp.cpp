/*
 * Tcp.cpp
 *
 *  Created on: Oct 24, 2015
 *      Author: lieven
 */

#include "Tcp.h"
#include <string.h>

//#include <LinkedList.h>
//#include <QueueTemplate.h>

typedef struct {
	IpAddress _host;
	uint16_t _port;
	Tcp* _instance;
} Callback;

// LinkedList<Tcp*> ll();
/*
 static Callback* findInstance(struct espconn* pconn) {
 Callback* cursor=callbackList.first()->_content;
 for (cursor = callback.first()->_content; cursor != 0; cursor=cursor->next()) {
 if (cursor->_port == pconn->proto.tcp->remote_port)
 if ( memcmp(cursor->_host.b, pconn->proto.tcp->remote_ip,4) == 0)
 return cursor;
 }
 return 0;
 }*/

bool IROM Tcp::match(struct espconn* pconn, Tcp* pTcp) {
	if (pTcp->_remote_port == pconn->proto.tcp->remote_port)
		if (memcmp(pTcp->_remote_ip.b, pconn->proto.tcp->remote_ip, 4) == 0)
			return true;
	return false;
}

Tcp* getInstance(void *arg) {
	struct espconn *pCon = (struct espconn *) arg;
	Tcp* pTcp;
//	ll.first();
	return (Tcp*) pCon->reverse;
}

//const char* TCP_OS = "TCP_OS";

uint8_t StrToIP(const char* str, void *ip);

extern Wifi* wifi;

void logCb(const char* s, void* arg) {
	struct espconn* pconn = (struct espconn*) arg;
	INFO(" %s --  esp : %x , tcp :  %x  , ip : %d.%d.%d.%d:%d ", s, pconn,
			pconn->reverse, pconn->proto.tcp->remote_ip[0],
			pconn->proto.tcp->remote_ip[1], pconn->proto.tcp->remote_ip[2],
			pconn->proto.tcp->remote_ip[3], pconn->proto.tcp->remote_port);
}

Tcp::Tcp(Wifi* wifi) :
		Handler("Tcp"), _rxd(512), _txd(256), _buffer(100) {
	_wifi = wifi;
//	INFO("TCP : ctor");
	_conn = new (struct espconn);
	strcpy(_host, "");
	_remote_port = 80;
	_remote_ip.addr = 0;
	_connected = false;
	ets_memset(_conn, 0, sizeof(_conn));
	_conn->type = ESPCONN_TCP;
	_conn->state = ESPCONN_NONE;
	_conn->proto.tcp = (esp_tcp *) malloc(sizeof(esp_tcp));
	ets_memset(_conn->proto.tcp, 0, sizeof(esp_tcp));
//	registerCb(_conn);
	_bytesTxd = 0;
	_bytesRxd = 0;
	_overflowTxd = 0;
	_connections = 0;
	_local_port = 0;
}

Tcp::Tcp(Wifi* wifi, struct espconn* pconn) :
		Handler("Tcp"), _rxd(512), _txd(256), _buffer(100) {
	_wifi = wifi;
	_conn = pconn;

	ets_sprintf(_host, "%d.%d.%d.%d", pconn->proto.tcp->remote_ip[0],
			pconn->proto.tcp->remote_ip[1], pconn->proto.tcp->remote_ip[2],
			pconn->proto.tcp->remote_ip[3]);
	_remote_port = pconn->proto.tcp->remote_port;
	os_memcpy(&_remote_ip, pconn->proto.tcp->remote_ip, 4);
	_connected = true;
	_bytesTxd = 0;
	_bytesRxd = 0;
	_overflowTxd = 0;
	_connections = 0;
	_local_port = 0;

}

Tcp::~Tcp() {
	if (_conn->proto.tcp)
		free(_conn->proto.tcp);
	INFO("deleted");
}

Erc Tcp::write(Bytes& bytes) {
	return write(bytes.data(), bytes.length());
}

Erc Tcp::write(uint8_t* pb, uint32_t length) {
	uint32_t i = 0;
	while (_txd.hasSpace() && (i < length)) {
		_txd.write(pb[i++]);
	};
	if (i < length) {
		_overflowTxd++;
		ERROR("TCP BUFFER OVERFLOW");
	}
	send();
	return E_OK;
}

Erc Tcp::write(uint8_t data) {
	if (_txd.hasSpace())
		_txd.write(data);
	else {
		_overflowTxd++;
	}
	send();
	return E_OK;
}

uint8_t Tcp::read() {
	return _rxd.read();
}

bool Tcp::hasData() { // not in  as it will be called in interrupt
	return _rxd.hasData();
}

bool Tcp::hasSpace() {
	return _txd.hasSpace();
}

//	callkback when tcp connection is established
//_________________________________________________________
//
void Tcp::connectCb(void* arg) {
	logCb(__FUNCTION__, arg);
	struct espconn* pconn = (struct espconn*) arg;
	Tcp *pTcp = new Tcp(wifi, pconn);
	pTcp->registerCb(pconn);
	return;

	espconn_set_opt(pconn, ESPCONN_NODELAY);
	espconn_set_opt(pconn, ESPCONN_COPY);

	Msg::publish(pTcp, SIG_CONNECTED);
	pTcp->_connected = true;
}

void Tcp::disconnectCb(void* arg) {
	logCb(__FUNCTION__, arg);
	struct espconn* pconn = (struct espconn*) arg;
	Tcp *pTcp = getInstance(pconn);
//	delete pTcp;
	return;

	/*	INFO(" connected to  %d.%d.%d.%d:%d ", conn->proto.tcp->remote_remote_ip[0],
	 conn->proto.tcp->remote_remote_ip[1], conn->proto.tcp->remote_remote_ip[2],
	 conn->proto.tcp->remote_remote_ip[3], conn->proto.tcp->remote_port); */

//	INFO("TCP: Disconnected %s:%d ", pTcp->_host, pTcp->_remote_port);
//	pTcp->_remote_ip.addr = 0;
	Msg::publish(pTcp, SIG_DISCONNECTED);
	pTcp->_connected = false;
}

void Tcp::send() { // send buffered data, max 100 bytes
	while (true) {
		if (_buffer.length()) {
			// retry send same data
		} else {
			while (_txd.hasData() && _buffer.hasSpace(1)) {
				_buffer.write(_txd.read());
			}
		}
		if (_buffer.length() == 0)
			return;
		int8_t erc = espconn_sent(_conn, _buffer.data(), _buffer.length());
		if (erc == 0) {
			_bytesTxd += _buffer.length();
			_buffer.clear();
//		INFO(" TCP:send %d bytes", length);
		} else {
			return;
		}
	}
}

void Tcp::writeFinishCb(void* arg) {
	logCb(__FUNCTION__, arg);
	struct espconn* pconn = (struct espconn*) arg;
	Tcp *pTcp = getInstance(pconn);
	return;

	pTcp->send();
	Msg::publish(pTcp, SIG_TXD);
}

void Tcp::sendCb(void *arg) {
	logCb(__FUNCTION__, arg);
	struct espconn* pconn = (struct espconn*) arg;
	Tcp *pTcp = getInstance(pconn);
	return;

//	INFO("TCP: Txd %s:%d ", pTcp->_host, pTcp->_remote_port);
	pTcp->send();
	Msg::publish(pTcp, SIG_TXD);
}

#include <Message.h>

Message message(100);


void Tcp::recvCb(void* arg, char *pdata, unsigned short len) {
	logCb(__FUNCTION__, arg);
	if ( message.length()==0) {
		message.addField(Message::PROP_CMD,1);
		message.addField(Message::TEXT,"Hello world\n");
	}
	INFO(" bytes : %d ",len);
	struct espconn* pconn = (struct espconn*) arg;
	Tcp *pTcp = getInstance(pconn);
	espconn_sent(pconn, message.data(), message.length());
	return;

//	INFO("TCP: Rxd %s:%d length : %d", pTcp->_host, pTcp->_remote_port, len);
	pTcp->_bytesRxd += len;
	int i;
	for (i = 0; i < len; i++) {
//		INFO("0x%X",pdata[i]);
		if (pTcp->_rxd.hasSpace())
			pTcp->_rxd.write(pdata[i]);
		else {
			WARN("TCP overflow");
			break;
		}
	}
	Msg::publish(pTcp, SIG_RXD);
}

void Tcp::reconnectCb(void* arg, int8_t err) {
	logCb(__FUNCTION__, arg);
	struct espconn* pconn = (struct espconn*) arg;
	Tcp *pTcp = getInstance(pconn);
	return;

	INFO("TCP: Reconnect %s:%d err : %d", pTcp->_host, pTcp->_remote_port, err);
	Msg::publish(pTcp, SIG_DISCONNECTED);
	pTcp->_connected = false;
}

void Tcp::dnsFoundCb(const char *name, ip_addr_t *ipaddr, void *arg) {
	logCb(__FUNCTION__, arg);
	Tcp *pTcp = getInstance(arg);

	if (ipaddr == NULL) {
		INFO("DNS: Found, but got no ip, try to reconnect");
		return;
	};
	INFO("DNS: found ip %d.%d.%d.%d", *((uint8 * ) &ipaddr->addr),
			*((uint8 * ) &ipaddr->addr + 1), *((uint8 * ) &ipaddr->addr + 2),
			*((uint8 * ) &ipaddr->addr + 3));

	if (pTcp->_remote_ip.addr == 0 && ipaddr->addr != 0) {
		memcpy(pTcp->_conn->proto.tcp->remote_ip, &ipaddr->addr, 4);
		espconn_connect((pTcp->_conn));
		INFO("TCP: connecting...");
	}
}

void Tcp::config(const char* host, uint16_t port) {
	strncpy(_host, host, sizeof(_host));
	_remote_port = port;
}
//____________________________________________________________
//			Tcp::connect
//____________________________________________________________
void Tcp::connect() {
	connect(_host, _remote_port);
}

void Tcp::connect(const char* host, uint16_t dstPort) {
	INFO(" Connecting ");
//	ets_memset(_conn, 0, sizeof(_conn));
	_conn->type = ESPCONN_TCP;
	_conn->state = ESPCONN_NONE;
//	_conn->proto.tcp = (esp_tcp *) malloc(sizeof(esp_tcp));
	ets_memset(_conn->proto.tcp, 0, sizeof(esp_tcp));
	_conn->proto.tcp->local_port = espconn_port();
	_conn->proto.tcp->remote_port = dstPort;
	_conn->reverse = this;
	registerCb(_conn);
//	TcpCallback::mainRegisterConnection(_conn, this);
//	espconn_regist_connectcb(_conn, TcpCallback::mainConnectCb);
//	espconn_regist_reconcb(_conn, TcpCallback::mainReconnectCb);
	if (StrToIP(_host, _conn->proto.tcp->remote_ip)) {
		INFO("TCP: Connect to ip  %s:%d", _host, dstPort);
		espconn_connect(_conn);
	} else {
		INFO("TCP: Connect to domain %s:%d", host, dstPort);
		espconn_gethostbyname(_conn, host, &_remote_ip.ipAddr, Tcp::dnsFoundCb);
	}
	INFO(" Heap size : %d", system_get_free_heap_size());
}

void Tcp::disconnect() {
	espconn_disconnect(_conn);
}

bool Tcp::isConnected() {
	return _connected;
}
//____________________________________________________________
//
//____________________________________________________________
bool Tcp::dispatch(Msg& msg) {
	PT_BEGIN()
	;
	INIT: {
		INFO(" TCP started : %x ", this);
		PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
		INFO(" SIG_INIT");
		goto WIFI_DISCONNECTED;
	}
	WIFI_DISCONNECTED: {
		while (true) {
			PT_YIELD_UNTIL(_wifi->isConnected());
			listen(2323);
			goto CONNECTING;
		}
	}
	CONNECTING: {
		while (true) {
//			listen(2323);
			timeout(3000);
			PT_YIELD_UNTIL(isConnected() || timeout());
			if (isConnected())
				goto CONNECTED;
			if (msg.is(this, SIG_DISCONNECTED)) {
				timeout(1000);
				PT_YIELD_UNTIL(timeout());
				goto CONNECTING;
			}
		}
	}
	CONNECTED: {
		while (true) {
			timeout(2000);
			PT_YIELD_UNTIL(
					msg.is(this, SIG_DISCONNECTED)
							|| msg.is(_wifi, SIG_DISCONNECTED));
			if (msg.is(this, SIG_DISCONNECTED)) {
				timeout(1000);
				PT_YIELD_UNTIL(timeout());
				goto CONNECTING;
			}
			if (msg.is(_wifi, SIG_DISCONNECTED))
				goto WIFI_DISCONNECTED;
		}
	}
	;
PT_END()
;
return true;
}

uint8_t StrToIP(const char* str, void *ip) {
INFO(" convert  Ip address : %s", str);
int i; // The count of the number of bytes processed.
const char * start; // A pointer to the next digit to process.
start = str;

for (i = 0; i < 4; i++) {
	char c; /* The digit being processed. */
	int n = 0; /* The value of this byte. */
	while (1) {
		c = *start;
		start++;
		if (c >= '0' && c <= '9') {
			n *= 10;
			n += c - '0';
		}
		/* We insist on stopping at "." if we are still parsing
		 the first, second, or third numbers. If we have reached
		 the end of the numbers, we will allow any character. */
		else if ((i < 3 && c == '.') || i == 3) {
			break;
		} else {
			return 0;
		}
	}
	if (n >= 256) {
		return 0;
	}
	((uint8_t*) ip)[i] = n;
}
return 1;
}

void Tcp::listen(uint16_t port) {
INFO(" listening. ");
_conn->type = ESPCONN_TCP;
_conn->state = ESPCONN_NONE;
ets_memset(_conn->proto.tcp, 0, sizeof(esp_tcp));
//	_conn.proto.tcp->local_port = espconn_port();
_conn->proto.tcp->local_port = port;
_conn->reverse = 0;
if (espconn_accept(_conn) != ESPCONN_OK) {
	INFO("ERR : espconn_accept");
}
registerCb(_conn);
}

void Tcp::registerCb(struct espconn* pconn) {
pconn->reverse = this;
espconn_regist_connectcb(pconn, connectCb);
espconn_regist_disconcb(pconn, disconnectCb);
espconn_regist_recvcb(pconn, recvCb);
espconn_regist_sentcb(pconn, sendCb);
espconn_regist_write_finish(pconn, writeFinishCb);
logCb(__FUNCTION__, pconn);
}

