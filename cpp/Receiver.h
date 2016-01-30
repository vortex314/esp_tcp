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
	 Receiver(Tcp* tcp);
	 void init() ;
	 bool dispatch(Msg& msg);
	 Erc send(Bytes& bytes);
	 virtual Erc write(uint8_t b);
//	 virtual Erc write(uint8_t* pb, uint32_t length);
	 virtual Erc write(Bytes& bytes);
//	  ~Stream(){};
	 virtual bool hasData();
	 virtual bool hasSpace();
	 virtual uint8_t read();
	 virtual bool isConnected();
	 virtual void connect();
	 virtual void disconnect();

};

#endif /* RECEIVER_H_ */
