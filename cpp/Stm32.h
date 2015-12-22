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
#include <CborQueue.h>
//#include <Stm32Cmd.h>

// <cmd><messageId><expectedLength><expected_acks><blocksPerAck><block1>...<blokckn>
// <cmd><messageId><erc><bytes>
// GET-1234-10-1-0x11 0xEE -> GET-1234-erc

enum CMD { STM32_CMD_UART_DATA = 1, STM32_CMD_BOOT_ENTER, STM32_CMD_BOOT_REQ, STM32_CMD_RESET};


class Msg;


class Stm32: public Handler {
private:
	static const uint8_t STM32_SYNC = 0x7F;
	static const uint8_t STM32_ACK = 0x79;
	static const uint8_t STM32_NACK = 0x1F;
	int _errno;
	Mqtt* _mqtt;
	UartEsp8266* _uart;
	Gpio* _pinReset;
	Gpio* _pinBoot0;

	Topic* _topic;

	Cbor _request;
	Cbor _response;
	Bytes _uartIn;
	Bytes _dataIn;
	CborQueue _queue;

	CMD _cmd;
	uint32_t _messageId;
	uint32_t _retries;
	uint32_t _ackCount;
	uint32_t _acksExpected;
	uint32_t _bytesExpected;

public:
	IROM Stm32(Mqtt* mqtt, UartEsp8266* uart, Gpio* reset, Gpio* boot0);
	void init() IROM;
	IROM virtual ~Stm32();
	IROM virtual bool dispatch(Msg& msg);
	IROM bool CmdReset(Msg& msg);
	IROM bool CmdGet(Msg& msg);
	IROM void addUartData();
	IROM void uartClear();
	IROM bool waitUartComplete();
	IROM static Erc stm32CmdIn(void* instance, Cbor& cbor);
	IROM void status(Str& str);
	IROM void status(const char* str);
	IROM void log(const char* fmt, ...);
	IROM void progress(uint32_t percent);
//	static Erc stm32CmdOut(void* instance, Cbor& cbor);
};

#endif /* STM32_H_ */
