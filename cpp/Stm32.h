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

enum Stm32Cmd {
	STM32_RESET = 0xFF,
	STM32_GET_VERSION_AND_COMMANDS = 0x00,
	STM32_GET_VERSION_AND_READ_PROTECTION = 0X01,
	STM32_GET_ID = 0x02,
	STM32_READ_MEMORY = 0x11,
	STM32_GO = 0x21,
	STM32_WRITE_MEMORY = 0x31,
	STM32_ERASE = 0x43,
	STM32_ERASE_EXTENDED = 0x44,
	STM32_WRITE_PROTECT = 0x63,
	STM32_WRITE_UNPROTECT = 0x73,
	STM32_READOUT_PROTECT = 0x82,
	STM32_READOUT_UNPROTECT = 0x92
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

	Stm32Cmd _cmd;
	uint32_t _messageId;
	uint32_t _retries;

public:
	IROM Stm32(Mqtt* mqtt, UartEsp8266* uart, Gpio* reset, Gpio* boot0);IROM void init();IROM virtual ~Stm32();IROM virtual bool dispatch(
			Msg& msg);IROM bool CmdReset(Msg& msg);IROM bool CmdGet(Msg& msg);IROM void addUartData();IROM void uartClear();IROM bool uartDataComplete();IROM bool waitUartData(
			uint8_t* pb, uint32_t length);IROM static Erc stm32CmdIn(
			void* instance, Cbor& cbor);IROM void status(Str& str);IROM void status(
			const char* str);IROM void log(const char* fmt, ...);IROM void progress(
			uint32_t percent);
//	static Erc stm32CmdOut(void* instance, Cbor& cbor);
};

#endif /* STM32_H_ */
