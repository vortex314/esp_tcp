/*
 * Message.h
 *
 *  Created on: Dec 28, 2015
 *      Author: lieven
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_
#include <Cbor.h>

class Message: public Cbor {
public:
	typedef enum {
		INVALID,
		DST_DOMAIN,
		DST_DEVICE,
		DST_INSTANCE,
		SRC_DOMAIN,
		SRC_DEVICE,
		SRC_INSTANCE,
		MESSAGE_ID,
		TYPE, // REQUEST,RESPONE,RECORD,RELEASE,COMPLETE,ACK
		QOS,	// NO_CONF,ACK,ONCE
		PROP_CMD,
		DATA=256,
		TEXT,
		UART_PORT,
		UART_BAUDRATE,
		UART_MODE,
		TCP_HOST,
		TCP_PORT
	} Field;

	Message(int size);
	virtual ~Message();
	template<class typ> void addField(Field field, typ x) {
		addKey((int) field);
		add(x);
	}
	template<class typ> bool getField(Field field, typ x) {
		if (gotoKey(field) && get(x)) {
			return true;
		}
		return false;
	}


	inline void addSrc(int src) {
		addField(SRC_INSTANCE, src);
	}
	inline bool getSrc(int& src) {
		return getField(SRC_INSTANCE,src);
	}


};

#endif /* MESSAGE_H_ */
