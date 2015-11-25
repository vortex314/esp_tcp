/*
 * MqttSeq.h
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#ifndef MQTT_H_
#define MQTT_H_

// #include "Event.h"
//#include "Sequence.h"
#include "stdint.h"
#include "Stream.h"
#include "CircBuf.h"
#include "MqttConstants.h"
#include "Log.h"
#include "MqttMsg.h"
#include <CborQueue.h>
#include "Handler.h"

//#include "Event.h"
#include "Msg.h"

//************************************** CONSTANTS ****************************
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 2000
#define TIME_CONNECT 5000
#define TIME_WAIT_CONNECT 5000
#define	TIME_PING ( TIME_KEEP_ALIVE /3 )
#define TIME_FOREVER UINT32_MAX
#define MQTT_SIZE_TOPIC	40
#define MQTT_SIZE_VALUE	256
#define MQTT_SIZE_MESSAGE 	300
#define MAX_RETRIES	4

class MqttPublisher;
class MqttSubscriber;
class MqttSubscription;
class MqttPinger;
class Mqtt;
#include "MqttFramer.h"

enum {
	CMD_MQTT_PUBLISH,	// topic, bytes, flags
	CMD_MQTT_PUBLISH_TOPIC // topic ptr
};

class Mqtt: public Handler {
public:
	Str _prefix;
	MqttSubscriber* _mqttSubscriber;
	MqttPublisher* _mqttPublisher;
	MqttSubscription* _mqttSubscription;
	MqttPinger* _mqttPinger;
	MqttMsg _mqttOut; //
	bool _isConnected;
	MqttFramer* _framer;
	CborQueue _queue;

private:
	uint32_t _retries;

public:
	IROM Mqtt(MqttFramer* framer);
	IROM ~Mqtt();
	IROM void sendConnect();

	IROM bool dispatch(Msg& msg);
	IROM static uint16_t nextMessageId();
	IROM void getPrefix(Str& prefix);
	IROM void setPrefix(const char * prefix);
	IROM bool isConnected();
	IROM Handler* subscribe(Str& topic);
	IROM Handler* publish(Str& topic, Bytes& message, uint32_t flags);
private:
	IROM void sendSubscribe(uint8_t flags);
};

class MqttPinger: public Handler {
	Mqtt* _mqtt;
	uint16_t _retries;
public:
	IROM MqttPinger(Mqtt* mqtt);
	IROM bool dispatch(Msg& msg);
};

class MqttSubscriber: public Handler {
public:
	IROM MqttSubscriber(Mqtt& mqtt);
	IROM bool dispatch(Msg& msg);
	IROM void sendPubRec();
	IROM void callBack();
	// will invoke
private:
	Mqtt& _mqtt;
	Str _topic;
	Bytes _message;
	uint32_t _flags;
	uint16_t _messageId;
	uint16_t _retries;
};

class MqttPublish {
public:
	Str topic;
	Bytes message;
	uint32_t flags;
	IROM MqttPublish(int topicSize, int messageSize) :
			topic(topicSize), message(messageSize) {
	}
};

class MqttPublisher: public Handler {
public:
	IROM MqttPublisher(Mqtt& mqtt);
	IROM bool dispatch(Msg& msg);
	IROM Handler* publish(Str& topic, Bytes& msg, uint32_t flags);
	// will send PUB_OK,PUB_FAIL
private:
	IROM void sendPublish();
	IROM void sendPubRel();
	Mqtt& _mqtt;
	enum State {
		ST_READY, ST_BUSY,
	} _state;
	Str _topic;
	Bytes _message;
	uint16_t _messageId;
	uint32_t _flags;
	uint16_t _retries;
};

class MqttSubscription: public Handler {
public:
	IROM MqttSubscription(Mqtt& mqtt);
	IROM bool dispatch(Msg& msg);
	IROM Handler* subscribe(Str& topic);
private:
	Mqtt& _mqtt;
	uint16_t _retries;
	uint16_t _messageId;
	Str _topic;
	IROM void sendSubscribe();
};

#endif /* MQTT_H_ */
