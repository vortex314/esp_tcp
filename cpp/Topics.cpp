#include "Timer.h"
#include "Msg.h"
#include "Handler.h"
#include "Mqtt.h"
#include <stdlib.h>
#include "Prop.h"
/*
typedef struct {
	uint32_t uiValue;
	const char *szValue;
} EnumEntry;

IROM bool strToEnum(uint32_t& iValue, EnumEntry* et, uint32_t count, Str& value) {
	uint32_t i;
	for (i = 0; i < count; i++)
		if (value.startsWith(et[i].szValue)) {
			iValue = et[i].uiValue;
			return true;
		}
	return false;
}

IROM bool enumToStr(Str& str, EnumEntry* et, uint32_t count, uint32_t value) {
	uint32_t i;
	for (i = 0; i < count; i++)
		if (et[i].uiValue == value) {
			str << et[i].szValue;
			return true;
		}
	return false;
}*/
//_____________________________________________________________________________
//

//_____________________________________________________________________________
//
typedef uint64_t (*pfu64)(uint64_t val);
class UInt64Topic: public Prop {
	pfu64 _fp;
public:
	IROM UInt64Topic(const char *name, pfu64 fp) :
			Prop(name, (Flags )
					{ T_UINT64, M_READ, T_1SEC, QOS_0, NO_RETAIN }) {
		_fp = fp;
	}
	IROM void toBytes(Str& topic,Bytes& message) {
		Str& str = (Str&) message;
		str.append(_fp(0));
	}
	IROM void fromBytes(Str& topic,Bytes& message) {
		Str& str = (Str&) message;
		_fp(atoll(str.c_str()));
	}
};
//_____________________________________________________________________________
//
class StringTopic: public Prop {
	const char *_s;
public:
	IROM StringTopic(const char *name, const char *s) :
			Prop(name, (Flags )
					{ T_STR, M_READ, T_10SEC, QOS_0, NO_RETAIN }) {
		_s = s;
	}
	IROM void toBytes(Str& topic,Bytes& message) {
		((Str&) message).append(_s);
	}
};

//_____________________________________________________________________________
//
class RealTimeTopic: public Prop {
	uint64_t bootTime;
public:
	IROM RealTimeTopic() :
			Prop("system/now", (Flags )
					{ T_UINT64, M_RW, T_1SEC, QOS_0, NO_RETAIN }) {
		bootTime = 0;
	}

	IROM void toBytes(Str& topic,Bytes& message) {
		Str& str = (Str&) message;
		str.append(bootTime + Sys::millis());
	}
	IROM void fromBytes(Str& topic,Bytes& message) {
		Str& str = (Str&) message;
		uint64_t now = atoll(str.c_str());
		bootTime = now - Sys::millis();
	}
};
//_____________________________________________________________________________
//
class SystemOnlineTopic: public Prop {
public:
	IROM SystemOnlineTopic() :
			Prop("system/online", (Flags )
					{ T_BOOL, M_READ, T_1SEC, QOS_1, NO_RETAIN }) {
	}

	IROM void toBytes(Str& topic,Bytes& message) {
		Str& str = (Str&) message;
		str.append(true);
	}
};

SystemOnlineTopic systemOnline;
StringTopic systemVersion("system/version", __DATE__ " " __TIME__);
RealTimeTopic rt;

//GpioTopics ledTopic("gpio/PC13", gpioLed);
