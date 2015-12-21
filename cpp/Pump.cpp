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
#include "Topic.h"
#include "Gpio.h"
#include "Stm32.h"

mutex_t mutex;

Flash* flash;
LedBlink *led;
Msg* msg;
Wifi* wifi;
Tcp* tcp;
MqttMsg* mqttMsg;
Mqtt* mqtt;
MqttFramer* mqttFramer;
TopicSubscriber* topicSubscriber;
TopicPublisher* topicPublisher;
Gpio* gpioReset;
Gpio* gpioFlash;
Stm32* stm32;

#define MSG_TASK_PRIO        		1
#define MSG_TASK_QUEUE_SIZE    	100
#define MSG_SEND_TIMOUT			5

extern "C" void MsgPump();
extern "C" void MsgPublish(void* src, Signal signal);
extern "C" void MsgInit();
extern uint64_t SysWatchDog;
os_event_t MsgQueue[MSG_TASK_QUEUE_SIZE];
os_timer_t pumpTimer;
extern "C" void feedWatchDog();

inline void Post(const char* src, Signal signal) {
	system_os_post((uint8_t) MSG_TASK_PRIO, (os_signal_t) signal,
			(os_param_t) src);
}
uint32_t used=0;

void IROM MSG_TASK(os_event_t *e) {
//	if (Msg::_queue->getUsed()>used) {
//		used=Msg::_queue->getUsed();
//		INFO(" used : %d",used);
//	}
	while (msg->receive()) {
		Handler::dispatchToChilds(*msg);
	}
	feedWatchDog(); // if not called within 1 second calls dump_stack;
}

const char* CLOCK_ID = "CLOCK";

void IROM tick_cb(void *arg) {
	Msg::publish(CLOCK_ID, SIG_TICK);
}
//-----------------------------------------------------------------------
extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

static void do_global_ctors(void) {
	void (**p)(void);
	for (p = &__init_array_start; p != &__init_array_end; ++p)
		(*p)();
}
//----------------------------------------------------------------------

char deviceName[40];

extern IROM void TopicsCreator();
#include "CborQueue.h"

extern "C" IROM void MsgInit() {
//	INFO(" Start Message Pump ");
	do_global_ctors();
	Msg::init();

//	initPins();
	gpioReset = new Gpio(2); // D2, GPIO4 see http://esp8266.co.uk/tutorials/introduction-to-the-gpio-api/
	gpioReset->setMode("OOD");

	for (int i=1;i<10;i++) {
		ets_delay_us(100000);
		gpioReset->digitalWrite(1);
		ets_delay_us(100000);
		gpioReset->digitalWrite(0);
	}

	ets_delay_us(100000);
	ets_sprintf(deviceName, "limero314/ESP_%08X/", system_get_chip_id());

	CreateMutex(&mutex);
	msg = new Msg(256);


//	flash = new Flash();
//	flash->init();
	wifi = new Wifi();
	tcp = new Tcp(wifi);
	mqttFramer = new MqttFramer(tcp);
	mqtt = new Mqtt(mqttFramer);
	led = new LedBlink(tcp);

	gpioFlash = new Gpio(0);
	stm32 = new Stm32(mqtt, UartEsp8266::getUart0(), gpioReset, gpioFlash);

//	topicMgr = new TopicMgr(mqtt);

	new Topic("system/online", (void*) true, 0, Topic::getConstantBoolean,
			Topic::F_QOS1); // ---------------- at least one topic
	topicPublisher = new TopicPublisher(mqtt);
	topicSubscriber = new TopicSubscriber(mqtt);

	TopicsCreator();

	INFO(" SSID : %s, PSWD : %s", (const char*) STA_SSID,
			(const char*) STA_PASS);

	wifi->config((const char*) STA_SSID, (const char*) STA_PASS);
	tcp->config("iot.eclipse.org", 1883);
	mqtt->setPrefix(deviceName);

	system_os_task(MSG_TASK, MSG_TASK_PRIO, MsgQueue, MSG_TASK_QUEUE_SIZE);
	Msg::publish(__FUNCTION__, SIG_INIT);
	os_timer_disarm(&pumpTimer);
	os_timer_setfn(&pumpTimer, (os_timer_func_t *) tick_cb, (void *) 0);
	os_timer_arm(&pumpTimer, 10, 1);

}

