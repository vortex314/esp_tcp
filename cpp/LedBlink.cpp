/*
 * LedBlink1.cpp
 *
 *  Created on: Oct 7, 2015
 *      Author: lieven
 */

#include "LedBlink.h"
extern "C" {
#include "c_types.h"
#include "gpio16.h"
}

IROM LedBlink::LedBlink(void* src) :
		Handler("LedBlink") {
	_isOn = false;
	_msecInterval = 200;
	_src = src;
}

IROM void LedBlink::init() {
	gpio16_output_conf();
	gpio16_output_set(0); // LED is ON
}

IROM LedBlink::~LedBlink() {
}

IROM bool LedBlink::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	init();
	while (true) {
		timeout(_msecInterval);
		PT_YIELD_UNTIL(
				msg.is(_src, SIG_CONNECTED) || msg.is(_src, SIG_DISCONNECTED)
						|| timeout());
		switch (msg.signal()) {
		case SIG_TICK: {
			gpio16_output_set(_isOn);
			_isOn = !_isOn;
			break;
		}
		case SIG_CONNECTED: {
			_msecInterval = 1000;
			break;
		}
		case SIG_DISCONNECTED: {
			_msecInterval = 200;
			break;
		}
		default: {
		}
		}

	}
	PT_END();
	return false;
}

