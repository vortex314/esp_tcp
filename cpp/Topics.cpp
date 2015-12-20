/*
 * Topics.cpp
 *
 *  Created on: Nov 11, 2015
 *      Author: lieven
 */

#include "Erc.h"
#include "Mqtt.h"
#include "Topic.h"
#include "UartEsp8266.h"
#include "Gpio.h"

class UartTopic {
public:
	IROM static Erc sendData(void* pv, Cbor& cbor) {
		UartEsp8266* uart = (UartEsp8266*) pv;
		Bytes bytes(3);
		if (cbor.get(bytes))
			return uart->write(bytes);
		else
			return EINVAL;

	}
	IROM static Erc recvData(void* pv, Cbor& cbor) {
		UartEsp8266* uart = (UartEsp8266*) pv;
		Bytes bytes(100);
		while (uart->hasData() && bytes.hasSpace(1)) {
			bytes.write(uart->read());
		}
		if (bytes.length() > 0) {
			cbor.add(bytes);
			return E_OK;
		}
		return ENODATA;
	}
	IROM static Erc setBaudrate(void* pv, Cbor& cbor) {
		UartEsp8266* uart = (UartEsp8266*) pv;
		uint32_t br;
		if (cbor.get(br)) {
			return uart->setBaudrate(br);
		}
		return EINVAL;
	}
	IROM static Erc getBaudrate(void* pv, Cbor& cbor) {
		UartEsp8266* uart = (UartEsp8266*) pv;
		cbor.add(uart->getBaudrate());
		return E_OK;
	}
	IROM static Erc setMode(void* pv, Cbor& cbor) {
		char mode[4];
		UartEsp8266* uart = (UartEsp8266*) pv;
		if (cbor.get(mode,3)) {
//			INFO(" set Mode : %s ", mode);
			return uart->setMode(mode);
		}
		return EINVAL;
	}
	IROM static Erc getMode(void* pv, Cbor& cbor) {
		char mode[4];
		UartEsp8266* uart = (UartEsp8266*) pv;
		uart->getMode(mode);
		cbor.add(mode);
		return E_OK;
	}
	IROM static void create(UartEsp8266* uart) {
		new Topic("uart0/overflows", &UartEsp8266::_uart0->_overflowTxd, 0,
				Topic::getInteger, 0);
		new Topic("uart0/bytesRxd", &UartEsp8266::_uart0->_bytesRxd, 0,
				Topic::getUI32, 0);
		new Topic("uart0/bytesTxd", &UartEsp8266::_uart0->_bytesTxd, 0,
				Topic::getUI32, 0);
		new Topic("uart0/data", UartEsp8266::_uart0, UartTopic::sendData,
				0, 2);
		new Topic("uart0/baudrate", UartEsp8266::_uart0, UartTopic::setBaudrate,
				UartTopic::getBaudrate, 0);
		new Topic("uart0/mode", UartEsp8266::_uart0, UartTopic::setMode,
				UartTopic::getMode, 0);
	}
};

class GpioTopic {
public:
	IROM static char* newString(const char* first, const char* second) {
		char* ptr = (char*) malloc(strlen(first) + strlen(second) + 2);
		strcpy(ptr, first);
		strcat(ptr, "/");
		strcat(ptr, second);
		return ptr;
	}
	IROM static Erc setMode(void* pv, Cbor& cbor) {
		Str mode(5);
		Gpio* gpio = (Gpio*) pv;
		if (cbor.get(mode)) {
//			INFO(" set Mode : %s ", mode.c_str());
			return gpio->setMode((char*) mode.c_str());
		}
		return EINVAL;
	}
	IROM static Erc getMode(void* pv, Cbor& cbor) {
		char mode[5];
		Gpio* gpio = (Gpio*) pv;
		gpio->getMode(mode);
		cbor.add(mode);
		return E_OK;
	}

	IROM static Erc getDigital(void* pv, Cbor& cbor) {
		uint8_t v;
		Gpio* gpio = (Gpio*) pv;
		gpio->digitalRead(&v);
		cbor.add(v);
		return E_OK;
	}

	IROM static Erc setDigital(void* pv, Cbor& cbor) {
		uint32_t v;
		Gpio* gpio = (Gpio*) pv;
		if (cbor.get(v)) {
			return gpio->digitalWrite(v);
		}
		return EINVAL;
	}

	IROM static void create(const char* pin, Gpio* gpio) {

		new Topic(newString(pin, "mode"), gpio, setMode, getMode, 0);
		new Topic(newString(pin, "data"), gpio, setDigital, getDigital, 0);

	}
};

Gpio gpio0(0);
Gpio gpio2(2);

class MqttTopic {
public:
	IROM static void create() {
		new Topic("mqtt/topicSize", (void*) MQTT_SIZE_TOPIC, 0,
				Topic::getConstantInt, 0);
		new Topic("mqtt/valueSize", (void*) MQTT_SIZE_VALUE, 0,
				Topic::getConstantInt, 0);
		new Topic("mqtt/messageSize", (void*) MQTT_SIZE_MESSAGE, 0,
				Topic::getConstantInt, 0);
	}
};

Erc getSystemUptime(void* pv, Cbor& cbor) {
	cbor.add(Sys::millis());
	return E_OK;
}

void TopicsCreator(void) {
	new Topic("system/version", (void*) __FILE__ " " __DATE__ " " __TIME__, 0,
			Topic::getConstantChar, Topic::F_QOS1);
	new Topic("system/uptime", 0, 0, getSystemUptime, Topic::F_QOS2);

	UartTopic::create(UartEsp8266::_uart0);
	MqttTopic::create();
	GpioTopic::create("gpio0", &gpio0);
	GpioTopic::create("gpio2", &gpio2);

}
