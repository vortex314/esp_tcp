/*
 * Receiver.h
 *
 *  Created on: Jan 24, 2016
 *      Author: lieven
 */

#ifndef RECEIVER_H_
#define RECEIVER_H_
#include "Sys.h"
#include "Tcp.h"
#include "Slip.h"
#include "Handler.h"
#include "Stream.h"

class Receiver: public Handler, public Stream {
private:
	Tcp* _tcp;
	Slip* _slip;

public:
	IROM Receiver(Tcp* tcp);
	IROM void init() ;
	IROM bool dispatch(Msg& msg);
	IROM Erc send(Bytes& bytes);
	IROM virtual Erc write(uint8_t b);
	IROM virtual Erc write(uint8_t* pb, uint32_t length);
	IROM virtual Erc write(Bytes& bytes);
//	IROM  ~Stream(){};
	IROM virtual bool hasData();
	IROM virtual bool hasSpace();
	IROM virtual uint8_t read();
	IROM virtual bool isConnected();
	IROM virtual void connect();
	IROM virtual void disconnect();

};

#endif /* RECEIVER_H_ */
