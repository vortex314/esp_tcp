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
	while (msg->receive()) {
		Handler::dispatchToChilds(*msg);
		msg->free();
	}
	SysWatchDog = Sys::millis() + 1000; // if not called within 1 second calls dump_stack;
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
#include "Json.h"

class Stm32 {
public:
	static const char* fields[];
	enum Fields {
		CMD, MSG_ID, ADDRESS
	};
	enum Cmds {
		RESET, STATUS, GO
	};
	static const char* cmds[3];

	static const char* load() {
		const char* fs[] = { "aaaa", "bbbb" };
		return fs[0];
	}

	static const char* enumToString(uint32_t i) {
		if (i >= 0 && i < sizeof(cmds))
			return cmds[i];
		return "";
	}

	static int stringToEnum(const char* s) {
		uint32_t i;
		for (i = 0; i < sizeof(cmds); i++)
			if (strcmp(cmds[i], s) == 0)
				return i;
		return -1;
	}

};

const char* Stm32::cmds[] = { "RESET", "STATUS", "GO" };
const char* Stm32::fields[] = { "cmd", "msgId", "address", "bytes" };

extern "C" IROM void MsgInit() {
	INFO(" Start Message Pump ");
	do_global_ctors();
	Msg::init();
	INFO(" %s : %d ", Stm32::enumToString(Stm32::STATUS),
				Stm32::stringToEnum("STATUS"));
	int i, j;
	bool b;
	Str s(20);
	Json json(100);
	for (i = 0; i < 3; i++) {
		INFO(" JSON TEST ");
		json.clear();
		json.addArray().add(100).add("Hello world").add(false).addNull();
		json.addMap().addKey("key").add("value").addBreak();
		json.addBreak();
		INFO("json:%s", json.c_str());
		json.parse();
		json.scanf("[iSb{", &j, &s, &b);
		INFO(" json : [%d,'%s',%d]", j, s.c_str(), b);
	}
//	initPins();
	/*
	 CborQueue queue(1000);
	 int i;
	 Cbor cbor(30);
	 Cbor ret(0);
	 Str string(40);
	 for (i = 0; i < 10; i++) {
	 uint32_t erc;
	 bool b;
	 double f;
	 char str[20];
	 uint32_t l;

	 cbor.clear();
	 cbor.addf("bsi", true, " Hi di hi", 12);
	 ((Bytes)cbor).toHex(string.clear());
	 INFO("hex- : %s",string.c_str());

	 erc = queue.putf("bsi", true, " Hi di hi", 12);
	 if (erc)
	 INFO(" couldn't putf  %d : %d ", i, erc);
	 erc = queue.get(cbor);
	 if (erc)
	 INFO(" couldn't get  %d : %d ", i, erc);
	 ((Bytes)cbor).toHex(string.clear());
	 INFO("hex : %s",string.c_str());

	 erc = queue.getf("bfsi", &b, &f, str, 20, &l);
	 if (erc)
	 INFO(" couldn't getf  %d : %d ", i, erc);
	 if (l != 12)
	 INFO(" couldn't get cbor uint32_t %d", i);*/
//}
	INFO(" PUMPING ");ets_delay_us(100000);
	ets_sprintf(deviceName, "limero314/ESP_%08X/", system_get_chip_id());

	CreateMutex(&mutex);
	msg = new Msg(256);
	INFO(" PUMPING ");ets_delay_us(100000);

//	flash = new Flash();
//	flash->init();
	INFO(" PUMPING ");ets_delay_us(100000);

	wifi = new Wifi();
	tcp = new Tcp(wifi);
	mqttFramer = new MqttFramer(tcp);
	mqtt = new Mqtt(mqttFramer);
	led = new LedBlink(tcp);
//	topicMgr = new TopicMgr(mqtt);

	new Topic("system/online", (void*) true, 0, Topic::getConstantBoolean,
			Topic::F_QOS1); // ---------------- at least one topic
	topicPublisher = new TopicPublisher(mqtt);
	topicSubscriber = new TopicSubscriber(mqtt);

	TopicsCreator();

	wifi->config((const char*) STA_SSID, (const char*) STA_PASS);
	tcp->config("iot.eclipse.org", 1883);
	mqtt->setPrefix(deviceName);

	system_os_task(MSG_TASK, MSG_TASK_PRIO, MsgQueue, MSG_TASK_QUEUE_SIZE);
	Msg::publish(__FUNCTION__, SIG_INIT);
	os_timer_disarm(&pumpTimer);
	os_timer_setfn(&pumpTimer, (os_timer_func_t *) tick_cb, (void *) 0);
	os_timer_arm(&pumpTimer, 10, 1);

}

