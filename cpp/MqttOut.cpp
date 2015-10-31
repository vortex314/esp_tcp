/*
 * MqttOut.cpp
 *
 *  Created on: 22-jun.-2013
 *      Author: lieven2
 */

#include "MqttOut.h"
#include <string.h>

// #define LOG(x) std::cout << Sys::upTime() << " | MQTT OUT " << x << std::endl
#define LOG(x)
IROM MqttOut::MqttOut(int size) : Bytes(size),_prefix(30)
  {
}

void IROM MqttOut::prefix(Str& prefix)  {
    _prefix.clear();
    _prefix << prefix;
}

void IROM MqttOut::add(uint8_t value) {
    write(value);
}

void IROM MqttOut::addHeader(int hdr) {
    clear();
    write(hdr);
}

void IROM MqttOut::addRemainingLength(uint32_t length) {
    do {
        uint8_t digit = length & 0x7F;
        length /= 128;
        if (length > 0) {
            digit |= 0x80;
        }
        write(digit);
    } while (length > 0);
}

void IROM MqttOut::addUint16(uint16_t value) {
    write(value >> 8);
    write(value & 0xFF);
}

void IROM MqttOut::addString(const char *str) {
    addUint16(strlen(str));
    addBytes((uint8_t*) str, strlen(str));
}

void IROM MqttOut::addStr(Str& str) {
    addUint16(str.length());
    addBytes(str);
}

void IROM MqttOut::addComposedString(Str& prefix, Str &str) {
    addUint16(prefix.length() + str.length());
    addBytes( prefix);
    addBytes(str );
}

void IROM MqttOut::addMessage(uint8_t* src, uint32_t length) {
    addUint16(length);
    addBytes(src, length);
}

void IROM MqttOut::addBytes(Bytes& bytes) {
    bytes.offset(0);
    for (uint32_t i = 0; i < bytes.length(); i++)
        write(bytes.read());
}

void IROM MqttOut::addBytes(uint8_t* bytes, uint32_t length) {
    for (uint32_t i = 0; i < length; i++)
        write(*bytes++);
}

void IROM MqttOut::Connect(uint8_t hdr, const char *clientId, uint8_t connectFlag,
        const char *willTopic, Bytes& willMsg, const char *username, const char* password,
        uint16_t keepAlive) {
            LOG("CONNECT");
    uint8_t connectFlags = connectFlag;

    uint16_t clientidlen = strlen(clientId);
    uint16_t usernamelen = strlen(username);
    uint16_t passwordlen = strlen(password);
    uint16_t payload_len = clientidlen + 2;
    uint16_t willTopicLen = strlen(willTopic) + _prefix.length();
    uint16_t willMsgLen = willMsg.length();

    // Preparing the connectFlags
    if (usernamelen) {
        payload_len += usernamelen + 2;
        connectFlags |= MQTT_USERNAME_FLAG;
    }
    if (passwordlen) {
        payload_len += passwordlen + 2;
        connectFlags |= MQTT_PASSWORD_FLAG;
    }
    if (willTopicLen) {
        payload_len += willTopicLen + 2;
        payload_len += willMsgLen + 2;
        connectFlags |= MQTT_WILL_FLAG;
    }

    // Variable header
    uint8_t var_header[] = {0x00, 0x06, 0x4d, 0x51, 0x49, 0x73, 0x64, 0x70, // Protocol name: MQIsdp
        0x03, // Protocol version
        connectFlags, // Connect connectFlags
        (uint8_t) (keepAlive >> 8), (uint8_t) (keepAlive & 0xFF), // Keep alive
    };

    // Fixed header
    uint8_t fixedHeaderSize = 2; // Default size = one byte Message Type + one byte Remaining Length
    uint8_t remainLen = sizeof (var_header) + payload_len;
    if (remainLen > 127) {
        fixedHeaderSize++; // add an additional byte for Remaining Length
    }

    // Message Type
    addHeader(MQTT_MSG_CONNECT | hdr);
    addRemainingLength(remainLen);
    addBytes(var_header, sizeof (var_header));
    // Client ID - UTF encoded
    addString(clientId);
    if (willTopicLen) {
        Str wt(willTopic);
        addComposedString(_prefix,wt);
        addUint16(willMsg.length());
        addBytes(willMsg);
    }
    if (usernamelen) { // Username - UTF encoded
        addString(username);
    }
    if (passwordlen) { // Password - UTF encoded
        addString(password);
    }
}

void IROM MqttOut::Publish(uint8_t hdr,  Str& topic, Bytes& msg,
        uint16_t messageId) {
            LOG("PUBLISH");
    addHeader(MQTT_MSG_PUBLISH + hdr);
    bool addMessageId = (hdr & MQTT_QOS_MASK) ? true : false;
    int remLen = topic.length() + _prefix.length() + 2 + msg.length();
    if (addMessageId)
        remLen += 2;
    addRemainingLength(remLen);
    addComposedString(_prefix,topic);
    /*    addUint16(strlen(_prefix) + strlen(topic));
        addBytes((uint8_t*) _prefix, strlen(_prefix));
        addBytes((uint8_t*) topic, strlen(topic));*/
    if (addMessageId)
        addUint16(messageId);
    addBytes(msg);
}

void IROM MqttOut::ConnAck(uint8_t erc) {
    LOG("CONNACK");
    addHeader(MQTT_MSG_CONNACK);
    addRemainingLength(2);
    add(0);
    add(erc);
}

void IROM MqttOut::Disconnect() {
    LOG("DISCONNECT");
    addHeader(MQTT_MSG_DISCONNECT);
    addRemainingLength(0);
}

void IROM MqttOut::PubRel(uint16_t messageId) {
    LOG("PUBREL");
    addHeader(MQTT_MSG_PUBREL | MQTT_QOS1_FLAG);
    addRemainingLength(2);
    addUint16(messageId);
}

void IROM MqttOut::PubAck(uint16_t messageId) {
    LOG("PUBACK");
    addHeader(MQTT_MSG_PUBACK | MQTT_QOS1_FLAG);
    addRemainingLength(2);
    addUint16(messageId);
}

void IROM MqttOut::PubRec(uint16_t messageId) {
    LOG("PUBREC");
    addHeader(MQTT_MSG_PUBREC | MQTT_QOS1_FLAG);
    addRemainingLength(2);
    addUint16(messageId);
}

void IROM MqttOut::PubComp(uint16_t messageId) {
    LOG("PUBCOMP");
    addHeader(MQTT_MSG_PUBCOMP | MQTT_QOS1_FLAG);
    addRemainingLength(2);
    addUint16(messageId);
}

void IROM MqttOut::Subscribe(uint8_t hdr, Str& topic, uint16_t messageId,
        uint8_t requestedQos) {
            LOG("SUBSCRIBE");
    addHeader(hdr | MQTT_MSG_SUBSCRIBE);
    addRemainingLength(topic.length()  + 2 + 2 + 1);
    addUint16(messageId);
    addStr(topic);
    add(requestedQos);
}

void IROM MqttOut::PingReq() {
    LOG("PINGREQ");
    addHeader(MQTT_MSG_PINGREQ); // Message Type, DUP flag, QoS level, Retain
    addRemainingLength(0); // Remaining length
}

void IROM MqttOut::PingResp() {
    LOG("PINGRESP");
    addHeader(MQTT_MSG_PINGRESP); // Message Type, DUP flag, QoS level, Retain
    addRemainingLength(0); // Remaining length
}


