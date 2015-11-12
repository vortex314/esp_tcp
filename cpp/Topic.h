/*
 * Topic.h
 *
 *  Created on: Nov 9, 2015
 *      Author: lieven
 */

#ifndef TOPIC_H_
#define TOPIC_H_

#include "Sys.h"
#include "Cbor.h"
#include "Erc.h"
#include "Mqtt.h"
#include "Msg.h"

typedef Erc (*Xdr)(void* instance, Cbor& bytes);

class Topic {

	static Topic* _first;
	Topic* _next;
	const char* _name;
	Xdr _putter;
	Xdr _getter;
	void* _instance;
	int _flags;


public:

	enum Flags {
		F_QOS0 = 0,
		F_QOS1 = MQTT_QOS1_FLAG,
		F_QOS2 = MQTT_QOS2_FLAG,
		F_RETAIN = MQTT_RETAIN_FLAG,
	};
	Topic(const char* name, void* instance, Xdr putter, Xdr getter, int flags);
	virtual ~Topic();
	static Topic* first();
	Topic* next();
	const char* getName() {
		return _name;
	}
	bool match(Str& name);
	Erc putter(Cbor& cbor);
	Erc getter(Cbor& cbor);
	bool hasGetter() {
		return _getter != 0;
	}
	int flags() {
		return _flags;
	}
	void changed();
	static Erc getInteger(void *instance, Cbor& bytes);
	static Erc getUI32(void *instance, Cbor& bytes);
	static Erc getString(void *instance, Cbor& bytes);
	static Erc getConstantInt(void *instance, Cbor& bytes);
	static Erc getConstantBoolean(void *instance, Cbor& bytes);
	static Topic* find(Str& str);

};

class TopicPublisher: public Handler {
private:
	Mqtt* _mqtt;
	Str _topic;
	Cbor _value;
	static Topic* _mqttError;
	Topic* _currentTopic;
	Topic* _changedTopic;
	Str _mqttErrorString;
	void nextTopic();
public:
	TopicPublisher(Mqtt* mqtt);
	virtual ~TopicPublisher();
	bool dispatch(Msg& msg);
};

class TopicSubscriber: public Handler {
private:
	Mqtt* _mqtt;
	Str _topic;
	Cbor _value;
	Handler* _src;

	Str _mqttErrorString;
public:
	TopicSubscriber(Mqtt* mqtt);
	virtual ~TopicSubscriber();
	bool dispatch(Msg& msg);
};

#endif /* TOPIC_H_ */
