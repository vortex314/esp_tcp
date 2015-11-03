/*
 * Props.cpp
 *
 *  Created on: Nov 2, 2015
 *      Author: lieven
 */

#include "Props.h"

Props::Props(Mqtt* mqtt) : Handler("Props") {
	_mqtt = mqtt;

}

Props::~Props() {
	// TODO Auto-generated destructor stub
}
Str topic("system/uptime");
Str value(20);
Handler *src;
IROM bool Props::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	WAIT_CONNECT: {
		PT_YIELD_UNTIL(_mqtt->isConnected());
		goto CONNECTED;
	}
	CONNECTED: {
		while (_mqtt->isConnected()) {
			timeout(1000);
			PT_YIELD_UNTIL(timeout());
			value.clear() << Sys::millis();
			src = _mqtt->publish(topic, value,
					(Flags) {T_INT32,M_READ,T_100SEC,QOS_2,NO_RETAIN,true});
			PT_YIELD_UNTIL(!src->isRunning());
		}
		goto WAIT_CONNECT;

	}

	PT_END();
	return false;
}
