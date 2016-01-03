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

template<class T>
class List {
public:
	List();
	Erc add(T&) {
		if (_first == 0) {
			_first = new List<T>();
		} else {

		}
		return E_OK;
	}
	uint32_t size();
private:
	static List* _first;
	List* _next;
	T& t;
};

typedef struct {
	IpAddress _host;
	uint16_t _port;
	Tcp* _instance;
} Callback;

//List<Callback> tcps();

uint32_t Tcp::_maxConnections = 5;
Tcp** Tcp::_tcps = 0;

void Tcp::globalInit(Wifi* wifi, uint32_t maxConnections) {
	_tcps = new (Tcp*[5]);
	uint32_t i;
	for (i = 0; i < _maxConnections; i++)
		_tcps[i] = new Tcp(wifi);
}

void Tcp::logConn(const char* s, void* arg) {
	struct espconn* pconn = (struct espconn*) arg;
	if (pconn)
		INFO(
				" %s -- this : %x/%x  esp : %x , tcp :  %x  , ip : %d.%d.%d.%d:%d ",
				s, this, this->_conn, pconn, pconn->reverse,
				pconn->proto.tcp->remote_ip[0], pconn->proto.tcp->remote_ip[1],
				pconn->proto.tcp->remote_ip[2], pconn->proto.tcp->remote_ip[3],
				pconn->proto.tcp->remote_port);
}

void Tcp::loadEspconn(struct espconn* pconn) {
	_conn = pconn;
	ets_sprintf(_host, "%d.%d.%d.%d", pconn->proto.tcp->remote_ip[0],
			pconn->proto.tcp->remote_ip[1], pconn->proto.tcp->remote_ip[2],
			pconn->proto.tcp->remote_ip[3]);
	_remote_port = pconn->proto.tcp->remote_port;
	os_memcpy(&_remote_ip, pconn->proto.tcp->remote_ip, 4);
}

bool Tcp::match(struct espconn* pconn, Tcp* pTcp) {
/*	INFO(" compare %X:%d : %X:%d ", *((uint32_t* )pTcp->_remote_ip.b),
			pTcp->_remote_port, *((uint32_t* )pconn->proto.tcp->remote_ip),
			pconn->proto.tcp->remote_port);*/

	if (pTcp->_remote_port == pconn->proto.tcp->remote_port)
		if (memcmp(pTcp->_remote_ip.b, pconn->proto.tcp->remote_ip, 4) == 0)
			return true;
	return false;
}

Tcp* Tcp::findTcp(struct espconn* pconn) {
	uint32_t i;
	for (i = 0; i < _maxConnections; i++) {
		if (Tcp::match(pconn, _tcps[i]))
			return _tcps[i];
	}
	return 0;
}

Tcp* Tcp::findFreeTcp(struct espconn* pconn) {
	uint32_t i;
	for (i = 0; i < _maxConnections; i++) {
		if (_tcps[i]->isConnected() == false)
			return _tcps[i];
	}
	return 0;
}

void Tcp::registerCb(struct espconn* pconn) {

	pconn->reverse = this;
	espconn_regist_time(pconn, 1000, 1);		// disconnect after 1000 sec
	espconn_regist_connectcb(pconn, connectCb);
	espconn_regist_disconcb(pconn, disconnectCb);
	espconn_regist_recvcb(pconn, recvCb);
	espconn_regist_sentcb(pconn, sendCb);
	espconn_regist_write_finish(pconn, writeFinishCb);
	logConn(__FUNCTION__, pconn);
}

Tcp* getInstance(void *arg) {
	struct espconn *pconn = (struct espconn *) arg;
	Tcp* pTcp = (Tcp*) pconn->reverse;
	if (!Tcp::match(pconn, pTcp)) {
		INFO(" !!!!!!!!!!!!!!!! espconn and pTcp don't match ");
		pTcp = Tcp::findTcp(pconn);
	}
//	ll.first();
	return pTcp;
}

//const char* TCP_OS = "TCP_OS";

uint8_t StrToIP(const char* str, void *ip);

extern Wifi* wifi;

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

	_connected = true;
	loadEspconn(pconn);

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
	logConn(__FUNCTION__, _conn);
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

	struct espconn* pconn = (struct espconn*) arg;
//	Tcp *pTcp = new Tcp(wifi, pconn);
	Tcp* pTcp = findFreeTcp(pconn);

	if (pTcp) {

		pTcp->loadEspconn(pconn);
		pTcp->logConn(__FUNCTION__, arg);
		pTcp->registerCb(pconn);

		espconn_set_opt(pconn, ESPCONN_NODELAY);
		espconn_set_opt(pconn, ESPCONN_COPY);
		espconn_set_keepalive(pconn, ESPCONN_KEEPALIVE, (void*) 1000);
		espconn_regist_time(pconn, 1000, 0);

		Msg::publish(pTcp, SIG_CONNECTED);
		pTcp->_connected = true;
	} else {
		INFO(" no free TCP : disconnecting ");
		espconn_disconnect(pconn);
	}
	return;
}

void Tcp::disconnectCb(void* arg) {
	struct espconn* pconn = (struct espconn*) arg;
//	Tcp *pTcp = getInstance(pconn);
	Tcp *pTcp = findTcp(pconn);
	if (pTcp) {
		pTcp->logConn(__FUNCTION__, arg);
		Msg::publish(pTcp, SIG_DISCONNECTED);
		pTcp->_connected = false;
	} else
		INFO("connection not found");
	return;
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
	struct espconn* pconn = (struct espconn*) arg;
	//	Tcp *pTcp = getInstance(pconn);
	Tcp *pTcp = findTcp(pconn);
	pTcp->logConn(__FUNCTION__, arg);
	return;

	pTcp->send();
	Msg::publish(pTcp, SIG_TXD);
}

void Tcp::sendCb(void *arg) {
	struct espconn* pconn = (struct espconn*) arg;
	//	Tcp *pTcp = getInstance(pconn);
	Tcp *pTcp = findTcp(pconn);
	pTcp->logConn(__FUNCTION__, arg);
	return;

//	INFO("TCP: Txd %s:%d ", pTcp->_host, pTcp->_remote_port);
	pTcp->send();
	Msg::publish(pTcp, SIG_TXD);
}

#include <Message.h>

Message message(100);

void Tcp::recvCb(void* arg, char *pdata, unsigned short len) {
	if (message.length() == 0) {
		message.addField(Message::DST_PROP, 1);
		message.addField(Message::TEXT, "Hello world\n");
	}
	INFO(" bytes : %d ", len);
	struct espconn* pconn = (struct espconn*) arg;
	//	Tcp *pTcp = getInstance(pconn);
	Tcp *pTcp = findTcp(pconn);
	if (pTcp) {
		pTcp->logConn(__FUNCTION__, arg);
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
	} else {
		INFO(" Tcp not found ");
	}
}

void Tcp::reconnectCb(void* arg, int8_t err) {
	struct espconn* pconn = (struct espconn*) arg;
	//	Tcp *pTcp = getInstance(pconn);
	Tcp *pTcp = findTcp(pconn);
	pTcp->logConn(__FUNCTION__, arg);
	return;

	INFO("TCP: Reconnect %s:%d err : %d", pTcp->_host, pTcp->_remote_port, err);
	Msg::publish(pTcp, SIG_DISCONNECTED);
	pTcp->_connected = false;
}

void Tcp::dnsFoundCb(const char *name, ip_addr_t *ipaddr, void *arg) {
	//	Tcp *pTcp = getInstance(pconn);
	Tcp *pTcp = findTcp((struct espconn*) arg);
	pTcp->logConn(__FUNCTION__, arg);

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
	WIFI_DISCONNECTED: {
		while (true) {
			PT_YIELD_UNTIL(_wifi->isConnected());
			INFO("Tcp started. %x", this);
			goto CONNECTING;
		}
	}
	CONNECTING: {
		while (true) {
			timeout(100000);
			PT_YIELD_UNTIL(isConnected() || timeout());
			if (isConnected()) {
				goto CONNECTED;
			}
		}
	}
	CONNECTED: {
		while (true) {
			PT_YIELD_UNTIL(!isConnected() || !_wifi->isConnected());
			if (!_wifi->isConnected())
				goto WIFI_DISCONNECTED;
			if (!isConnected())
				goto CONNECTING;
		}
	}
	;
PT_END()
;
return true;
}

TcpClient::TcpClient(Wifi* wifi) :
	Tcp(wifi) {
}

void TcpClient::config(const char* host, uint16_t port) {
strncpy(_host, host, sizeof(_host));
_remote_port = port;
logConn(__FUNCTION__, _conn);
}

bool TcpClient::dispatch(Msg& msg) {
PT_BEGIN()
;
WIFI_DISCONNECTED: {
	while (true) {
		PT_YIELD_UNTIL(_wifi->isConnected());
		INFO("TcpClient started. %x", this);
		goto CONNECTING;
	}
}
CONNECTING: {
	while (true) {
		connect();
		PT_YIELD_UNTIL(isConnected());
		if (isConnected()) {
			goto CONNECTED;
		}
	}
}
CONNECTED: {
	while (true) {
		PT_YIELD_UNTIL(!isConnected() || !_wifi->isConnected());
		if (!_wifi->isConnected())
			goto WIFI_DISCONNECTED;
		if (!isConnected())
			goto CONNECTING;
	}
}
;
PT_END()
;
return true;
}

TcpServer::TcpServer(Wifi* wifi) :
Tcp(wifi) {
_local_port = 23;
logConn(__FUNCTION__, _conn);
_connected = true;	// to allocate TCP instance
}

Erc TcpServer::config(uint32_t maxConnections, uint16_t port) {
_local_port = port;
logConn(__FUNCTION__, _conn);
return E_OK;
}

bool TcpServer::dispatch(Msg& msg) {
PT_BEGIN()
;
WIFI_DISCONNECTED: {
while (true) {
	PT_YIELD_UNTIL(_wifi->isConnected());
	INFO("TcpServer started. %x", this);
	goto CONNECTING;
}
}
CONNECTING: {
while (true) {
	listen();
	timeout(999999999);
	PT_YIELD_UNTIL(isConnected());
	goto CONNECTED;
}
}
CONNECTED: {
while (true) {
	PT_YIELD_UNTIL(!isConnected() || !_wifi->isConnected());
	if (!_wifi->isConnected())
		goto WIFI_DISCONNECTED;
	if (!isConnected())
		goto CONNECTING;
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

void TcpServer::listen() {
_conn->type = ESPCONN_TCP;
_conn->state = ESPCONN_NONE;
ets_memset(_conn->proto.tcp, 0, sizeof(esp_tcp));
//	_conn.proto.tcp->local_port = espconn_port();
_conn->proto.tcp->local_port = _local_port;
_conn->reverse = 0;
if (espconn_accept(_conn) != ESPCONN_OK) {
INFO("ERR : espconn_accept");
}
registerCb(_conn);
logConn(__FUNCTION__, _conn);
}

