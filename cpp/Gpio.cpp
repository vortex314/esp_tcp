/*
 * Gpio.cpp
 *
 *  Created on: Nov 17, 2015
 *      Author: lieven
 */

#include <Gpio.h>
#include "Arduino.h"

/*
 * 		MODE_DISABLED = 0,
 MODE_OUTPUT_OPEN_DRAIN,
 MODE_OUTPUT_PUSH_PULL,
 MODE_INPUT,
 MODE_INPUT_PULLUP
 */

const char* Gpio::_sMode[] = { "DIS", "OOD", "OPP", "INP", "IPU" };
const int arduinoModes[] = { INPUT, OUTPUT_OPEN_DRAIN, OUTPUT, INPUT,
INPUT_PULLUP };

IROM Gpio::Gpio(uint8_t pin) {
	_pin = pin;
	_mode = MODE_DISABLED;
}

IROM Gpio::~Gpio() {

}

IROM Erc Gpio::setMode(char* str) {
	uint32_t i;
	for (i = 0; i < sizeof(_sMode); i++)
		if (strcmp(_sMode[i], str) == 0) {
			_mode = (Gpio::Mode) i;
			pinMode(_pin, arduinoModes[i]);
		}
	return E_OK;
}

IROM Erc Gpio::getMode(char* str) {
	strcpy( str, Gpio::_sMode[_mode]);
	return E_OK;
}

IROM Erc Gpio::digitalWrite(uint8_t i) {
	::digitalWrite(_pin, i);
	return E_OK;
}

IROM Erc Gpio::digitalRead(uint8_t* i) {
	*i = ::digitalRead(_pin);
	return E_OK;
}

