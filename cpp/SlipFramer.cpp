/*
 * Receiver.cpp
 *
 *  Created on: Jan 24, 2016
 *      Author: lieven
 */

#include <SlipFramer.h>

SlipFramer::SlipFramer(Tcp* tcp) :
		Handler("Receiver") {
	_tcp = tcp;
	_slip = new Slip(256);
}
void SlipFramer::init() {

}

Erc SlipFramer::write(uint8_t b) {
	return E_OK;
}
Erc SlipFramer::send(Bytes& bytes) {
	Slip* slip = (Slip*) &bytes;
	slip->encode();
	slip->frame();
	_tcp->write(*slip);
	return E_OK;
}

bool SlipFramer::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	INFO("");
	init();
	while (true) {
		PT_YIELD_UNTIL(msg.is(_tcp, SIG_RXD) );
		if (msg.is(_tcp, SIG_RXD)) {
			Bytes bytes(0);
			msg.rewind().getMapped(bytes);
			INFO("bytes received %d",bytes.length());
			while (bytes.hasData()) {
				if (_slip->fill(bytes.read())) {
					INFO("SLIP message received");
					Msg::queue().putf("uuB", this, SIG_RXD, _slip);
					_slip->clear();
				}
			}
		}
	}
PT_END()
;
return false;
}
Erc SlipFramer::write(Bytes& bytes) {
return E_OK;
}
//	  ~Stream(){};
bool SlipFramer::hasData() {
return false;
}
bool SlipFramer::hasSpace() {
return true;
}
uint8_t SlipFramer::read() {
return 0;
}
bool SlipFramer::isConnected() {
return true;
}
void SlipFramer::connect() {
return;
}
void SlipFramer::disconnect() {
return;
}

