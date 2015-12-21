/*
 * Stm32.cpp
 *
 *  Created on: Nov 12, 2015
 *      Author: lieven
 */

#include <Stm32.h>
#include <Mqtt.h>

// <cmd><messageId><expectedLength><expected_acks><blocksPerAck><block1>...<blokckn>
// <cmd><messageId><erc><bytes>
// GET-1234-10-1-0x11 0xEE -> GET-1234-erc

#include <Stm32.h>
#include <Stm32Cmd.h>

Stm32::Stm32(Mqtt* mqtt, UartEsp8266* uart, Gpio* pinReset, Gpio* pinBoot) :
		Handler("Stm32"), _request(256), _response(256), _uartIn(256), _dataIn(
				300), _queue(1000) {
	_mqtt = mqtt;
	_uart = uart;
	_pinBoot0 = pinBoot;
	_pinReset = pinReset;
	_cmd = Stm32Cmd::INVALID;
	_messageId = 0;
	_pinReset = pinReset;
	_pinBoot0 = pinBoot;
	_errno = 0;
	_topic = new Topic("stm32/cmd", this, stm32CmdIn, 0,
			Topic::F_QOS2 + Topic::F_NO_POLL);
	_retries = 0;
}

void Stm32::init() {
	_pinReset->setMode("OPP");
	_pinReset->digitalWrite(1);
	/*	_pinBoot0->setMode("OPP");
	 _pinBoot0->digitalWrite(1);*/
}

void Stm32::status(const char* s) {
	if (!_mqtt->isConnected())
		return;
	Cbor str(30);
	str.add(s);
	_mqtt->publish("stm32/status", str, Topic::F_QOS0);
}

char _logBuffer[100];
Cbor cbor(100);
extern "C" {
#include <util.h>
}
;

void Stm32::log(const char* format, ...) {
	char *p;
	_logBuffer[0] = 0;
	va_list args;
	va_start(args, format);
	ets_vsnprintf(_logBuffer, sizeof(_logBuffer), format, args);
	va_end(args);
	cbor.clear();
	cbor.add(_logBuffer);
	_mqtt->publish("stm32/log", cbor, Topic::F_QOS0);
}
/*
 void Stm32::log(const char* s) {
 if ( !_mqtt->isConnected()) return;
 Cbor str(30);
 str.add(s);
 _mqtt->publish("stm32/log",str,Topic::F_QOS0);
 }
 */
void Stm32::progress(uint32_t perc) {
	if (!_mqtt->isConnected())
		return;
	Cbor str(30);
	str.add(perc);
	_mqtt->publish("stm32/progress", str, Topic::F_QOS0);
}

bool Stm32::waitUartData(uint8_t* pb, uint32_t length) {
	while (_uart->hasData()) {
		uint8_t b = _uart->read();
		_uartIn.write(b);
		uint32_t i;
		for (i = 0; i < length; i++) {
			if (pb[i] == b)
				return true;
		}
	}
	return false;
}

bool Stm32::dispatch(Msg& msg) {
	int erc;
	bool ackReceived;
	PT_BEGIN()
	INIT: {
		PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
		init();
	}
	START: {
		_response.clear();
		_uartIn.clear();
		status("Ready");
		progress(0);
		timeout(5000);
		PT_YIELD_UNTIL(_queue.hasData() || timeout());
		if (timeout())
			goto START;
		_queue.get(_request);
		_request.scanf("iiiB", &_cmd, &_messageId, &_errno, &_dataIn);
//		INFO(" received command");
		if (_cmd == Stm32Cmd::STM32_RESET)
			goto STATE_RESET;
		if (_cmd == Stm32Cmd::STM32_GET_VERSION_AND_COMMANDS)
			goto STATE_REQUEST;
		goto START;
	}
	STATE_RESET: {
		// extract data to work with
		erc = E_OK;
		status("Resetting");
		progress(1);
		_uart->pins(1);
		_uart->setMode("8E1");
		_uart->setBaudrate(115200);
		_pinReset->digitalWrite(0);
		timeout(10);
		PT_YIELD_UNTIL(timeout());
		_pinReset->digitalWrite(1);
		timeout(10);
		PT_YIELD_UNTIL(timeout());
		uartClear();
		progress(10);
		_retries = 0;
		ackReceived=false;
		while (_retries++ < 100) {
			_uart->write(STM32_SYNC);
			timeout(100);
			PT_YIELD_UNTIL(waitUartData((uint8_t* )"\x79", 1) || timeout());
			if ( timeout() ) {

			} else {
				ackReceived=true;
				break;
			}
		}
		progress(100);
		if (_retries >= 100) {
			_errno = EHOSTUNREACH;
			log("no device found : %s", strerror(errno));
		} else if (ackReceived) {
			log(" device reset after %d SYNC", _retries);
			_errno = E_OK;
		}
		status(strerror(_errno));
		_response.addf("iii", _cmd, _messageId, _errno);
		_response.addNull();
		_mqtt->publish("stm32/cmd", _response, MQTT_QOS2_FLAG);
//		_uart->pins(0);
		goto START;
	}
	STATE_GET: {
		status("Executing Request");
		_retries = 0;
		_errno = 0;
		_uartIn.clear();
		progress(1);
//		_uart->pins(1);
		_uart->write(_dataIn);
		timeout(10);
		PT_YIELD_UNTIL(waitUartData((uint8_t* )"\x79\x1F", 2) || timeout());
		progress(100);
		status(strerror(_errno));
		_response.addf("iiiB", _cmd, _messageId, _errno, &_uartIn);
		_mqtt->publish("stm32/cmd", _response, MQTT_QOS2_FLAG);
//		_uart->pins(0);
		goto START;
	}
	STATE_REQUEST: {
		status("Executing Request");
		_retries = 0;
		_errno = 0;
		_uartIn.clear();
		progress(1);
//		_uart->pins(1);
		_uart->write(_dataIn);
		timeout(10);
		PT_YIELD_UNTIL(waitUartData((uint8_t* )"\x79\x1F", 2) || timeout());
		progress(100);
		status(strerror(_errno));
		_response.addf("iiiB", _cmd, _messageId, _errno, &_uartIn);
		_mqtt->publish("stm32/cmd", _response, MQTT_QOS2_FLAG);
//		_uart->pins(0);
		goto START;
	}
PT_END()
;
}

Erc Stm32::stm32CmdIn(void* instance, Cbor& cbor) {
Stm32* stm32 = (Stm32*) instance;
// INFO("put message for STM32");
return stm32->_queue.put(cbor);
}

Stm32::~Stm32() {

}

void Stm32::addUartData() {
while (_uart->hasData()) {
	_uartIn.write(_uart->read());
}
}

void Stm32::uartClear() {
while (_uart->hasData())	// ---------- clear uart
	_uart->read();
}

bool Stm32::uartDataComplete() {

return false;
}

//____________________________________________________________________________

