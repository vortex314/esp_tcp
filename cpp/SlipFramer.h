/*
 * Receiver.h
 *
 *  Created on: Jan 24, 2016
 *      Author: lieven
 */

#ifndef SLIPFRAMER_H_
#define SLIPFRAMER_H_
#include "Sys.h"
#include "Tcp.h"
#include "Slip.h"
#include "Handler.h"
#include "Stream.h"

class SlipFramer: public Handler, public Stream {
private:
	Tcp* _tcp;
	Slip* _slip;

public:
	 SlipFramer(Tcp* tcp);
	 void init() ;
	 bool dispatch(Msg& msg);
	 Erc send(Bytes& bytes);
	  Erc write(uint8_t b);
//	 virtual Erc write(uint8_t* pb, uint32_t length);
	  Erc write(Bytes& bytes);
//	  ~Stream(){};
	  bool hasData();
	  bool hasSpace();
	  uint8_t read();
	  bool isConnected();
	  void connect();
	  void disconnect();

};

#endif /* SLIPFRAMER_H_ */
