/*
 * MqttMsg.h
 *
 *  Created on: Oct 25, 2015
 *      Author: lieven
 */

#ifndef MQTTMSG_H_
#define MQTTMSG_H_
#include "MqttConstants.h"

enum MqttField {
	KEY_HEADER, KEY_TOPIC, KEY_MESSAGE, KEY_CLIENT, KEY_USER, KEY_PASSWORD,
};

class MqttMsg: public Bytes {
private:
	Str _prefix;
	Str _message;
	Str _topic;
	Str _user;
	Str _password;
	Str _clientId;
	uint8_t _header;
	uint32_t _remainingLength;
	uint32_t _lengthToRead;
	uint32_t _offsetVarHeader;
	uint8_t _returnCode;
	uint16_t _messageId;

	enum RecvState {
		ST_HEADER, ST_LENGTH, ST_PAYLOAD, ST_COMPLETE
	} _recvState;
private:
	void IROM addHeader(int header);
	void IROM addRemainingLength(uint32_t length);
	void IROM addUint16(uint16_t value);
	void IROM addString(const char *string);
	void IROM addStr(Str& str);
	void IROM  addComposedString(Str& prefix, Str& str);
	void IROM addMessage(uint8_t* src, uint32_t length);
	void IROM addBytes(Bytes& bytes);
	void IROM addBytes(uint8_t* bytes, uint32_t length);
public:
	IROM MqttMsg(uint32_t size);
	void IROM Connect(uint8_t hdr, const char *clientId, uint8_t connectFlag,
			const char* willTopic, Bytes& willMsg, const char *username,
			const char* password, uint16_t keepAlive);
	void IROM ConnAck(uint8_t erc);
	void IROM Disconnect();
	void IROM PingReq();
	void IROM PingResp();
	void IROM Publish(uint8_t hdr, Str& topic, Bytes& msg, uint16_t message_id);
	void IROM PubRel(uint16_t messageId);
	void IROM PubAck(uint16_t messageId);
	void IROM PubRec(uint16_t messageId);
	void IROM PubComp(uint16_t messageId);
	void IROM Subscribe(Str& topic, uint16_t messageId, uint8_t requestedQos);
	void IROM prefix(Str& pr);
public:

	bool IROM feed(uint8_t b);

	uint16_t IROM messageId(); // if < 0 then not found
	uint8_t IROM type();
	uint8_t IROM qos();
	bool IROM complete();
	void IROM complete(bool st);
	void IROM reset();
//	void add(uint8_t data);
	bool IROM calcLength(uint8_t data);
	bool IROM parse();
	void IROM readUint16(uint16_t * pi);
	void IROM mapUtfStr(Str& str);
	void IROM readBytes(Bytes* b, int length);
	Str* IROM topic();
	Bytes* IROM message();
	const char * IROM toString(Str& str);

};

#endif /* MQTTMSG_H_ */
