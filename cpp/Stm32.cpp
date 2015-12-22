/*
 * Stm32.cpp
 *
 *  Created on: Nov 12, 2015
 *      Author: lieven
 */

#include <Stm32.h>
#include <Mqtt.h>
// topic : stm32/cmd
// topic : stm32/status - from device : status of programming, ready refreshes every 5 seconds
// topic : stm32/log - log different activities
// topic : stm32/uart - to&from uart log traces
// stm32/cmd
// 		request in : <cmd:u><messageId:u>[<data:Bytes><expected_acks_bytes:u>] * N
// 		response out : <cmd:u><messageId:u><erc:u><data:bytes>
// 		if expected_acks > 2 indicates length of data received
// CRC calculated at host side and added as data to field
//
// cmd : 0 = STM32_CMD_RESET
// cmd : 1 = STM32_CMD_BOOT_ENTER ( reset STM32 and send SYNC char )
// cmd : 2 = STM32_CMD_BOOT_REQ ( wait Acks )
// cmd : 3 = STM32_CMD_UART_DATA ( output data or input data )

Stm32::Stm32(Mqtt* mqtt, UartEsp8266* uart, Gpio* pinReset, Gpio* pinBoot) :
		Handler("Stm32"), _request(300), _response(300), _uartIn(300), _dataIn(
				300), _queue(1000) {
	_mqtt = mqtt;
	_uart = uart;
	_pinBoot0 = pinBoot;
	_pinReset = pinReset;
	_cmd = STM32_CMD_UART_DATA;
	_messageId = 0;
	_pinReset = pinReset;
	_pinBoot0 = pinBoot;
	_errno = 0;
	_topic = new Topic("stm32/cmd", this, stm32CmdIn, 0,
			Topic::F_QOS2 + Topic::F_NO_POLL);
	_bytesExpected = 0;
	_acksExpected = 0;
	_ackCount = 0;
	_retries = 0;
}

Stm32::~Stm32() {
}

void Stm32::init() {
	/*	_uart->pins(1);
	 _uart->setMode("8E1");
	 _uart->setBaudrate(115200);
	 _pinBoot0->setMode("OPP");
	 _pinBoot0->digitalWrite(1); */
	_pinReset->setMode("OPP");
	_pinReset->digitalWrite(1);
}

Erc Stm32::stm32CmdIn(void* instance, Cbor& cbor) {
	Stm32* stm32 = (Stm32*) instance;
// INFO("put message for STM32");
	return stm32->_queue.put(cbor);
}

void Stm32::status(const char* s) {
	if (!_mqtt->isConnected())
		return;
	Cbor str(30);
	str.add(s);
	_mqtt->publish("stm32/status", str, Topic::F_QOS0);
}

char _logBuffer[100];
Cbor cbor(200);
extern "C" {
#include <util.h>
}
;

void Stm32::log(const char* format, ...) {
	_logBuffer[0] = 0;
	va_list args;
	va_start(args, format);
	ets_vsnprintf(_logBuffer, sizeof(_logBuffer), format, args);
	va_end(args);
	cbor.clear();
	cbor.add(_logBuffer);
	_mqtt->publish("stm32/log", cbor, Topic::F_QOS0);
}

void Stm32::progress(uint32_t perc) {
	if (!_mqtt->isConnected())
		return;
	Cbor str(30);
	str.add(perc);
	_mqtt->publish("stm32/progress", str, Topic::F_QOS0);
}

bool Stm32::waitUartComplete() {
	while (_uart->hasData()) {
		uint8_t b = _uart->read();
		_uartIn.write(b);
		if (b == STM32_ACK) {
			_ackCount++;
			if (_ackCount >= _acksExpected)
				return true;
		}
		if (_uartIn.length() == _bytesExpected)
			return true;
	}
	return false;
}

bool Stm32::dispatch(Msg& msg) {
	bool ackReceived;
	PT_BEGIN()
	INIT: {
		PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
		init();
	}
	START: {
		_response.clear();
		_uartIn.clear();
		_errno = 0;
		status("Ready");
		progress(0);
		timeout(5000);
		PT_YIELD_UNTIL(_queue.hasData() || timeout() || msg.is(_uart, SIG_RXD));
		if (timeout()) {
			goto START;
		} else if (msg.is(_uart, SIG_RXD)) {
			while (_uart->hasData())
				_uartIn.write(_uart->read());
			_response.addf("uuuB", STM32_CMD_UART_DATA, _messageId++, 0,
					&_uartIn);
			_mqtt->publish("stm32/uart", _response, MQTT_QOS0_FLAG);
			goto START;
		} else {
			_queue.get(_request);
			_request.scanf("uuu", &_cmd, &_messageId, &_errno);
			switch (_cmd) {
			case STM32_CMD_BOOT_ENTER:
				goto STATE_BOOT_ENTER;
			case STM32_CMD_BOOT_REQ:
				goto STATE_BOOT_CMD;
			case STM32_CMD_RESET:
				goto STATE_RESET;
				//enum CMD { STM32_CMD_UART_DATA=1, STM32_CMD_BOOT_ENTER, STM32_CMD_BOOT_REQ, STM32_CMD_RESET};
			default:
				log("invalid command received :%d", _cmd);
			}
		};
	}
	goto START;
	STATE_BOOT_ENTER: {
		// extract data to work with
		_errno = E_OK;
		status("Entering bootloader.");
		progress(1);
		_uart->pins(1);
		_uart->setMode("8E1");
		_uart->setBaudrate(115200);
		_pinReset->setMode("OPP");
		_pinReset->digitalWrite(0);
		timeout(10);
		PT_YIELD_UNTIL(timeout());
		_pinReset->digitalWrite(1);
		timeout(10);
		PT_YIELD_UNTIL(timeout());
		while (_uart->hasData())	// empty UART
			_uart->read();
		progress(10);
		_retries = 0;
		_acksExpected = 1;
		_bytesExpected = 1000;
		_ackCount = 0;
		ackReceived = false;
		while (_retries++ < 10) {
			_uart->write(STM32_SYNC);
			timeout(10);
			PT_YIELD_UNTIL(msg.is(_uart, SIG_RXD) || timeout());
			if (waitUartComplete()) {
				ackReceived = true;
				break;
			}
		}
		progress(100);
		if (ackReceived) {
			log(" device reset after %d SYNC", _retries);
			_errno = E_OK;
		} else if (_retries >= 10) {
			_errno = EHOSTUNREACH;
			log("no device found : %s %d %d %d ", strerror(_errno), _ackCount,
					_acksExpected, _bytesExpected);
		};
		status(strerror(_errno));
		_response.addf("uuuB", _cmd, _messageId, _errno, &_uartIn);
		_mqtt->publish("stm32/cmd", _response, MQTT_QOS2_FLAG);
//		_uart->pins(0);
		goto START;
	}
	STATE_RESET: {
		// extract data to work withStm32Cmd::INVALID
		_errno = E_OK;
		status("Resetting.");
		_uart->pins(1);
		_uart->setMode("8E1");
		_uart->setBaudrate(115200);
		_pinReset->digitalWrite(0);
		timeout(10);
		PT_YIELD_UNTIL(timeout());
		_pinReset->digitalWrite(1);
		timeout(10);
		PT_YIELD_UNTIL(timeout());
		progress(100);
		status(strerror(_errno));
		_response.addf("uuuB", _cmd, _messageId, _errno, 0);
		_mqtt->publish("stm32/cmd", _response, MQTT_QOS2_FLAG);
//		_uart->pins(0);
		goto START;
	}
	STATE_BOOT_CMD: {
		status("Executing Request");
		_retries = 0;
		_errno = 0;
		_uartIn.clear();
		progress(1);
//		_uart->pins(1);
		while (_request.hasData()) {
			int count;
			if (_request.scanf("Bi", &_dataIn, &count)) {
				if (count >= 0) {
					_bytesExpected = 1000;
					_acksExpected = count;
				} else {
					_bytesExpected = -count;
					_acksExpected = 1000;
				}
				_uart->write(_dataIn);
				_ackCount = 0;
				timeout(30);// at 115200 baud, 8E1, 115 bytes per 10 msec, 345 bytes in 30 msec
				while (true) {
					PT_YIELD_UNTIL(msg.is(_uart, SIG_RXD) || timeout());
					if (waitUartComplete())
						break;
					if (timeout()) {
						log("timed out on uart receive ");
						_errno = ETIMEDOUT;
						break;
					}
				}
			} else {
				log(" bad request ");
				_errno = EINVAL;
			}
			if (_errno)
				break;
		}
		progress(100);
		status(strerror(_errno));
		_response.addf("uuuB", _cmd, _messageId, _errno, &_uartIn);
		_mqtt->publish("stm32/cmd", _response, MQTT_QOS2_FLAG);
//		_uart->pins(0);
		goto START;
	}
PT_END()
;
}

//____________________________________________________________________________
