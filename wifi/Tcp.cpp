/*
 * Tcp.cpp
 *
 *  Created on: Oct 24, 2015
 *      Author: lieven
 */

#include "Tcp.h"

//const char* TCP_OS = "TCP_OS";

uint8_t StrToIP(const char* str, void *ip);

IROM Tcp::Tcp(Wifi* wifi) :
		Handler("Tcp"), _rxd(256), _txd(256), _buffer(100) {
	_wifi = wifi;
//	INFO("TCP : ctor");
	os_strcpy(_host, "www.google.com");
	_dstPort = 80;
	_ip.addr = 0;
	_connected = false;
	ets_memset(&_conn, 0, sizeof(_conn));
	_conn.type = ESPCONN_TCP;
	_conn.state = ESPCONN_NONE;
	_conn.proto.tcp = (esp_tcp *) malloc(sizeof(esp_tcp));
	ets_memset(_conn.proto.tcp, 0, sizeof(esp_tcp));
	_bytesTxd = 0;
	_bytesRxd = 0;
	_overflowTxd = 0;
	_connections = 0;
	_srcPort = 0;
}

IROM Tcp::~Tcp() {
	if (_conn.proto.tcp)
		free(_conn.proto.tcp);
}

Erc IROM Tcp::write(Bytes& bytes) {
	return write(bytes.data(), bytes.length());
}

Erc IROM Tcp::write(uint8_t* pb, uint32_t length) {
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

Erc IROM Tcp::write(uint8_t data) {
	if (_txd.hasSpace())
		_txd.write(data);
	else {
		_overflowTxd++;
	}
	send();
	return E_OK;
}

uint8_t IROM Tcp::read() {
	return _rxd.read();
}

IROM bool Tcp::hasData() { // not in IROM as it will be called in interrupt
	return _rxd.hasData();
}

bool IROM Tcp::hasSpace() {
	return _txd.hasSpace();
}

Tcp* IROM getInstance(void *arg) {
	struct espconn *pCon = (struct espconn *) arg;
	return (Tcp*) pCon->reverse;
}
//	callkback when tcp connection is established
//_________________________________________________________
//
void IROM Tcp::connectCb(void *arg) {
	Tcp *pTcp = getInstance(arg);

	espconn_regist_disconcb(&pTcp->_conn, Tcp::disconnectCb);
	espconn_regist_recvcb(&pTcp->_conn, Tcp::recvCb);
	espconn_regist_sentcb(&pTcp->_conn, Tcp::sentCb);
	espconn_set_opt(&pTcp->_conn, ESPCONN_NODELAY);
	espconn_set_opt(&pTcp->_conn, ESPCONN_COPY);
	espconn_regist_write_finish(&pTcp->_conn, Tcp::sentCb);
	INFO("TCP: Connected to %s:%d", pTcp->_host, pTcp->_dstPort);

	Msg::publish(pTcp, SIG_CONNECTED);
	pTcp->_connected = true;
}

void IROM Tcp::disconnectCb(void *arg) {
	Tcp *pTcp = getInstance(arg);

	INFO("TCP: Disconnected %s:%d ", pTcp->_host, pTcp->_dstPort);
	pTcp->_ip.addr = 0;
	Msg::publish(pTcp, SIG_DISCONNECTED);
	pTcp->_connected = false;
}

void IROM Tcp::send() { // send buffered data, max 100 bytes
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
		int8_t erc = espconn_sent(&_conn, _buffer.data(), _buffer.length());
		if (erc == 0) {
			_bytesTxd += _buffer.length();
			_buffer.clear();
//		INFO(" TCP:send %d bytes", length);
		} else {
			return;
		}
	}
}

void IROM Tcp::sentCb(void *arg) {
	Tcp *pTcp = getInstance(arg);

//	INFO("TCP: Txd %s:%d ", pTcp->_host, pTcp->_dstPort);
	pTcp->send();
	Msg::publish(pTcp, SIG_TXD);
}

void IROM Tcp::recvCb(void *arg, char *pdata, unsigned short len) {
	Tcp *pTcp = getInstance(arg);

//	INFO("TCP: Rxd %s:%d length : %d", pTcp->_host, pTcp->_dstPort, len);
	pTcp->_bytesRxd += len;
	int i;
	for (i = 0; i < len; i++) {
//		INFO("0x%X",pdata[i]);
		pTcp->_rxd.write(pdata[i]);
	}
	Msg::publish(pTcp, SIG_RXD);
}

void IROM Tcp::reconnectCb(void *arg, int8_t err) {
	Tcp *pTcp = getInstance(arg);

	INFO("TCP: Reconnect %s:%d err : %d", pTcp->_host, pTcp->_dstPort, err);
	Msg::publish(pTcp, SIG_DISCONNECTED);
	pTcp->_connected = false;
}

void IROM Tcp::dnsFoundCb(const char *name, ip_addr_t *ipaddr, void *arg) {
	Tcp *pTcp = getInstance(arg);

	if (ipaddr == NULL) {
		INFO("DNS: Found, but got no ip, try to reconnect");
		return;
	};
	INFO("DNS: found ip %d.%d.%d.%d", *((uint8 * ) &ipaddr->addr),
			*((uint8 * ) &ipaddr->addr + 1), *((uint8 * ) &ipaddr->addr + 2),
			*((uint8 * ) &ipaddr->addr + 3));

	if (pTcp->_ip.addr == 0 && ipaddr->addr != 0) {
		memcpy(pTcp->_conn.proto.tcp->remote_ip, &ipaddr->addr, 4);
		espconn_connect(&(pTcp->_conn));
		INFO("TCP: connecting...");
	}
}

void IROM Tcp::config(const char* host, uint16_t port) {
	strncpy(_host, host, sizeof(_host));
	_dstPort = port;
}
//____________________________________________________________
//			Tcp::connect
//____________________________________________________________

void IROM Tcp::connect() {
	INFO(" Connecting ");
//	ets_memset(&_conn, 0, sizeof(_conn));
	_conn.type = ESPCONN_TCP;
	_conn.state = ESPCONN_NONE;
//	_conn.proto.tcp = (esp_tcp *) malloc(sizeof(esp_tcp));
	ets_memset(_conn.proto.tcp, 0, sizeof(esp_tcp));
	_conn.proto.tcp->local_port = espconn_port();
	_conn.proto.tcp->remote_port = _dstPort;
	_conn.reverse = this;
	espconn_regist_connectcb(&_conn, Tcp::connectCb);
	espconn_regist_reconcb(&_conn, Tcp::reconnectCb);
	if (StrToIP(_host, &_conn.proto.tcp->remote_ip)) {
		INFO("TCP: Connect to ip  %s:%d", _host, _dstPort);
		espconn_connect(&_conn);
	} else {
		INFO("TCP: Connect to domain %s:%d", _host, _dstPort);
		espconn_gethostbyname(&_conn, _host, &_ip, Tcp::dnsFoundCb);
	}
	INFO(" Heap size : %d", system_get_free_heap_size());
}

void IROM Tcp::disconnect() {
	espconn_disconnect(&_conn);
}

bool IROM Tcp::isConnected() {
	return _connected;
}
//____________________________________________________________
//
//____________________________________________________________
bool IROM Tcp::dispatch(Msg& msg) {
	PT_BEGIN()
	;
	INIT: {
		PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
		INFO(" SIG_INIT");
		goto WIFI_DISCONNECTED;
	}
	;
	WIFI_DISCONNECTED: {
		while (true) {
			PT_YIELD_UNTIL(msg.is(_wifi, SIG_CONNECTED));
			goto CONNECTING;
		}
	}
	;
	CONNECTING: {
		while (true) {
			connect();
			timeout(3000);
			PT_YIELD_UNTIL(
					msg.is(this, SIG_DISCONNECTED)
							|| msg.is(this, SIG_CONNECTED) || timeout());
			if (msg.is(this, SIG_CONNECTED))
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

IROM uint8_t StrToIP(const char* str, void *ip) {
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

