/*
 * Cmd.h
 *
 *  Created on: Jan 26, 2016
 *      Author: lieven
 */

#ifndef CMD_H_
#define CMD_H_

#include <Handler.h>
#include <Stream.h>
#include <SlipFramer.h>




class Cmd : public Handler {
	SlipFramer* _stream;
public:
	 Cmd(SlipFramer* stream);
	 virtual ~Cmd();
	 void init() ;
	 bool dispatch(Msg& msg);
};



#endif /* CMD_H_ */
