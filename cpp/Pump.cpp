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

extern "C" const char* CLOCK_ID;
extern "C" const char* MQTT_ID;
extern "C" const char* TCP_ID;
extern "C" const char* WIFI_ID;

#include "UartEsp8266.h"
#include "mutex.h"
#include "Flash.h"
#include "LedBlink.h"
#include "Wifi.h"
#include "Sys.h"
#include "Tcp.h"
#include "MqttMsg.h"
#include "Mqtt.h"

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
}

void IROM MSG_TASK(os_event_t *e) {
	Msg::publish((const void*) e->par, (Signal) e->sig);
	while (msg->receive()) {
		if (msg->signal() != SIG_TICK)
			INFO(">>>> %s , %s ",
					((Handler* )msg->src())->getName(), strSignal[msg->signal()]);
		Handler::dispatchToChilds(*msg);
		msg->free();
	}
	SysWatchDog = Sys::millis() + 1000; // if not called within 1 second calls dump_stack;
}

void IROM tick_cb(void *arg) {
	Post(CLOCK_ID, SIG_TICK);
}

extern "C" IROM void MsgInit() {
	INFO(" Start Message Pump ");
	Msg::init();
	msg = new Msg(256);
	led = new LedBlink();
	wifi = new Wifi();
	wifi->config((const char*) STA_SSID, (const char*) STA_PASS);
	led->init();
	CreateMutex(&mutex);
	flash = new Flash();
	flash->init();
	tcp = new Tcp(wifi);
//	tcp->config("192.168.0.227", 8008);
	tcp->config("iot.eclipse.org", 1883);
//	mqttMsg = new MqttMsg(*tcp);
	mqtt = new Mqtt(tcp);
	mqtt->setPrefix("/limero314/ESP_TEST/");

	system_os_task(MSG_TASK, MSG_TASK_PRIO, MsgQueue, MSG_TASK_QUEUE_SIZE);
	Post(__FUNCTION__, SIG_INIT);
	os_timer_disarm(&pumpTimer);
	os_timer_setfn(&pumpTimer, (os_timer_func_t *) tick_cb, (void *) 0);
	os_timer_arm(&pumpTimer, 20, 1);

}

