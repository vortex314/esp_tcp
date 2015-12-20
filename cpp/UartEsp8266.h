/*
 * UartEsp8266.h
 *
 *  Created on: Sep 17, 2015
 *      Author: lieven
 */

#ifndef UARTESP8266_H_
#define UARTESP8266_H_

#include "Uart.h"
#include "CircBuf.h"

// EXPOSED C routines for C interrupt handlers

class UartEsp8266: public Uart {
public:
	CircBuf _rxd;
	CircBuf _txd;
//	uint8_t _index;
	uint32_t _bytesRxd;
	uint32_t _bytesTxd;
	uint32_t _overflowTxd;
	uint32_t _baudrate;
	char _mode[4];
	uint32_t _uartNo;
	static UartEsp8266* _uart0;
	static UartEsp8266* _uart1;
	static UartEsp8266* getUart0();
public:

	IROM UartEsp8266(uint32_t uartNo);
	IROM ~UartEsp8266();
	IROM void receive(uint8_t b);
	IROM void init(uint32_t baud);
	IROM Erc write(Bytes& data);
	IROM Erc write(uint8_t b);
	IROM Erc write(uint8_t* pb, uint32_t length);
	IROM bool hasData();
	IROM bool hasSpace();
	IROM uint8_t read();
	IROM void connect();
	IROM void disconnect();
	IROM bool isConnected();
	IROM virtual Erc setBaudrate(uint32_t baudrate);
	IROM virtual uint32_t getBaudrate();
	IROM virtual Erc setMode(const char* str);
	IROM virtual void getMode(char* str);
	IROM Erc pins(uint32_t idx);

};

#endif /* UARTESP8266_H_ */
