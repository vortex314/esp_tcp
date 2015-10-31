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

#include "Handler.h"
#include "Flags.h"
//#include "Event.h"
#include "Msg.h"

//************************************** CONSTANTS ****************************
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 2000
#define TIME_CONNECT 5000
#define TIME_WAIT_CONNECT 5000
#define	TIME_PING ( TIME_KEEP_ALIVE /3 )
#define TIME_FOREVER UINT32_MAX
#define SIZE_TOPIC	40
#define SIZE_MESSAGE	256
#define SIZE_MQTT	300
#define MAX_RETRIES	4

class MqttPublisher;
class MqttSubscriber;
class MqttSubscription;
class MqttPinger;
class Mqtt;
#include "MqttFramer.h"

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

private:
	uint32_t _retries;

public:
	Mqtt(MqttFramer* framer);
	~Mqtt();
	void sendConnect();

	bool dispatch(Msg& msg);
	static uint16_t nextMessageId();
	void getPrefix(Str& prefix);
	void setPrefix(const char * prefix);
	bool isConnected();
	Handler* subscribe(Str& topic);
	Handler* publish(Str& topic,Bytes& message,Flags flags);
private:
	void sendSubscribe(uint8_t flags);
};

class MqttPinger: public Handler {
	Mqtt* _mqtt;
	uint16_t _retries;
public:
	MqttPinger(Mqtt* mqtt);
	bool dispatch(Msg& msg);
};


class MqttSubscriber: public Handler {
public:
	MqttSubscriber(Mqtt& mqtt);
	bool dispatch(Msg& msg);
	void sendPubRec();
	void callBack();
	// will invoke
private:
	Mqtt& _mqtt;
	Str _topic;
	Bytes _message;
	Flags _flags;
	uint16_t _messageId;
	uint16_t _retries;
};

class MqttPublish {
public:
	Str topic;
	Bytes message;
	Flags flags;
	MqttPublish(int topicSize, int messageSize) :
			topic(topicSize), message(messageSize) {
	}
};

class MqttPublisher: public Handler  {
public:
	MqttPublisher(Mqtt& mqtt);
	bool dispatch(Msg& msg);
	Handler* publish(Str& topic, Bytes& msg, Flags flags);
	// will send PUB_OK,PUB_FAIL
private:
	void sendPublish();
	void sendPubRel();
	Mqtt& _mqtt;
	enum State {
		ST_READY, ST_BUSY,
	} _state;
	Str _topic;
	Bytes _message;
	uint16_t _messageId;
	Flags _flags;
	uint16_t _retries;
};

class MqttSubscription: public Handler {
public:
	MqttSubscription(Mqtt& mqtt);
	bool dispatch(Msg& msg);
	Handler* subscribe(Str& topic);
private:
	Mqtt& _mqtt;
	uint16_t _retries;
	uint16_t _messageId;
	Str _topic;
	void sendSubscribe();
};


#endif /* MQTT_H_ */
