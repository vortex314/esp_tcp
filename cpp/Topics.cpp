/*
 * Topics.cpp
 *
 *  Created on: Nov 11, 2015
 *      Author: lieven
 */

#include "Erc.h"
#include "Mqtt.h"
#include "Topic.h"


 Erc getSystemUptime(void* pv, Cbor& cbor) {
	cbor.add(Sys::millis());
	return E_OK;
}
#include "UartEsp8266.h"

class UartTopic {
public:
//	static Bytes bytes(256);
	IROM static void create(UartEsp8266* uart) {
		new Topic("uart0/overflows", &UartEsp8266::_uart0->_overflowTxd, 0,
				Topic::getInteger, 0);
		new Topic("uart0/bytesRxd", &UartEsp8266::_uart0->_bytesRxd, 0,
				Topic::getUI32, 0);
		new Topic("uart0/bytesTxd", &UartEsp8266::_uart0->_bytesTxd, 0,
				Topic::getUI32, 0);
		new Topic("uart0/data", UartEsp8266::_uart0, UartTopic::sendData,
				UartTopic::recvData, 2);
		new Topic("uart0/baudrate", UartEsp8266::_uart0, UartTopic::setBaudrate,
				UartTopic::getBaudrate, 2);
		new Topic("uart0/mode", UartEsp8266::_uart0, UartTopic::setMode,
				UartTopic::getMode, 2);
	}
	IROM static Erc sendData(void* pv, Cbor& cbor) {
		UartEsp8266* uart = (UartEsp8266*) pv;
		Bytes bytes(3);
		if (cbor.get(bytes))
			return uart->write(bytes);
		else
			return EINVAL;

	}
	IROM static Erc recvData(void* pv, Cbor& cbor){
		UartEsp8266* uart = (UartEsp8266*) pv;
		Bytes bytes(100);
		while ( uart->hasData() && bytes.hasSpace(1)) {
			bytes.write(uart->read());
		}
		if ( bytes.length() >0 ) {
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
		Str mode(0);
		UartEsp8266* uart = (UartEsp8266*) pv;
		if (cbor.get(mode)) {
			return uart->setMode(mode);
		}
		return EINVAL;
	}
	IROM static Erc getMode(void* pv, Cbor& cbor) {
		Str mode(4);
		UartEsp8266* uart = (UartEsp8266*) pv;
		uart->getMode(mode);
		cbor.add(mode);
		return E_OK;
	}
};

class MqttTopic {
public:
	IROM static void create() {
		new Topic("mqtt/topicSize", (void*) MQTT_SIZE_TOPIC, 0,
				Topic::getConstantInt, 0);
		new Topic("mqtt/valueSize", (void*) MQTT_SIZE_VALUE, 0,
				Topic::getConstantInt, 0);
		new Topic("mqtt/messageSize", (void*) MQTT_SIZE_MESSAGE, 0,
				Topic::getConstantInt, 0);
		;
	}
};

 void TopicsCreator(){
	new Topic("system/online", (void*) true, 0, Topic::getConstantBoolean,
			Topic::F_QOS1 + Topic::F_RETAIN);
//	new Topic("system/uptime", 0, 0, getSystemUptime, Topic::F_QOS2);
	UartTopic::create(UartEsp8266::_uart0);
	MqttTopic::create();

}
