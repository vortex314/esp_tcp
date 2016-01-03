/*
 * Message.h
 *
 *  Created on: Dec 28, 2015
 *      Author: lieven
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_
#include <Cbor.h>
/*
 *  domain/device/in
 *  domain/device/out
 *
 */

class Message: public Cbor {
public:
	typedef enum {
		INVALID,

		DST_DOMAIN,
		DST_DEVICE,
		DST_INSTANCE,
		DST_PROP,

		SRC_DOMAIN,
		SRC_DEVICE,
		SRC_INSTANCE,
		SRC_PROP,

		MESSAGE_ID,
		TYPE, // REQUEST,RESPONE,RECORD,RELEASE,COMPLETE,ACK
		QOS,	// NO_CONF,ACK,ONCE
		ERRNO,
		PAYLOAD,

		DATA=256,
		TEXT,
		//--------------------------- UART
		UART_PORT,
		UART_BAUDRATE,
		UART_MODE,
		// -------------------------- TCP
		TCP_HOST,
		TCP_PORT
	} Field;

	Message(int size);
	virtual ~Message();
	template<class T> void addField(Field field, T x) {
		addKey((int) field);
		add(x);
	}
	template<class T> bool getField(Field field, T x) {
		if (gotoKey(field) && get(x)) {
			return true;
		}
		return false;
	}

};

#endif /* MESSAGE_H_ */
