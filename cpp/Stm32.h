/*
 * Stm32.h
 *
 *  Created on: Nov 12, 2015
 *      Author: lieven
 */

#ifndef STM32_H_
#define STM32_H_

#include "Msg.h"
#include "Topic.h"
#include "Erc.h"
#include "Handler.h"
#include "UartEsp8266.h"
#include "Cbor.h"

// <cmd><messageId><expectedLength><expected_acks><blocksPerAck><block1>...<blokckn>
// <cmd><messageId><erc><bytes>
// GET-1234-10-1-0x11 0xEE -> GET-1234-erc

#define STM32_SYNC 0x7F
#define STM32_ACK 0x79
#define STM32_NACK 0x1F

class Stm32: public Handler {
private:
	UartEsp8266* _uart;
	Cbor _in;
	Cbor _out;
	Bytes _uartIn;
	uint32_t _expectedResponseLength;
	uint32_t _expectedAckCount;
	uint32_t _messageId;
public:
	Stm32(UartEsp8266* uart);
	virtual ~Stm32();
	bool dispatch(Msg& msg);
	void addUartData();
	void uartClear();
	bool uartDataComplete();
};

#endif /* STM32_H_ */
