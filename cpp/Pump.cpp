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
#include "Gpio.h"


mutex_t mutex;

Flash* flash;
LedBlink *led;
Msg* msg;
Wifi* wifi;
TcpClient* tcpClient;
TcpServer* tcpServer;

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

inline void task_post(const char* src, Signal signal) {
	system_os_post((uint8_t) MSG_TASK_PRIO, (os_signal_t) signal,
			(os_param_t) src);
}

void IROM task_handler(os_event_t *e) {		// foreground task to handle signals async
	while (msg->receive()) {								// process all messages
		Handler::dispatchToChilds(*msg);					// send message to all
	}
	feedWatchDog(); 			// if not called within 1 second calls dump_stack
}
void IROM task_start(){
	system_os_task(task_handler, MSG_TASK_PRIO, MsgQueue, MSG_TASK_QUEUE_SIZE);
}
// ----------------------------------------------------------------------
//			SIG_TICK generator
//-----------------------------------------------------------------------
os_timer_t pumpTimer;
const char* CLOCK_ID = "CLOCK";

void IROM tick_cb(void *arg) {
	Msg::publish(CLOCK_ID, SIG_TICK);
}

void IROM tick_timer_start(){
	os_timer_disarm(&pumpTimer);										// start SIG_TICK clock
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
#include "ListMap.h"
#include <ListMap.cpp>

ListMap<uint32_t> symbols;

extern "C" IROM void MsgInit() {
//	INFO(" Start Message Pump ");
	do_global_ctors();
	symbols.add("one",1);


//	initPins();
	gpioReset = new Gpio(2); 			// D2, GPIO4 see http://esp8266.co.uk/tutorials/introduction-to-the-gpio-api/
	gpioReset->setMode("OOD");

/*	for (int i=1;i<10;i++) {
		ets_delay_us(100000);
		gpioReset->digitalWrite(1);
		ets_delay_us(100000);
		gpioReset->digitalWrite(0);
	}*/

	ets_delay_us(100000);
	ets_sprintf(deviceName, "limero314/ESP_%08X/", system_get_chip_id());

	CreateMutex(&mutex);
	msg = new Msg(256);


//	flash = new Flash();
//	flash->init();
	INFO(" SSID : %s, PSWD : %s", (const char*) STA_SSID,
			(const char*) STA_PASS);

	wifi = new Wifi();
	wifi->config((const char*) STA_SSID, (const char*) STA_PASS);
//	Tcp::globalInit(wifi,5);

	tcpServer=new TcpServer(wifi);
	tcpServer->config(5,2323);

	tcpClient = new TcpClient(wifi);
	tcpClient->config("iot.eclipse.org", 1883);

	led = new LedBlink(wifi);

	gpioFlash = new Gpio(0);

	task_start();
//	system_os_task(MSG_TASK, MSG_TASK_PRIO, MsgQueue, MSG_TASK_QUEUE_SIZE);
	Msg::publish(__FUNCTION__, SIG_INIT);								// send first SIG_INIT signal
	tick_timer_start();
}

