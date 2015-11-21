/*
 * MqttOut.h
 *
 *  Created on: 22-jun.-2013
 *      Author: lieven2
 */

#ifndef MQTTOUT_H_
#define MQTTOUT_H_
#include <string.h>
#include "Bytes.h"
//#include "Interface.h"
#include "MqttConstants.h"

class MqttOut : public Bytes {
public:
    MqttOut(int size);
private:
    IROM void add(uint8_t value);
    IROM void addHeader(int header);
    IROM void addRemainingLength(uint32_t length);
    IROM void addUint16(uint16_t value);
    IROM void addString(const char *string);
    IROM void addStr(Str& str);
    IROM void addComposedString(Str& prefix, Str& str);
    IROM void addMessage(uint8_t* src, uint32_t length);
    IROM void addBytes(Bytes& bytes);
    IROM void addBytes(uint8_t* bytes,uint32_t length);
public:
    IROM void Connect(uint8_t hdr, const char *clientId, uint8_t connectFlag, const char* willTopic,
            Bytes& willMsg, const char *username, const char* password, uint16_t keepAlive);
    IROM void ConnAck(uint8_t erc);
    IROM void Disconnect();
    IROM void PingReq();
    IROM void PingResp();
    IROM void Publish(uint8_t hdr,  Str& topic, Bytes& msg,
            uint16_t message_id);
    IROM void PubRel(uint16_t messageId);
    IROM void PubAck(uint16_t messageId);
    IROM void PubRec(uint16_t messageId);
    IROM void PubComp(uint16_t messageId);
    IROM void Subscribe(uint8_t hdr, Str& topic, uint16_t messageId,
            uint8_t requestedQos);
    IROM void prefix(Str& pr);
private:
    Str _prefix;
};


#endif /* MQTTOUT_H_ */
