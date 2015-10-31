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
    void add(uint8_t value);
    void addHeader(int header);
    void addRemainingLength(uint32_t length);
    void addUint16(uint16_t value);
    void addString(const char *string);
    void addStr(Str& str);
    void addComposedString(Str& prefix, Str& str);
    void addMessage(uint8_t* src, uint32_t length);
    void addBytes(Bytes& bytes);
    void addBytes(uint8_t* bytes,uint32_t length);
public:
    void Connect(uint8_t hdr, const char *clientId, uint8_t connectFlag, const char* willTopic,
            Bytes& willMsg, const char *username, const char* password, uint16_t keepAlive);
    void ConnAck(uint8_t erc);
    void Disconnect();
    void PingReq();
    void PingResp();
    void Publish(uint8_t hdr,  Str& topic, Bytes& msg,
            uint16_t message_id);
    void PubRel(uint16_t messageId);
    void PubAck(uint16_t messageId);
    void PubRec(uint16_t messageId);
    void PubComp(uint16_t messageId);
    void Subscribe(uint8_t hdr, Str& topic, uint16_t messageId,
            uint8_t requestedQos);
    void prefix(Str& pr);
private:
    Str _prefix;
};


#endif /* MQTTOUT_H_ */
