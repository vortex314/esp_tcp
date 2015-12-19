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
public:
	static Topic* _first;
	Topic* _next;
	const char* _name;
	Xdr _putter;
	Xdr _getter;
	void* _instance;
	int _flags;

public:

	enum Flags {
		F_QOS0 = 0, //
		F_QOS1 = MQTT_QOS1_FLAG, // 2
		F_QOS2 = MQTT_QOS2_FLAG, // 4
		F_RETAIN = MQTT_RETAIN_FLAG, // 1
		F_NO_POLL = 8
	};

	IROM Topic(const char* name, void* instance, Xdr putter, Xdr getter,
			int flags); //
	IROM virtual ~Topic(); //
	IROM static Topic* first(); //
	IROM Topic* next(); //
	IROM const char* getName() {
		return _name;
	}
	IROM bool match(Str& name); //
	IROM Erc putter(Cbor& cbor); //
	IROM Erc getter(Cbor& cbor); //
	IROM bool hasGetter() {
		return _getter != 0;
	}
	IROM int flags() {
		return _flags;
	}
	IROM void changed(); //
	IROM static Erc getInteger(void *instance, Cbor& bytes); //
	IROM static Erc getUI32(void *instance, Cbor& bytes); //
	IROM static Erc getString(void *instance, Cbor& bytes); //
	IROM static Erc getConstantChar(void *instance, Cbor& bytes); //
	IROM static Erc getConstantInt(void *instance, Cbor& bytes); //
	IROM static Erc getConstantBoolean(void *instance, Cbor& bytes); //
	IROM static Topic* find(Str& str); //
	IROM int getFlags() const; //
	IROM Xdr getGetter() const; //
	IROM void* getInstance() const; //
	IROM const Topic*& getNext() const; //
	IROM Xdr getPutter() const;
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
	IROM TopicPublisher(Mqtt* mqtt);IROM virtual ~TopicPublisher();IROM bool dispatch(
			Msg& msg);
};

class TopicSubscriber: public Handler {
private:
	Mqtt* _mqtt;
	Str _topic;
	Cbor _value;
	Handler* _src;

	Str _mqttErrorString;
public:
	IROM TopicSubscriber(Mqtt* mqtt); //
	IROM virtual ~TopicSubscriber(); //
	IROM bool dispatch(Msg& msg);IROM void error(Str& str);
};

#endif /* TOPIC_H_ */
