/*
 * Receiver.cpp
 *
 *  Created on: Jan 24, 2016
 *      Author: lieven
 */

#include <Receiver.h>

 Receiver::Receiver(Tcp* tcp) :
		Handler("Receiver") {
	_tcp = tcp;
	_slip = new Slip(256);
}
 void Receiver::init() {

}

 Erc Receiver::write(uint8_t b) {
	return E_OK;
}
 Erc Receiver::send(Bytes& bytes) {
	Slip* slip = (Slip*)&bytes;
	slip->encode();
	slip->frame();
	_tcp->write(*slip);
	return E_OK;
}
 Erc Receiver::write(Bytes& bytes) {
	return E_OK;
}
//	  ~Stream(){};
 bool Receiver::hasData() {
	return false;
}
 bool Receiver::hasSpace() {
	return true;
}
 uint8_t Receiver::read() {
	return 0;
}
 bool Receiver::isConnected() {
	return true;
}
 void Receiver::connect() {
	return;
}
 void Receiver::disconnect() {
	return;
}
 bool Receiver::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	INFO("");
	init();
	while (true) {
		timeout(20000);

		PT_YIELD_UNTIL(msg.is(_tcp, SIG_RXD) || timeout());
		if (msg.is(_tcp, SIG_RXD)) {
			Bytes bytes(0);
			msg.rewind().getMapped(bytes);
			while( bytes.hasData()) {
				if ( _slip->fill(bytes.read())) {
					Msg::queue().putf("uuB",this,SIG_RXD,_slip);
					_slip->clear();
				}
			}
		}
	}
PT_END()
;
return false;
}
