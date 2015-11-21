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

Stm32::Stm32(UartEsp8266* uart) :
		Handler("Stm32"), _in(256), _out(256), _uartIn(256) {
	_uart = uart;
}

Stm32::~Stm32() {

}

void Stm32::addUartData() {
	while (_uart->hasData()) {
		_uartIn.write(_uart->read());
	}
}

void Stm32::uartClear() {
	_uartIn.clear();
}

bool Stm32::uartDataComplete() {
	if (_uartIn.length() >= _expectedResponseLength) {
		uint32_t i;
		uint32_t count = 0;
		for (i = 0; i < _uartIn.length(); i++) {
			if ((_uartIn.peek(i) == STM32_ACK)
					|| (_uartIn.peek(i) == STM32_NACK))
				count++;
		}
		if (count >= _expectedAckCount)
			return true;
	}
	return false;
}

bool Stm32::dispatch(Msg& msg) {
	if (msg.is(_uart, SIG_RXD)) {
		addUartData();
		return true;
	};
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	while (true) {
		PT_YIELD_UNTIL(msg.is(this, SIG_RXD)); // Topic Msg
		if ( msg.scanf("iii",&_messageId,&_expectedResponseLength,&_expectedAckCount) ){

		}
		uartClear();
		erc = sendAllBlocks(msg);
		timeout(_expectedResponseLength / 10);
		PT_YIELD_UNTIL(uartDataComplete() || timeout());
		cbor.add(messageId).add(erc).add(bytes);
		Msg::publish(cmdTopic, SIG_CHANGE);
	}
PT_END()
}*/

