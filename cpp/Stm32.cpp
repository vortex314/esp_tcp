/*
 * Stm32.cpp
 *
 *  Created on: Nov 12, 2015
 *      Author: lieven
 */

#include <Stm32.h>

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
#include <BipBuffer.h>
BipBuffer cmds;

Erc Stm32::stm32CmdIn(void* instance, Cbor& cbor) {
	Stm32* stm32 = (Stm32*) instance;
	uint32_t cmd;
	uint32_t messageId;
	cbor.scanf("ii", &cmd, &messageId);
	if (stm32->_cmd == CMD_IDLE) {
		stm32->_cmd = cmd;
		if (stm32->_cmd != CMD_IDLE)
			stm32->restart();
	}
	return E_OK;
}

Erc Stm32::stm32CmdOut(void* instance, Cbor& cbor) {
	Stm32* stm32 = (Stm32*) instance;
	cbor.addf("ii", stm32->_messageId, stm32->_errno);
	return E_OK;
}

Stm32::Stm32(UartEsp8266* uart, Gpio* pinReset, Gpio* pinBoot) :
		Handler("Stm32"), _in(256), _out(256), _uartIn(256) {
	_uart = uart;
	_pinBoot0 = pinBoot;
	_pinReset = pinReset;
	_cmd = 0;
	_messageId = 0;
	_pinReset = pinReset;
	_pinBoot0 = pinBoot;
	_errno = 0;
	_topic = new Topic("stm32/cmd", this, stm32CmdIn, stm32CmdOut,
			Topic::F_QOS2);
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

bool Stm32::CmdReset(Msg& msg) {
	static uint32_t retries = 0;
	bool ackReceived = false;

	PT_BEGIN()
	_uart->setMode("8E1");
	_uart->setBaudrate(115200);
	_pinReset->digitalWrite(0);
	timeout(10);
	PT_YIELD_UNTIL(timeout());
	_pinReset->digitalWrite(1);
	timeout(10);
	PT_YIELD_UNTIL(timeout());
	uartClear();
	while (retries++ < 100) {
		_uart->write(STM32_SYNC);
		timeout(10);
		PT_YIELD_UNTIL(
				ackReceived = (_uart->hasData() && (STM32_ACK == _uart->read())) // --------- data In
				|| (timeout()));
		if (ackReceived)
			break;
	}
	if (retries == 100)
		_errno = EHOSTUNREACH;
	else if (ackReceived)
		_errno = E_OK;
	else
		_errno = EINVAL;
PT_END()
}
//____________________________________________________________________________
//
bool Stm32::dispatch(Msg& msg) {
switch (_cmd) {
case CMD_RESET: {
	return CmdReset(msg);
}
case CMD_IDLE: {
	return CmdGet(msg);
}
}
return true;
}

