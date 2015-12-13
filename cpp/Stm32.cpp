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
/*
 class Stm32Topic {
 typedef enum {
 CMD_GET = 0, CMD_WRITE = 0x61
 } Cmd;
 static Erc cmdIn(void* pv, Cbor& cbor) {
 Stm32* stm32 = (Stm32*) pv;
 Msg::publish(stm32, SIG_RXD);
 return E_OK;
 }
 static Erc cmdOut(void* pv, Cbor& cbor) {
 Stm32* stm32 = (Stm32*) pv;
 Msg::publish(stm32, SIG_RXD);
 return E_OK;
 }
 static void create(Stm32* stm32) {
 new Topic("stm32/cmd", stm32, cmdIn, cmdOut, Topic::F_QOS2);
 //		new Topic("stm32/mode", modeIn, modeOut, F_QOS2);
 }
 };

 */
#include <Stm32.h>
#include <Stm32Cmd.h>

Stm32::Stm32(Mqtt* mqtt, UartEsp8266* uart, Gpio* pinReset, Gpio* pinBoot) :
		Handler("Stm32"), _request(256), _response(256), _uartIn(256), _queue(
				1000) {
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


void Stm32::status(const char* s) {
	if ( !_mqtt->isConnected()) return;
	Cbor str(30);
	str.add(s);
	_mqtt->publish("stm32/status",str,Topic::F_QOS0);
}

void Stm32::log(const char* s) {
	if ( !_mqtt->isConnected()) return;
	Cbor str(30);
	str.add(s);
	_mqtt->publish("stm32/log",str,Topic::F_QOS0);
}

bool Stm32::dispatch(Msg& msg) {
	int erc;
	bool ackReceived;
	PT_BEGIN()
	INIT: {
		PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	}
	START: {
		_response.clear();
		status("Ready");
		timeout(5000);
		PT_YIELD_UNTIL(_queue.hasData() || timeout());
		if (timeout())
			goto START;
		_queue.get(_request);
		_request.scanf("ii", &_cmd, &_messageId);
		INFO(" received command");
		if (_cmd == Stm32Cmd::RESET)
			goto STATE_RESET;
		if (_cmd == Stm32Cmd::GET)
			goto STATE_GET;
		goto START;
	}
	STATE_RESET: {
		// extract data to work with
		erc = E_OK;
		status("Resetting");
		_uart->setMode("8E1");
		_uart->setBaudrate(115200);
		_pinReset->digitalWrite(0);
		timeout(10);
		PT_YIELD_UNTIL(timeout());
		_pinReset->digitalWrite(1);
		timeout(10);
		PT_YIELD_UNTIL(timeout());
		uartClear();
		_retries = 0;
		while (_retries++ < 100) {
			_uart->write(STM32_SYNC);
			timeout(10);
			PT_YIELD_UNTIL(
					ackReceived = (_uart->hasData()
							&& (STM32_ACK == _uart->read())) // --------- data In
					|| (timeout()));
			if (ackReceived)
				break;
		}
		if (_retries == 100)
			_errno = EHOSTUNREACH;
		else if (ackReceived)
			_errno = E_OK;
		else
			_errno = EINVAL;
		status(strerror(_errno));
		_response.cmd(_cmd).erc(_errno);
		_mqtt->publish("stm32/cmd", _response, MQTT_QOS2_FLAG);
		goto START;
	}
	STATE_GET: {
		goto START;
	}
PT_END()
;
}

Erc Stm32::stm32CmdIn(void* instance, Cbor& cbor) {
Stm32* stm32 = (Stm32*) instance;
INFO("put message for STM32");
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

