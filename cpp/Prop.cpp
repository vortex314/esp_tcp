/*
 * Prop.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Mqtt.h"
#include "Prop.h"
#include "Cbor.h"
// #include "Fsm.h"

Prop* Prop::_first;

const char* sType[] = { "UINT8", "UINT16", "UINT32", "UINT64", "INT8", "INT16",
		"INT32", "INT64", "BOOL", "FLOAT", "DOUBLE", "BYTES", "ARRAY", "MAP",
		"STR", "OBJECT" };

const char* sMode[] = { "READ", "WRITE", "RW" };

IROM Prop::Prop(const char* name, Flags flags) {
	_next = 0;
	init(name, flags);
}

IROM void Prop::init(const char* name, Flags flags) {
	_name = (char*) malloc(strlen(name) + 1);
	strcpy(_name, name);
	_flags = flags;
	_flags.doPublish = true;
	_lastPublished = 0;
	if (_first == 0) {
		_first = this;
	} else {
		Prop* cursor = _first;
		while (cursor->_next != 0) {
			cursor = cursor->_next;
		}
		cursor->_next = this;
	}
}
static int pow10[10] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
		100000000, 1000000000 };
IROM bool Prop::hasToBePublished() {
	if (_flags.mode == M_WRITE)
		return false;
	if (Sys::millis() > (_lastPublished + pow10[_flags.interval]))
		return true;
	return _flags.doPublish;
}

IROM void Prop::doPublish() {
	_flags.doPublish = true;
}

IROM void Prop::isPublished() {
	_flags.doPublish = false;
	_lastPublished = Sys::millis();
}

IROM void Prop::metaToBytes(Bytes& message) {
	Cbor& msg = (Cbor&) message;
	int r;
	msg.addMap(4);
	msg.add("type");
	msg.add(sType[_flags.type]);
	msg.add("mode");
	r = _flags.mode;
	msg.add(sMode[r]);
	msg.add("qos");
	msg.add(_flags.qos);
	msg.add("interval");
	r = _flags.interval;
	msg.add(pow10[r]);
	msg.addBreak();
}

IROM void Prop::nextToBytes(Bytes& message) {
	if (_next == 0)
		return;
	Cbor& msg = (Cbor&) message;
	msg.add(_next->_name);
}

IROM void Prop::firstToBytes(Bytes& message) {
	if (_first == 0)
		return;
	Cbor& msg = (Cbor&) message;
	msg.add(_first->_name);
}

#include <cstdlib>

IROM Prop* Prop::findProp(Str& name) {
	Prop* cursor = _first;
	while (cursor != 0) {
		if (name.startsWith(cursor->_name))
			return cursor;
		cursor = cursor->_next;
	}
	return 0;
}

IROM PropMgr::PropMgr(Mqtt* mqtt) :
		Handler("Props"), _prefix(SIZE_TOPIC), _getPrefix( SIZE_TOPIC), _putPrefix(
		SIZE_TOPIC), _headPrefix(
		SIZE_TOPIC), _topic(SIZE_TOPIC), _message(SIZE_MESSAGE) {
	_cursor = Prop::_first;
	_state = ST_DISCONNECTED;
	_publishMeta = false;
	_mqtt = mqtt;
	_src = NULL;
	restart();
}

IROM void PropMgr::setPrefix(const char* prefix) {

	_prefix.clear() << prefix;
	_mqtt->setPrefix(prefix);
	_getPrefix.clear() << "GET" << _prefix;
	_putPrefix.clear() << "PUT" << _prefix;
	_headPrefix.clear() << "HEAD" << _prefix;
}

IROM void PropMgr::onPublish(Str& topic, Bytes& message) {
	Str str(SIZE_TOPIC);

	if (topic.startsWith(_putPrefix))   // "PUT/<device>/<topic>
			{
		str.substr(topic, _putPrefix.length());
		Prop* p = Prop::findProp(str);
		if (p) {
			message.offset(0);
			p->fromBytes(topic, message);
			p->doPublish();
		}
	} else if (topic.startsWith(_getPrefix))     // "GET/<device>/<topic>
			{
		str.substr(topic, _getPrefix.length());
		Prop* p = Prop::findProp(str);
		if (p) {
			if (message.length() == 0) {
				p->doPublish();
			} else {
				Str t(SIZE_TOPIC);
				t << _prefix;
				t << p->_name;
				if (message.equals((uint8_t*) "META", 4)) {
					t << ".META";
					Bytes msg(SIZE_MESSAGE);
					p->metaToBytes(msg);
					_mqtt->publish(t, msg, 0);
				} else if (message.equals((uint8_t*) "NEXT", 4)) {
					Str t(SIZE_TOPIC);
					t << _prefix;
					t << p->_name;
					t << ".NEXT";
					Bytes msg(SIZE_MESSAGE);
					p->nextToBytes(msg);
					_mqtt->publish(t, msg, 0);
				} else if (message.equals((uint8_t*) "FIRST", 5)) {
					Str t(SIZE_TOPIC);
					t << _prefix;
					t << p->_name;
					t << ".FIRST";
					Bytes msg(SIZE_MESSAGE);
					p->firstToBytes(msg);
					_mqtt->publish(t, msg, 0);
				}
			}
		}

	} else if (topic.startsWith(_headPrefix))   // "HEAD/<device>/<topic>
			{

	}

}

IROM void PropMgr::nextProp() {
	_cursor = _cursor->_next;
	if (_cursor == 0)
		_cursor = Prop::_first;
}

IROM bool PropMgr::dispatch(Msg& msg) {
	Str sub(100);

	PT_BEGIN()
	DISCONNECTED: {
		PT_YIELD_UNTIL(_mqtt->isConnected());
		_cursor = Prop::_first;
		goto SUB_PUT;
	}
	SUB_PUT: {
		sub.clear() << "PUT" << _prefix << "#";
		_src = _mqtt->subscribe(sub);
		if (_src == 0)
			goto DISCONNECTED;
		PT_YIELD_UNTIL(_src->isReady() || !_mqtt->isConnected());
		if (!_mqtt->isConnected())
			goto DISCONNECTED;
		goto SUB_GET;
	}
	SUB_GET: {
		sub.clear() << "GET" << _prefix << "#";
		_src = _mqtt->subscribe(sub);
		if (_src == 0)
			goto DISCONNECTED;
		PT_YIELD_UNTIL(_src->isReady() || !_mqtt->isConnected());
		if (!_mqtt->isConnected())
			goto DISCONNECTED;
	}

	PUBLISH: {
		PT_YIELD_UNTIL(
				!_mqtt->isConnected() || _mqtt->_mqttPublisher->isReady()); // sleep between prop publishing
		if (!_mqtt->isConnected())
			goto DISCONNECTED;
		if (_cursor->hasToBePublished()) {
			_topic.clear();
			_topic << _cursor->_name;
			_message.clear();
			_cursor->toBytes(_topic, _message);
			_src = _mqtt->publish(_topic, _message,0);
			if (_src)
				goto WAIT_ACK;
			else
				goto PUBLISH;
			// busy with previous
		} else {
			nextProp();
			if (_cursor == Prop::_first) {
				timeout(10);
				PT_YIELD_UNTIL(timeout()); // slep 10 msec between scans
			}
		}
		goto PUBLISH;
	}
	WAIT_ACK: {
		timeout(TIME_WAIT_REPLY);
		PT_YIELD_UNTIL(_src->isReady() || timeout());
		if (_src->isReady()) {
			_cursor->isPublished();
			nextProp();
			goto PUBLISH;
		} else
			goto DISCONNECTED;
		goto PUBLISH;
	}
PT_END()
}

IROM void Prop::publishAll() {
Prop* cursor = Prop::_first;
while (cursor->_next != 0) {
	cursor->doPublish();
	cursor = cursor->_next;
}
}
