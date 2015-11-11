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

	UartEsp8266(uint32_t uartNo);
	~UartEsp8266();
	void receive(uint8_t b);
	void init(uint32_t baud);
	Erc write(Bytes& data);
	Erc write(uint8_t b);
	Erc write(uint8_t* pb, uint32_t length);
	bool hasData();
	bool hasSpace();
	uint8_t read();
	void connect();
	void disconnect();
	bool isConnected();
	virtual Erc setBaudrate(uint32_t baudrate);
	virtual uint32_t getBaudrate();
	virtual Erc setMode(Str& str);
	virtual void getMode(Str& str);

};

#endif /* UARTESP8266_H_ */
