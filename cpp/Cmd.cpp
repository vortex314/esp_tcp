/*
 * Cmd.cpp
 *
 *  Created on: Jan 26, 2016
 *      Author: lieven
 */

#include <Cmd.h>

Cmd::Cmd(SlipFramer* stream) : Handler("Cmd"){
	_stream = stream;
}

Cmd::~Cmd() {
}

void Cmd::init(){
}


#include <Message.h>

IROM bool Cmd::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	init();
	while (true) {
		PT_YIELD_UNTIL(msg.is(_stream, SIG_RXD) );
		if (msg.is(_stream, SIG_RXD)) {
			INFO(" Message received ");
			uint32_t messageId=0;
			msg.rewind();
			Message request(0);
			msg.rewind().getMapped(request);
			request.getField(Message::MESSAGE_ID, messageId);
			INFO("Processing ... ");
			//TODO handle cmd
			Message response(200);
			response.addField(Message::MESSAGE_ID, messageId);
			_stream->write(response);
		}
	}
PT_END()
;
return false;
}
