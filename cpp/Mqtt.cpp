//____________________________________________________________________________
//
//____________________________________________________________________________
#include "Mqtt.h"
/*****************************************************************************
 *  Generate next message id
 ******************************************************************************/
uint16_t gMessageId = 1;

uint16_t IROM Mqtt::nextMessageId() {
	return ++gMessageId;
}
//____________________________________________________________________________
//       M      Q       T       T
//  sends MQTT_CONNECTED, MQTT_DISCONNECTED
//  listens for TCPCONNECTED, TCP_DISCONNECTED
//____________________________________________________________________________

IROM Mqtt::Mqtt(MqttFramer* framer) :
		Handler("Mqtt"), _prefix(MQTT_SIZE_TOPIC), _mqttOut(MQTT_SIZE_VALUE), _framer(
				framer) {
	_mqttPublisher = new MqttPublisher(*this);
	_mqttSubscriber = new MqttSubscriber(*this);
	_mqttSubscription = new MqttSubscription(*this);
	_mqttPinger = new MqttPinger(this);

	_mqttPublisher->stop();
	_mqttSubscriber->stop();
	_mqttPinger->stop();
	_mqttSubscription->stop();
	_isConnected = false;
	_retries = 0;

}

IROM Mqtt::~Mqtt() {
}

bool IROM Mqtt::isConnected() {
	return _isConnected;
}

void IROM Mqtt::setPrefix(const char* s) {
	_prefix.clear() << s;
}

void IROM Mqtt::getPrefix(Str& s) {
	s << _prefix;
}

void IROM Mqtt::sendConnect() {
	Str str("false");
	Str online("system/online");
	char clientId[20];
	ets_sprintf(clientId, "ESP%X", system_get_chip_id());

	_mqttOut.Connect(MQTT_QOS2_FLAG, clientId, MQTT_CLEAN_SESSION,
			online.c_str(), str, "", "", TIME_KEEP_ALIVE / 1000);
	_framer->send(_mqttOut);
}
Str loggerLine(100);
Handler* IROM Mqtt::publish(Str& topic, Bytes& message, uint32_t flags) {
	loggerLine.clear() << topic << " : ";
	((Cbor&) message).toString(loggerLine);
	INFO(" publish : %s ", loggerLine.c_str());
	return _mqttPublisher->publish(topic, message, flags);
}

Handler* IROM Mqtt::subscribe(Str& topic) {
	return _mqttSubscription->subscribe(topic);
}

//________________________________________________________________________________________________
//
//
//________________________________________________________________________________________________

bool IROM Mqtt::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	_retries = 0;
	_isConnected = false;
	_mqttOut.prefix(_prefix);

	DISCONNECTED: {
//		INFO("DISCONNECTED");
//		_stream.disconnect();
		_isConnected = false;
		Msg::publish(this, SIG_DISCONNECTED);
		_mqttPinger->stop();
		_mqttPublisher->stop(); // don't start if nothing to publish !!
		_mqttSubscriber->stop();
		_mqttSubscription->stop();
		while (true) // DISCONNECTED STATE
		{
//			_stream.connect();
//			timeout(TIME_CONNECT);
//			PT_YIELD_UNTIL(_stream.isConnected() || timeout());
			PT_YIELD_UNTIL(msg.is(_framer, SIG_CONNECTED));
			goto STREAM_CONNECTED;
		}
	}
	STREAM_CONNECTED: {
//		INFO("STREAM_CONNECTED");
		while (true) // LINK_CONNECTED
		{
			sendConnect();
			timeout(20000);
			PT_YIELD_UNTIL(
					msg.is(_framer, SIG_RXD, MQTT_MSG_CONNACK) || msg.is(_framer, SIG_DISCONNECTED) || timeout());
			// wait reply or timeout on connect send
			if (msg.is(_framer, SIG_DISCONNECTED) || timeout())
				goto DISCONNECTED;
			if (msg.is(_framer, SIG_RXD, MQTT_MSG_CONNACK)) {
				Msg::publish(this, SIG_CONNECTED);
				_isConnected = true;
				_mqttSubscriber->restart();
				_mqttPinger->restart();
				goto RECEIVING;
			}
		}
	}
	RECEIVING: {
//		INFO("RECEIVING");

	}
	WAIT_DISCONNECT: {
//		INFO("WAIT_DISCONNECT");
		PT_YIELD_UNTIL(!_framer->isConnected());
		goto DISCONNECTED;
	}
PT_END()
}
//________________________________________________________________________________________________
//
//			MQTT PINGER
//________________________________________________________________________________________________

MqttPinger::MqttPinger(Mqtt* mqtt) :
	Handler("MqttPinger") {
_mqtt = mqtt;
_retries = 0;
}

Str str("system/uptime");
Str str2(20);
bool IROM MqttPinger::dispatch(Msg& msg) {
PT_BEGIN()
WAITING: { // while data arrives, reset timer
	while (true) {
		timeout((TIME_KEEP_ALIVE / 3));
		PT_YIELD_UNTIL(msg.is(_mqtt->_framer, SIG_RXD) || timeout());
		if (timeout()) {
			goto PINGING;
		}
	}

}
PINGING: {
	_retries = 1;
	while (true) {
		_mqtt->_mqttOut.PingReq();
		_mqtt->_framer->send(_mqtt->_mqttOut);
		timeout(TIME_PING);

		PT_YIELD_UNTIL(
				msg.is(_mqtt->_framer,SIG_RXD,MQTT_MSG_PINGRESP)||timeout());

		if (msg.is(_mqtt->_framer, SIG_RXD, MQTT_MSG_PINGRESP)) {
			goto WAITING;
		}
		_retries++;
	}
}

PT_END()
;
return true;
}

//____________________________________________________________________________
//
//       MQTT PUBLISHER : Publish at different QOS levels, do retries
//____________________________________________________________________________

IROM MqttPublisher::MqttPublisher(Mqtt& mqtt) :
Handler("Publisher"), _mqtt(mqtt), _topic(MQTT_SIZE_TOPIC), _message(
MQTT_SIZE_VALUE) {
_flags = 0;
_messageId = 0;
_retries = 0;
_state = ST_READY;
}

Handler* IROM MqttPublisher::publish(Str& topic, Bytes& msg, uint32_t flags) {
if (!_mqtt.isConnected())
return 0;
if (isRunning())
return 0;
_retries = 0;
_topic = topic;
_message = msg;
_messageId = Mqtt::nextMessageId();
_flags = flags;
restart();
return this;
}

void IROM MqttPublisher::sendPublish() {
uint8_t header = 0;
if ((_flags & MQTT_QOS_MASK) == MQTT_QOS0_FLAG) {
_state = ST_READY;
} else if ((_flags & MQTT_QOS_MASK) == MQTT_QOS1_FLAG) {
header += MQTT_QOS1_FLAG;
timeout(TIME_WAIT_REPLY);
} else if ((_flags & MQTT_QOS_MASK) == MQTT_QOS2_FLAG) {
header += MQTT_QOS2_FLAG;
timeout(TIME_WAIT_REPLY);
}
if (_flags & MQTT_RETAIN_FLAG)
header += MQTT_RETAIN_FLAG;
if (_retries) {
header += MQTT_DUP_FLAG;
}
_mqtt._mqttOut.Publish(header, _topic, _message, _messageId);
_mqtt._framer->send(_mqtt._mqttOut);
}

void IROM MqttPublisher::sendPubRel() {
_mqtt._mqttOut.PubRel(_messageId);
_mqtt._framer->send(_mqtt._mqttOut);
}

bool IROM MqttPublisher::dispatch(Msg& msg) {
PT_BEGIN()
//READY:
{
_state = ST_READY;
PT_YIELD_UNTIL(msg.is(0, SIG_TICK));
_state = ST_BUSY;
if ((_flags & MQTT_QOS_MASK) == MQTT_QOS0_FLAG) {
	sendPublish();
	PT_YIELD_UNTIL(msg.is(_mqtt._framer, SIG_TXD));
	Msg::publish(this, SIG_ERC, 0);
	PT_EXIT()
	;
} else if ((_flags & MQTT_QOS_MASK) == MQTT_QOS1_FLAG)
	goto QOS1_ACK;
else if ((_flags & MQTT_QOS_MASK) == MQTT_QOS2_FLAG)
	goto QOS2_REC;
PT_EXIT()
;
}
QOS1_ACK: {
// INFO("QOS1_ACK");
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
	sendPublish();
	timeout(TIME_WAIT_REPLY);
	PT_YIELD_UNTIL(msg.is(_mqtt._framer,SIG_RXD,MQTT_MSG_PUBACK) || timeout());
	if (msg.is(_mqtt._framer, SIG_RXD, MQTT_MSG_PUBACK)) {
		int id;
		msg.get(id); 	// skip type in  <src><signal><type><msgId><qos><topic><value>
		msg.get(id);
//				INFO(" messageId compare %d : %d ",id,_messageId);
		if (id == _messageId) {
			Msg::publish(this, SIG_ERC, 0);
			PT_EXIT()
			;
		}
	}
}
Msg::publish(this, SIG_ERC, ETIMEDOUT);
PT_EXIT()
;
}
QOS2_REC: {
// INFO("QOS2_REC");
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
	sendPublish();
	timeout(TIME_WAIT_REPLY);
	PT_YIELD_UNTIL(msg.is(_mqtt._framer,SIG_RXD,MQTT_MSG_PUBREC) || timeout());
	if (msg.is(_mqtt._framer, SIG_RXD, MQTT_MSG_PUBREC)) {
		int id;
		msg.get(id);
//				INFO(" messageId compare %d : %d ",id,_messageId);
		if (id == _messageId) {
			goto QOS2_COMP;
		}
	}
}
Msg::publish(this, SIG_ERC, ETIMEDOUT);
PT_EXIT()
;
}
QOS2_COMP: {
// INFO("QOS2_COMP");
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
	sendPubRel();
	timeout(TIME_WAIT_REPLY);
	PT_YIELD_UNTIL(msg.is(_mqtt._framer,SIG_RXD,MQTT_MSG_PUBCOMP) || timeout());
	if (msg.is(_mqtt._framer, SIG_RXD, MQTT_MSG_PUBCOMP)) {
		int id;
		if (msg.get(id) && id == _messageId) {
			Msg::publish(this, SIG_ERC, 0);
			PT_EXIT()
			;
		}
	}
}
Msg::publish(this, SIG_ERC, ETIMEDOUT);
PT_EXIT()
;
}
PT_END()
}

//____________________________________________________________________________
//
//       MQTT SUBSCRIBER : receive subscriptions, ack,rec
//____________________________________________________________________________

IROM MqttSubscriber::MqttSubscriber(Mqtt &mqtt) :
Handler("Subscriber"), _mqtt(mqtt), _topic(MQTT_SIZE_TOPIC), _message(
MQTT_SIZE_VALUE) {
_flags = 0;
_messageId = 0;
_retries = 0;
}

void IROM MqttSubscriber::sendPubRec() {
_mqtt._mqttOut.PubRec(_messageId);
_mqtt._framer->send(_mqtt._mqttOut);
timeout(TIME_WAIT_REPLY);
}

void IROM MqttSubscriber::callBack() {
loggerLine.clear() << _topic << " : ";
((Cbor&) _message).toString(loggerLine);
INFO(" PUBLISH received : %s ", loggerLine.c_str());
Msg pub(256);
Str _shortTopic(40);
_shortTopic.clear().substr(_topic, 4 + _mqtt._prefix.length()); // skip "/PUT" +<prefix.length>
pub.create(this, SIG_RXD).addf("SB", &_shortTopic, &_message);
pub.send();
}

// #define PT_WAIT_FOR( ___pt, ___signals, ___timeout ) listen(___signals,___timeout);PT_YIELD(___pt);

bool IROM MqttSubscriber::dispatch(Msg& msg) {
PT_BEGIN()

READY: {
timeout(100000000);
PT_YIELD_UNTIL(
	msg.is(_mqtt._framer, SIG_RXD, MQTT_MSG_PUBLISH) || !_mqtt.isConnected() || timeout());
if (msg.is(_mqtt._framer, SIG_RXD, MQTT_MSG_PUBLISH)) { // <type><msgId><qos><topic><value>
int _qos = 0;
int _type = 0;

_topic.clear();
_message.clear();

msg.scanf("iiiSS", &_type, &_messageId, &_qos, &_topic, &_message);

if (_qos == MQTT_QOS0_FLAG) {

	callBack();
} else if (_qos == MQTT_QOS1_FLAG) {

	callBack();
	_mqtt._mqttOut.PubAck(_messageId);
	_mqtt._framer->send(_mqtt._mqttOut);

} else if (_qos == MQTT_QOS2_FLAG) {
	goto QOS2_WAIT_REL;

}
} else if (!_mqtt.isConnected()) {
PT_EXIT()
;
}
goto READY;
}
QOS2_WAIT_REL: {
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
sendPubRec();
timeout(TIME_WAIT_REPLY);
PT_YIELD_UNTIL(
		!_mqtt.isConnected() || msg.is(_mqtt._framer,SIG_RXD,MQTT_MSG_PUBREL) || timeout());
if (msg.is(_mqtt._framer, SIG_RXD, MQTT_MSG_PUBREL)) {
	callBack();
	msg.scanf("i", &_messageId);
	_mqtt._mqttOut.PubComp(_messageId);
	_mqtt._framer->send(_mqtt._mqttOut);
	goto READY;
}
}
goto READY;
}
PT_END()
}

//____________________________________________________________________________
//       SUBSCRIPTION
//____________________________________________________________________________

//________________________________________________________________________________________________
//
//		MQTT SUBSCRIPTION : initiate a subscribe, handle retries
//________________________________________________________________________________________________

bool IROM MqttSubscription::dispatch(Msg& msg) {
PT_BEGIN()
//		INFO("BEGIN");
_messageId = Mqtt::nextMessageId();
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
sendSubscribe();
timeout(TIME_WAIT_REPLY);
PT_YIELD_UNTIL(
msg.is(_mqtt._framer, SIG_RXD, MQTT_MSG_SUBACK) || msg.is(_mqtt._framer, SIG_DISCONNECTED ) || timeout());

if (msg.is(_mqtt._framer, SIG_RXD, MQTT_MSG_SUBACK)) {

//	INFO("SIG_RXD");
int id;
if (msg.get(id) && id == _messageId) {
Msg::publish(this, SIG_ERC, 0);
PT_EXIT()
;
}
} else if (msg.is(_mqtt._framer, SIG_DISCONNECTED)) {
INFO("DISC");
Msg::publish(this, SIG_ERC, ECONNABORTED);
PT_EXIT()
;
}
}
Msg::publish(this, SIG_ERC, EAGAIN);
PT_EXIT()
;
PT_END()
;
}

IROM MqttSubscription::MqttSubscription(Mqtt & mqtt) :
Handler("Subscription"), _mqtt(mqtt), _topic(MQTT_SIZE_TOPIC) {
_retries = 0;
_messageId = 0;
}

Handler* IROM MqttSubscription::subscribe(Str& topic) {
INFO("subscribe %s", topic.c_str());
if (!_mqtt.isConnected())
return 0;
if (isRunning())
return 0;
// INFO("subscribe %s accepted ",topic.c_str());
_messageId = Mqtt::nextMessageId();
_topic = topic;
restart();
return this;
}

void IROM MqttSubscription::sendSubscribe() {
_mqtt._mqttOut.Subscribe(_topic, _messageId, 2);
_mqtt._framer->send(_mqtt._mqttOut);
_retries++;
}
