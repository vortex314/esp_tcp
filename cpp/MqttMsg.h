/*
 * MqttMsg.h
 *
 *  Created on: Oct 25, 2015
 *      Author: lieven
 */

#ifndef MQTTMSG_H_
#define MQTTMSG_H_

#include <Handler.h>
#include "Tcp.h"

class MqttMsg: public Handler {
private :
	Tcp& _tcp;
public:
	MqttMsg(Tcp& tcp);
	virtual ~MqttMsg();
	bool dispatch(Msg& msg);

};

#endif /* MQTTMSG_H_ */
