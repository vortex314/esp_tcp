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
#include "Gpio.h"
#include "Cbor.h"

// <cmd><messageId><expectedLength><expected_acks><blocksPerAck><block1>...<blokckn>
// <cmd><messageId><erc><bytes>
// GET-1234-10-1-0x11 0xEE -> GET-1234-erc

enum Stm32Cmd {
	CMD_IDLE = 0,
	CMD_RESET,
	CMD_GET,
	CMD_GET_VERSION,
	CMD_GET_ID,
	CMD_READ_MEMORY,
	CMD_WRITE_MEMORY,
	CMD_GO,
	CMD_ERASE,
	CMD_ERASE_EXTENDED,
	CMD_WRITE_PROTECT,
	CMD_WRITE_UNPROTECT,
	CMD_READOUT_PROTECT,
	CMD_READOUT_UNPROTECT
};
class Stm32;
class Msg;

typedef bool (*Sequence)(Stm32& stm32, Msg& msg); // true while is busy

class Stm32: public Handler {
private:
	static const uint8_t STM32_SYNC = 0x7F;
	static const uint8_t STM32_ACK = 0x79;
	static const uint8_t STM32_NACK = 0x1F;
	int _errno;
	UartEsp8266* _uart;
	Gpio* _pinReset;
	Gpio* _pinBoot0;
	int _cmd;
	uint32_t _messageId;
	Topic* _topic;

	Cbor _in;
	Cbor _out;
	Bytes _uartIn;

public:
	Stm32(UartEsp8266* uart, Gpio* reset, Gpio* boot0);
	virtual ~Stm32();
	virtual bool dispatch(Msg& msg);
	bool CmdReset(Msg& msg);
	bool CmdGet(Msg& msg);
	void addUartData();
	void uartClear();
	bool uartDataComplete();
	static Erc stm32CmdIn(void* instance, Cbor& cbor);
	static Erc stm32CmdOut(void* instance, Cbor& cbor);
};

#endif /* STM32_H_ */
