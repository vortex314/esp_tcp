/*
 * Pump.cpp
 *
 *  Created on: Sep 14, 2015
 *      Author: lieven
 */
#include "Msg.h"
#include "Handler.h"

//#include "all.h"
#include "Sys.h"

extern "C" {
//#include "gpio16.h"
#include "osapi.h"
#include "ets_sys.h"
#include "user_interface.h"
#include "espmissingincludes.h"
#include "gpio_c.h"
}

#include "UartEsp8266.h"
#include "mutex.h"
#include "Flash.h"
#include "LedBlink.h"
#include "Wifi.h"
#include "Sys.h"
#include "Tcp.h"
#include "MqttMsg.h"
#include "Mqtt.h"
#include "Gpio.h"
#include "Cmd.h"
#include <DWM1000.h>

mutex_t mutex;

Flash* flash = 0;
LedBlink *led;
Msg* msg;
Wifi* wifi;
TcpClient* tcpClient;
TcpServer* tcpServer;
Cmd* cmd;
DWM1000* dwm1000;
MqttMsg* mqttMsg;
Mqtt* mqtt;
MqttFramer* mqttFramer;

Gpio* gpioReset;
Gpio* gpioFlash;

#define MSG_SEND_TIMOUT			5

extern "C" void MsgPump();
extern "C" void MsgPublish(void* src, Signal signal);
extern "C" void MsgInit();
//------------------------------------------------------------------------------------
//		foreground task and message OS queue
//------------------------------------------------------------------------------------
#define MSG_TASK_PRIO        		1
#define MSG_TASK_QUEUE_SIZE    	100
os_event_t MsgQueue[MSG_TASK_QUEUE_SIZE];
extern "C" void feedWatchDog();

void task_post(const char* src, Signal signal) {
	system_os_post((uint8_t) MSG_TASK_PRIO, (os_signal_t) signal,
			(os_param_t) src);
}

void IROM task_handler(os_event_t *e) {	// foreground task to handle signals async
	while (msg->receive()) {							// process all messages
		Handler::dispatchToChilds(*msg);				// send message to all
	}
	feedWatchDog(); 		// if not called within 1 second calls dump_stack
}

void IROM task_start() {
	system_os_task(task_handler, MSG_TASK_PRIO, MsgQueue, MSG_TASK_QUEUE_SIZE);
}
// ----------------------------------------------------------------------
//			SIG_TICK generator
//-----------------------------------------------------------------------
os_timer_t pumpTimer;
const char* CLOCK_ID = "CLOCK";

void IRAM tick_cb(void *arg) {
	Msg::publish(CLOCK_ID, SIG_TICK);
}

void IROM tick_timer_start() {
	os_timer_disarm(&pumpTimer);						// start SIG_TICK clock
	os_timer_setfn(&pumpTimer, (os_timer_func_t *) tick_cb, (void *) 0);
	os_timer_arm(&pumpTimer, 10, 1);
}

//-----------------------------------------------------------------------------------
//		initialize global variables
//-----------------------------------------------------------------------------------
extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

static void do_global_ctors(void) {
	void (**p)(void);
	for (p = &__init_array_start; p != &__init_array_end; ++p)
		(*p)();
}
//----------------------------------------------------------------------------------

char deviceName[40];

#include "CborQueue.h"
#include <SlipFramer.h>

SlipFramer* slipFramer;
Tcp* tcp;
#include <Wildcard.h>
#include <Router.h>
#include <Json.h>
class Stm32: public Subscriber {
public:

	Erc handle(Cbor& msg) {
		INFO("have been called");
		return E_OK;
	}
};
Stm32 stm32;
#include <Base64.h>

//Enum Cmd(STRINGS, Values);

void getConfig(Str& dest, const char* key, const char* defaultValue) {
	if (flash == 0) {
		flash = new Flash();
		flash->init();
	};
	dest.clear();
	uint16_t len = dest.capacity();
	if (flash->get(key, dest.data(), &len) == E_OK) {
		INFO("%s : %d", dest.data(), len);
		dest.length(len);
		dest.c_str();
		INFO("%s : %d", dest.data(), len);
		return;
	}
	dest = defaultValue;
}


void getConfig(uint32_t* dest, const char* key, uint32_t defaultValue) {
	if (flash == 0) {
		flash = new Flash();
		flash->init();
	};
	uint16_t len = 4;
	if (flash->get(key, (uint8_t*) dest, &len) == E_OK) {
		INFO("%d : %d", *dest, len);
		return;
	}
	*dest = defaultValue;
}

/*
 char value[40], key[40];
 //	flash->put((uint16_t)0,(uint8_t*)"mqtt/port",9);
 //	flash->put((uint16_t)1,(uint8_t*)"test.mosquitto.org",18);
 uint16_t length = sizeof(key);
 flash->get((uint16_t) 0, (uint8_t*) key, &length);
 key[length] = 0;
 INFO(" key : %s ", key);
 length = sizeof(value);
 flash->get((uint16_t) 1, (uint8_t*) value, &length);
 value[length] = 0;
 INFO(" value : %s ", value);
 length = sizeof(value);
 flash->get("mqtt/port", value, &length);
 INFO(" value : %s ", value);
 char value[40], key[40];
 flash->put("key","newValue1");
 flash->set("mqtt/host","test.mosquitto.org");
 flash->set("mqtt/port","1883");
 flash->set("mqtt/prefix","limero/anchor1");
 flash->get(value, sizeof(value), "key", "default");
 INFO("value : %s ", value);
 flash->init();
 for (int i = 2; i < 100; i += 2) {
 if (flash->get(key, sizeof(key), i))
 break;
 if (flash->get(value, sizeof(value), i + 1))
 break;
 INFO(" %d : %s = %s ", i, key, value);
 }
 */

//	flash->get(value,sizeof(value),"key","default");
//	INFO("value : %s ",value);
extern "C" void deca_example();

extern "C" IROM void MsgInit() {

	do_global_ctors();
//	deca_example();

//	initPins();
	gpioReset = new Gpio(2); // D2, GPIO4 see http://esp8266.co.uk/tutorials/introduction-to-the-gpio-api/
	gpioReset->setMode("OOD");
	Json json(100);
	Str str(30);
	str = "Hello World";
	uint8_t array[] = { 0, 1, 2, 0x80, 0xFF };
	Bytes bytes(array, sizeof(array));
	Str str2(30);

	Base64::encode(str, bytes);
	bytes.toHex(str2.clear());

	INFO("%s:%s", str.c_str(), str2.c_str());

	Base64::decode(bytes, str);
	bytes.toHex(str2.clear());

	INFO("%s:%s", str.c_str(), str2.c_str());

	json.addMap().addKey("id").add(1234).addKey("cmd").add("write").addKey(
			"data").add(str.c_str());
	json.addKey("object").addMap().addKey("sub1").add(true);
	json.addKey("arr").addArray().add(1).add("hi").addBreak().addBreak().addBreak();

	if (json.parse() != E_OK)
		INFO(" parsing failed for %s ", json.c_str());

	json.clear().append("{\"key\":1234,\"cmd\":{\"key2\":11111}}");

	INFO("%s", json.c_str());

	json.parse();
	if (json.findKey("cmd")) {
		json.mapToken(str);
		INFO("cmd : %s", str.c_str());
		if (json.findKey("key2")) {
			Str str(0);
			json.mapToken(str);
			INFO("key2 : %s", str.c_str());
		}
	}
	json.rewind();
	if (json.findKey("key")) {
		json.mapToken(str);
		INFO("key : %s", str.c_str());
	}

	getConfig(str, "mqtt/port", "test.eclipse.org");
	INFO(" config %s : %d ", str.data(), str.length());

//	if ( wildcardMatch("put/aa/bb/stm32/cmd","put/*/*/stm32/cmd",true,'\0')) INFO("matched");

//	new Router("put/*/*/stm32/cmd", &stm32);
	Cbor cbor(10);
//	Router::publish("put/aa/bb/stm32/cmd",cbor);

	/*	for (int i=1;i<10;i++) {
	 ets_delay_us(100000);
	 gpioReset->digitalWrite(1);
	 ets_delay_us(100000);
	 gpioReset->digitalWrite(0);
	 }*/

	ets_delay_us(100000);
	ets_sprintf(deviceName, "limero314/ESP_%08X/", system_get_chip_id());

	CreateMutex(&mutex);
	msg = new Msg(512);


	Str SSID(50);
	Str PASS(100);

	getConfig(SSID, "wifi/ssid", (const char*) STA_SSID);
	getConfig(PASS, "wifi/pswd", (const char*) STA_PASS);
	INFO(" SSID : %s, PSWD : %s", SSID.c_str(),PASS.c_str());

	wifi = new Wifi();
	wifi->config(SSID.c_str(), PASS.c_str());

	tcpServer = new TcpServer(wifi);
	tcpServer->config(0, 2323);

	tcp = new Tcp(wifi);
	INFO("tcp %X ", tcp);

//	wifi = new Wifi();
//	tcp = new Tcp(wifi);
//	mqttFramer = new MqttFramer(tcp);
//	mqtt = new Mqtt(mqttFramer);

	slipFramer = new SlipFramer(tcp);
	cmd = new Cmd(slipFramer);

//	tcpClient = new TcpClient(wifi);
//	tcpClient->config("iot.eclipse.org", 1883);

	led = new LedBlink(wifi);

	gpioFlash = new Gpio(0);

//	dwm1000 = new DWM1000();

	task_start();

	Msg::publish(__FUNCTION__, SIG_INIT);		// send first SIG_INIT signal
	tick_timer_start();
}

