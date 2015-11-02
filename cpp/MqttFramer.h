/*
 * MqttFramer.h
 *
 *  Created on: Oct 31, 2015
 *      Author: lieven
 */

#ifndef MQTTFRAMER_H_
#define MQTTFRAMER_H_

#include <Handler.h>
#include "MqttMsg.h"
#include "Stream.h"
#include "Tcp.h"

class MqttFramer: public Handler {
private :
	Tcp* _stream;
	MqttMsg* _msg;
public:
	MqttFramer(Tcp* stream);
	virtual ~MqttFramer();
	bool isConnected();
	void connect();
	void disconnect();
	void send(MqttMsg& msg);
	bool dispatch(Msg& msg);
};

#endif /* MQTTFRAMER_H_ */
