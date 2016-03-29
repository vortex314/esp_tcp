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

#include <Json.h>
#include <Message.h>
#include <Logger.h>

IROM bool Cmd::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	init();
	while (true) {
		PT_YIELD_UNTIL(msg.is(_stream, SIG_RXD) );
		if (msg.is(_stream, SIG_RXD)) {

			Json request(0);
			msg.rewind().getMapped(request);
			LOG << " request : " << request << FLUSH;
			request.findKey("id");

			//TODO handle cmd
			Json response(200);
			response.addMap().addKey("error").add(E_OK).addBreak();
			LOG << " response : " << response << FLUSH;
			_stream->send(response);
		}
	}
PT_END()
;
return false;
}
