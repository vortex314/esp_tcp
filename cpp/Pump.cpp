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
#include "Props.h"

// uint32_t __count = 0;
//Sender sender();
mutex_t mutex;
//extern "C" uint32_t conflicts;
Flash* flash;
LedBlink *led;
Msg* msg;
Wifi* wifi;
Tcp* tcp;
MqttMsg* mqttMsg;
Mqtt* mqtt;
MqttFramer* mqttFramer;
Props* props;

#define MSG_TASK_PRIO        		1
#define MSG_TASK_QUEUE_SIZE    	100
#define MSG_SEND_TIMOUT			5

extern "C" void MsgPump();
extern "C" void MsgPublish(void* src, Signal signal);
extern "C" void MsgInit();
extern uint64_t SysWatchDog;
os_event_t MsgQueue[MSG_TASK_QUEUE_SIZE];
os_timer_t pumpTimer;
extern uint64_t SysWatchDog;

inline void Post(const char* src, Signal signal) {
	system_os_post((uint8_t) MSG_TASK_PRIO, (os_signal_t) signal,
			(os_param_t) src);
//	Msg::publish((const void*) src, (Signal) signal);
}

void IROM MSG_TASK(os_event_t *e) {
//	Msg::publish((const void*) e->par, (Signal) e->sig);
	while (msg->receive()) {
//		if (msg->signal() != SIG_TICK)
//			INFO(">>>> %s , %s ",
//					((Handler* )msg->src())->getName(), strSignal[msg->signal()]);
		Handler::dispatchToChilds(*msg);
		msg->free();
	}
	SysWatchDog = Sys::millis() + 1000; // if not called within 1 second calls dump_stack;
}

const char* CLOCK_ID = "CLOCK";

void IROM tick_cb(void *arg) {
	Msg::publish(CLOCK_ID, SIG_TICK);
}

extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

static void do_global_ctors(void) {
    void (**p)(void);
    for(p = &__init_array_start; p != &__init_array_end; ++p)
        (*p)();
}

extern "C" IROM void MsgInit() {
	INFO(" Start Message Pump ");
	do_global_ctors();
	Msg::init();
	CreateMutex(&mutex);
	msg = new Msg(256);

	flash = new Flash();
	flash->init();

	wifi = new Wifi();
	tcp = new Tcp(wifi);
	mqttFramer = new MqttFramer(tcp);
	mqtt = new Mqtt( mqttFramer);
	led = new LedBlink(tcp);
	props = new Props(mqtt);

	wifi->config((const char*) STA_SSID, (const char*) STA_PASS);
	tcp->config("iot.eclipse.org", 1883);
//	tcp->config("192.168.0.227", 1883);
//	tcp->config("test.mosquitto.org", 1883);

	char deviceName[40];
	ets_sprintf(deviceName, "/limero314/ESP_%08X/", system_get_chip_id());
	mqtt->setPrefix(deviceName);

//	led->init();

	system_os_task(MSG_TASK, MSG_TASK_PRIO, MsgQueue, MSG_TASK_QUEUE_SIZE);
	Msg::publish(__FUNCTION__, SIG_INIT);
	os_timer_disarm(&pumpTimer);
	os_timer_setfn(&pumpTimer, (os_timer_func_t *) tick_cb, (void *) 0);
	os_timer_arm(&pumpTimer, 10, 1);

}

