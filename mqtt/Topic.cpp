/*
 * Topic.cpp
 *
 *  Created on: Nov 9, 2015
 *      Author: lieven
 */

#include <Topic.h>

Topic* Topic::_first = 0;

IROM Topic::Topic(const char* name, void* instance, Xdr putter, Xdr getter,
		int flags) {
	INFO(" add topic : %s ", name);
	_name = name;
	_instance = instance;
	_putter = putter;
	_getter = getter;
	_flags = flags;
	_next = 0;
	if (_first == 0)
		_first = this;
	else {
		Topic* cursor = _first;
		while (cursor->_next) {
			cursor = cursor->_next;
		}
		cursor->_next = this;
	}
}

IROM Topic::~Topic() {

}

IROM Topic* Topic::first() {
	return _first;
}
IROM Topic* Topic::next() {
	return _next;
}
IROM bool Topic::match(Str& name) {
	return false;
}
IROM Erc Topic::putter(Cbor& cbor) {
	return _putter(_instance, cbor);
}
IROM Erc Topic::getter(Cbor& cbor) {
	if (_getter == 0) {
//		INFO(" ######### %s _getter : %x ", _name, _getter);
//		INFO(" #########_instance : %x ", _instance);
		return EADDRNOTAVAIL;
	}

	return _getter(_instance, cbor);
}

IROM void Topic::changed() {
	Msg::publish(this, SIG_CHANGE);
}

IROM Erc Topic::getInteger(void *instance, Cbor& cbor) {
	cbor.add(*(int*) instance);
	return E_OK;
}

IROM Erc Topic::getUI32(void *instance, Cbor& cbor) {
	cbor.add(*(uint32_t*) instance);
	return E_OK;
}

IROM Erc Topic::getString(void *instance, Cbor& cbor) {
	cbor.add(*(Str*) instance);
	return E_OK;
}

IROM Erc Topic::getConstantInt(void *instance, Cbor& cbor) {
	cbor.add((int) instance);
	return E_OK;
}

IROM Erc Topic::getConstantBoolean(void *instance, Cbor& cbor) {
	cbor.add((bool) instance);
	return E_OK;
}

IROM Erc Topic::getConstantChar(void *instance, Cbor& cbor) {
	cbor.add((const char*) instance);
	return E_OK;
}

IROM Topic* Topic::find(Str& str) {
	Topic *pt = 0;
//	INFO(" find %s", str.c_str());
	for (pt = first(); pt != 0; pt = pt->next()) {
		if (strncmp(pt->_name, str.c_str(), str.length()) == 0)
			break;
	}
	return pt;
}

//*********************************************************************************

IROM TopicSubscriber::TopicSubscriber(Mqtt* mqtt) :
		Handler("TopicMgr"), _topic(MQTT_SIZE_TOPIC), _value(MQTT_SIZE_VALUE), _mqttErrorString(
				100) {
	_mqtt = mqtt;
	_src = 0;
}

IROM TopicSubscriber::~TopicSubscriber() {
	_mqttErrorString.clear() << "subscriber started ";
}

void TopicSubscriber::error(Str& str) {
	_mqtt->publish("mqtt/error", str, 0);
}

Str _tempStr(100);

IROM bool TopicSubscriber::dispatch(Msg& msg) {
	Erc erc;
	Topic* pt;
	if (msg.is(_mqtt, SIG_DISCONNECTED)) {
		restart();
		return true;
	}
	PT_BEGIN()
	WAIT_CONNECT: {
		PT_WAIT_UNTIL(_mqtt->isConnected());
	}
	{
//-------------------------------------------------- subscribe to PUT/.../#
//		INFO("subscribe");
		_tempStr.clear() << "PUT/";
		_mqtt->getPrefix(_tempStr);
		_tempStr << "#";
		_src = _mqtt->subscribe(_tempStr);	// TODO could be zero
//		INFO("subscribing : %X", _src);
		PT_YIELD_UNTIL(_mqtt->_mqttSubscription->isReady());
	}
//-------------------------------------------------- wait PUT cmd
	while (true) {
		_topic.clear();
		_value.clear();
		PT_YIELD_UNTIL(msg.is(_mqtt->_mqttSubscriber, SIG_RXD));
//		INFO(" PUBLISH received ");
		if (msg.scanf("SB", &_topic, &_value)) {
//			INFO(" PUBLISH scanned %s  ", _topic.c_str());
			if ((pt = Topic::find(_topic))) {
//				INFO(" found topic :%s to update ", pt->getName());
				if (pt->getPutter() != 0) {
					_value.offset(0);
					erc = pt->putter(_value);
					if (erc) {
						ERROR(" put failed on topic ");
					}
				} else {
					ERROR("  topic has no put ");
				}
			} else {
				ERROR(" topic not found ");
			}
		} else {
			ERROR(" scanf failed ");
		}
	}
PT_END()
}

IROM TopicPublisher::TopicPublisher(Mqtt* mqtt) :
	Handler("TopicMgr"), _topic(MQTT_SIZE_TOPIC), _value(MQTT_SIZE_VALUE) {
_mqtt = mqtt;
_currentTopic = Topic::first();
_changedTopic = 0;

_mqttErrorString.clear() << "publisher started ";

}

IROM TopicPublisher::~TopicPublisher() {

}

IROM void TopicPublisher::nextTopic() {
uint32_t count = 0;
while (true) {
	_currentTopic = _currentTopic->next();
	if (_currentTopic == 0)
		_currentTopic = Topic::first();
	if (_currentTopic->hasGetter()
			&& ((_currentTopic->flags() & Topic::F_NO_POLL) == 0))
		break;
	if (count++ > 100) {
		ERROR(" no next Topic ! %s", _currentTopic->getName());
	}
}
}

IROM bool TopicPublisher::dispatch(Msg& msg) { // send changed topics or one by one
Topic* topic = 0;
Erc erc;
PT_BEGIN()
DISCONNECTED: {
	PT_YIELD_UNTIL(_mqtt->isConnected());
//------------------------------------------------------------------ list all topics
	for (topic = Topic::first(); topic != 0; topic = topic->next())
		INFO(" topic : %s  ", topic->getName());
	nextTopic();
	goto CONNECTED;
}
CONNECTED: {

	while (_mqtt->isConnected()) {
		timeout(5000);
		PT_YIELD_UNTIL(msg.is(0, SIG_CHANGE) || timeout());
		if (timeout()) {
			topic = _currentTopic;
		} else {
			topic = _changedTopic = (Topic*) msg.src();
		}
		_value.clear();
		if ((erc = topic->getter(_value)) == E_OK) {
			if (_mqtt->publish(topic->getName(), _value,
					(uint32_t) (topic->flags() & 0x7)) == E_OK) {
				if (topic == _changedTopic) {
					_changedTopic = 0;
				}
				nextTopic();
			} else {
				ERROR(" publish failed ");
			}
		} else {
			ERROR(" topic getter failed ");
			nextTopic();
		}
	}
	goto DISCONNECTED;
}
PT_END()
}

int Topic::getFlags() const {
return _flags;
}

Xdr Topic::getGetter() const {
return _getter;
}

void* Topic::getInstance() const {
return _instance;
}

Xdr Topic::getPutter() const {
return _putter;
}
