/*
 * UartEsp8266.cpp
 *
 *  Created on: Oct 4, 2015
 *      Author: lieven
 */

/*
 * Uart.cpp
 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */

#include "UartEsp8266.h"
extern "C" {
#include "uart.h"
void uart_tx_intr_enable(uint32_t uartno);
void uart_tx_intr_disable(uint32_t uartno);
void uart_rx_intr_disable(uint8 uart_no);
void uart_rx_intr_enable(uint8 uart_no);
void uart_config(uint32_t uart_no, uint32_t baudrate, char* mode);
}
#include "Str.h"

#define UART_DEFAULT_MODE "8E1"

 UartEsp8266::UartEsp8266(uint32_t uartNo) :
		_rxd(256), _txd(3000) {
	_uartNo = uartNo;
	_bytesRxd = 0;
	_bytesTxd = 0;
	_overflowTxd = 0;
	_baudrate = 115200;
	strcpy(_mode, UART_DEFAULT_MODE);
}

 UartEsp8266::~UartEsp8266() {
	ERROR(" dtor called ");
}
//__________________________________________________________________
//
 Erc UartEsp8266::write(Bytes& bytes) {
	bytes.offset(0);
	while (_txd.hasSpace() && bytes.hasData()) {
		_txd.write(bytes.read());
	};
	if (bytes.hasData()) {
		_overflowTxd++;
	}
	if (_txd.hasData()) {
		uart_tx_intr_enable(_uartNo);
	}
	return E_OK;
}

 Erc UartEsp8266::write(uint8_t* pb, uint32_t length) {
	uint32_t i = 0;
	while (_txd.hasSpace() && i < length) {
		_txd.write(pb[i++]);
	};
	if (i < length) {
		_overflowTxd++;
	}
	if (_txd.hasData()) {
		uart_tx_intr_enable(_uartNo);
	}
	return E_OK;
}

 Erc UartEsp8266::write(uint8_t data) {
	if (_txd.hasSpace())
		_txd.write(data);
	else {
		_overflowTxd++;
	}
	if (_txd.hasData()) {
		uart_tx_intr_enable(_uartNo);
	}
	return E_OK;
}

 uint8_t UartEsp8266::read() {
	return _rxd.read();
}

bool UartEsp8266::hasData() { // not in IROM as it will be called in interrupt
	return _rxd.hasData();
}

 bool UartEsp8266::hasSpace() {
	return _txd.hasSpace();
}

void UartEsp8266::receive(uint8_t b) { // not in IROM as it will be called in interrupt
	_rxd.writeFromIsr(b);
	_bytesRxd++;
}

//__________________________________________________________________ HARDWARE SPECIFIC

 void UartEsp8266::init(uint32_t baud) {

}

UartEsp8266* UartEsp8266::_uart0 = 0;
UartEsp8266* UartEsp8266::_uart1 = 0;

UartEsp8266* UartEsp8266::getUart0() {
	if (UartEsp8266::_uart0 == 0) {
		UartEsp8266::_uart0 = new UartEsp8266(0);
		UartEsp8266::_uart0->setBaudrate(115200);
		Str mode(4);
		mode = UART_DEFAULT_MODE;
		UartEsp8266::_uart0->setMode(mode);
	}
	return UartEsp8266::_uart0;
}

/**********************************************************
 * USART1 interrupt request handler:
 *********************************************************/
#include "uart.h"
// incoming char enqueue it
extern "C" void uart0RecvByte(uint8_t b) {
	UartEsp8266* uart0 = UartEsp8266::getUart0();
	uart0->receive(b);
}
// get next byte to transmit or return -1 if not available/empty circbuf
extern "C" int uart0SendByte() {
	UartEsp8266* uart0 = UartEsp8266::getUart0();
	if (uart0->_txd.hasData()) {
		uart0->_bytesTxd++;
		return uart0->_txd.readFromIsr();
	}
	return -1;
}
// enqueue char to send in TXD
extern "C" void uart0Write(uint8_t b) {
	UartEsp8266* uart0 = UartEsp8266::getUart0();
	if (b == '\n') {
		uart0->write('\r');
		uart0->write(b);
	} else if (b == '\r') {

	} else {
		uart0->write(b);
	}
}

extern "C" void uart0WriteBytes(uint8_t *pb, uint32_t size) {
	UartEsp8266* uart0 = UartEsp8266::getUart0();
	if (uart0->_txd.hasSpace(size))
		for (uint32_t i = 0; i < size; i++)
			uart0Write(*(pb + i));
	else {
		uart0Write('#');
	}
}
 void UartEsp8266::connect() {

}
 void UartEsp8266::disconnect() {

}
bool UartEsp8266::isConnected() {
	return true;
}

 uint32_t UartEsp8266::getBaudrate() {
	return _baudrate;
}

 Erc UartEsp8266::setBaudrate(uint32_t baudrate) {
	_baudrate = baudrate;
	uart_config(_uartNo, _baudrate, _mode);
	return E_OK;
}

 void UartEsp8266::getMode(Str& str) {
	str = _mode;
}

 Erc UartEsp8266::setMode(Str& str) {
	//TODO check legal values
	strncpy(_mode, str.c_str(), 3);
	INFO("uart settings %d %d %s", _uartNo, _baudrate, _mode);
	uart_config(_uartNo, _baudrate, _mode);
	return E_OK;
}

