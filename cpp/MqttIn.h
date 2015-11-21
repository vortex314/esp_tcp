/*
 * MqttIn.h
 *
 *  Created on: 22-jun.-2013
 *      Author: lieven2
 */

#ifndef MQTTIN_H_
#define MQTTIN_H_
#include "MqttConstants.h"
//#include "Event.h"
#include "Str.h"
#include "Cbor.h"
#include "Packer.h"
#include "Json.h"
#define TOPIC_LEN  100
#define MSG_LEN    256

class MqttIn
{
public:
    Bytes* _bytes;
    uint8_t _header;
    uint32_t _remainingLength;
    uint32_t _lengthToRead;
    uint32_t _offsetVarHeader;
    uint8_t _returnCode;
    uint16_t _messageId;
    bool _isBytesOwner;
    Str _topic;
    Bytes _message;

    enum RecvState
    {
        ST_HEADER, ST_LENGTH, ST_PAYLOAD, ST_COMPLETE
    } _recvState;
public:
    IROM MqttIn();
    IROM MqttIn(Bytes* bytes);
    IROM MqttIn(int size);
    IROM void remap(Bytes* bytes);
//   MqttIn(MqttIn& src);
    IROM virtual ~MqttIn();
    IROM void clone(MqttIn& m)
    {
//        Bytes::clone(m);
        parse();
    }
    IROM void Feed(uint8_t b);

    IROM uint16_t messageId(); // if < 0 then not found
    IROM uint8_t type();
    IROM uint8_t qos();
    IROM bool complete();
    IROM void complete(bool st);
    IROM void reset();
    IROM void add(uint8_t data);
    IROM bool addRemainingLength(uint8_t data);
    IROM bool parse();
    IROM void readUint16(uint16_t * pi);
    IROM void readUtf(Str* str);
    IROM void readBytes(Bytes* b, int length);
    IROM Str* topic();
    IROM Bytes* message();
    IROM void toString(Str& str);
    IROM Bytes* getBytes()
    {
        return _bytes;
    }
};

#endif /* MQTTIN_H_ */
