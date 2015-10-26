/*
 * MqttMsg.cpp
 *
 *  Created on: Oct 25, 2015
 *      Author: lieven
 */

#include "MqttMsg.h"

MqttMsg::MqttMsg(Tcp& tcp) :
		_tcp(tcp) {

}

MqttMsg::~MqttMsg() {
	// TODO Auto-generated destructor stub
}

bool IROM MqttMsg::dispatch(Msg& msg) {
	PT_BEGIN();
	INIT: {
		PT_YIELD_UNTIL(msg.is(0, SIG_INIT));
		INFO(" SIG_INIT");
		goto DISCONNECTED;
	};
	DISCONNECTED: {
		while (true) {
			PT_YIELD_UNTIL(msg.is((void*)TCP_ID, SIG_CONNECTED));
			goto CONNECTED;
		}
	};
	CONNECTED: {
		while (true) {
			INFO(" MQTT:send ");
			_tcp.write((uint8_t*)"BYE\n\r\n",6);
			timeout(2000);
			PT_YIELD_UNTIL(msg.is((void*)TCP_ID, SIG_DISCONNECTED) || timeout());
			if ( !timeout())goto DISCONNECTED;
		}
	};
	PT_END();
	return true;
}
