/*
 * MqttIn.cpp
 *
 *  Created on: 22-jun.-2013
 *      Author: lieven2
 */

#include "MqttIn.h"
#include <cstring>

#define LOG(x) std::cout << Sys::upTime() << " | MQTT  IN " << x << std::endl
const char* const MqttNames[] = { "UNKNOWN", "CONNECT", "CONNACK", "PUBLISH",
		"PUBACK", "PUBREC", "PUBREL", "PUBCOMP", "SUBSCRIBE", "SUBACK",
		"UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP", "DISCONNECT" };
const char* const QosNames[] = { "QOS0", "QOS1", "QOS2" };
IROM MqttIn::MqttIn(Bytes* bytes) :
		_topic(0), _message(0)   //+++ len=0
{
	_bytes = bytes;
	_remainingLength = 0;
	_header = 0;
	_recvState = ST_HEADER;
	_isBytesOwner = false;
	_returnCode = 0;
	_lengthToRead = 0;
	_offsetVarHeader = 0;
	_messageId = 1;
}

IROM MqttIn::MqttIn(int size) :
		_topic(0), _message(0)   //+++ len=0
{
	if (size)
		_bytes = new Bytes(size);
	_remainingLength = 0;
	_header = 0;
	_recvState = ST_HEADER;
	_isBytesOwner = true;
	_returnCode = 0;
	_lengthToRead = 0;
	_offsetVarHeader = 0;
	_messageId = 1;

}

IROM MqttIn::MqttIn() {
	_bytes = 0;
	_remainingLength = 0;
	_header = 0;
	_recvState = ST_HEADER;
	_isBytesOwner = true;
	_returnCode = 0;
	_lengthToRead = 0;
	_offsetVarHeader = 0;
	_messageId = 1;
}

IROM MqttIn::~MqttIn() {
	if (_isBytesOwner)
		delete _bytes;
}

void IROM MqttIn::remap(Bytes* bytes) {
	_bytes = bytes;
	_recvState = ST_HEADER;
	_isBytesOwner = false;
}

uint8_t IROM MqttIn::type() {
	return _header & MQTT_TYPE_MASK;
}

uint8_t IROM MqttIn::qos() {
	return _header & MQTT_QOS_MASK;
}

uint16_t IROM MqttIn::messageId() {
	return _messageId;
}

Str* IROM MqttIn::topic() {
	return &_topic;
}

Bytes* IROM MqttIn::message() {
	return &_message;
}

void IROM MqttIn::reset() {
	_recvState = ST_HEADER;
	_bytes->clear();

}

void IROM MqttIn::Feed(uint8_t data) {
	_bytes->write(data);
	if (_recvState == ST_HEADER) {
		_header = data;
		_recvState = ST_LENGTH;
		_remainingLength = 0;
	} else if (_recvState == ST_LENGTH) {
		if (addRemainingLength(data) == false)   // last byte read for length
				{
			_recvState = ST_PAYLOAD;
			_lengthToRead = _remainingLength;
			if (_remainingLength == 0)
				_recvState = ST_COMPLETE;
		}
	} else if (_recvState == ST_PAYLOAD) {
		_lengthToRead--;
		if (_lengthToRead == 0) {
			_recvState = ST_COMPLETE;
		}
	} else if (_recvState == ST_COMPLETE)
		Sys::warn(EINVAL, "");
}

bool IROM MqttIn::complete() {
	return (_recvState == ST_COMPLETE);
}

void IROM MqttIn::complete(bool b) {
	if (b)
		_recvState = ST_COMPLETE;
}

void IROM MqttIn::readUint16(uint16_t* pi) {
	*pi = _bytes->read() << 8;
	*pi += _bytes->read();
}

void IROM MqttIn::readUtf(Str* str) {
	uint16_t length;
	int i;
	str->clear();
	readUint16(&length);
	for (i = 0; i < length; i++) {
		str->write(_bytes->read());
	}
}

void IROM MqttIn::readBytes(Bytes* b, int length) {
	int i;
	b->clear();
	for (i = 0; i < length; i++) {
		b->write(_bytes->read());
	}
}

bool IROM MqttIn::addRemainingLength(uint8_t data) {
	_remainingLength <<= 7;
	_remainingLength += (data & 0x7F);
	return (data & 0x80);
}

void IROM MqttIn::toString(Str& str) {
	parse();
	str.append(MqttNames[type() >> 4]);
	str.append(":");
	str.append(QosNames[(_header & MQTT_QOS_MASK) >> 1]);
	if (_header & 0x1)
		str.append(",RETAIN");
	if (_header & MQTT_DUP_FLAG)
		str.append(",DUP");
	str << ", messageId : ";
	str << _messageId;

	if (type() == MQTT_MSG_PUBLISH) {
		str << (const char*) ", topic : ";
		str << _topic;
		str << ", message : ";
		str << (Str&) _message;
	} else if (type() == MQTT_MSG_SUBSCRIBE) {
		str << ", topic : ";
		str << _topic;
	} else if (type() == MQTT_MSG_CONNECT) {
		str << ", willTopic : " << _topic;
		str << ", willMessage : ";
		str << (Str&) _message;
	}
	str.append(" }");
}

bool IROM MqttIn::parse() {
	if (_bytes->length() < 2) {
		Sys::warn(EINVAL, "MQTT_LEN");
		return false;
	}
	_bytes->offset(0);
	_header = _bytes->read();
	_remainingLength = 0;
	_messageId = 0;
	while (addRemainingLength(_bytes->read()))
		;
	switch (_header & 0xF0) {
	case MQTT_MSG_CONNECT: {
		Str protocol(8);
		readUtf(&protocol);
		_bytes->read(); // version
		uint8_t connectFlags = _bytes->read();
		uint16_t keepAliveTimer;
		readUint16(&keepAliveTimer);
		Str clientId(23);
		readUtf(&clientId);
		if (connectFlags & MQTT_WILL_FLAG) {
			uint16_t l;
			readUint16(&l);
			_topic.map(_bytes->data() + _bytes->offset(), l);
			_bytes->move(l);                    // skip topic data
			readUint16(&l);
			_message.map(_bytes->data() + _bytes->offset(),
					_bytes->length() - _bytes->offset());
			_bytes->move(l);            // skip message data
		}
		Str userName(100);
		Str password(100);
		if (connectFlags & MQTT_USERNAME_FLAG)
			readUtf(&userName);
		if (connectFlags & MQTT_PASSWORD_FLAG)
			readUtf(&password);

		break;
	}
	case MQTT_MSG_CONNACK: {
		_bytes->read();
		_returnCode = _bytes->read();
		break;
	}
	case MQTT_MSG_PUBLISH: {
		uint16_t length;
		readUint16(&length);
		_topic.map(_bytes->data() + _bytes->offset(), length); // map topic Str to this part of payload
		_bytes->move(length);							// skip topic length
		int rest = _remainingLength - length - 2;
		if (_header & MQTT_QOS_MASK)   // if QOS > 0 load messageID from payload
		{
			readUint16(&_messageId);
			rest -= 2;
		} else {
			_messageId = 0;
		}
		_message.map(_bytes->data() + _bytes->offset(), rest); // map message to rest of payload
		break;
	}
	case MQTT_MSG_SUBSCRIBE: {
		readUint16(&_messageId);
		uint16_t length;
		readUint16(&length);
		_topic.map(_bytes->data() + _bytes->offset(), length);
		break;
	}
	case MQTT_MSG_SUBACK: {
		readUint16(&_messageId);
		break;
	}
	case MQTT_MSG_PUBACK: {
		readUint16(&_messageId);
		break;
	}
	case MQTT_MSG_PUBREC: {
		readUint16(&_messageId);
		break;
	}
	case MQTT_MSG_PUBREL: {
		readUint16(&_messageId);
		break;
	}
	case MQTT_MSG_PUBCOMP: {
		readUint16(&_messageId);
		break;
	}
	case MQTT_MSG_PINGREQ:
	case MQTT_MSG_PINGRESP: {
		break;
	}
	default: {
		Sys::warn(EINVAL, "MQTTIN_TYPE");
		break; // ignore bad package
	}
	}
	return true;
}

// put Active Objects global
// check malloc used after init ?
// stack or global ?
// test MqttPub
// all Stellaris dependent in one file
// publish CPU,FLASH,RAM
// Usb recv can be Bytes instead of MqttIn / drop parse
// try Usb::disconnect() => UsbCDCTerm()
//

