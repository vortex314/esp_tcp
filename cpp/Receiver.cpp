/*
 * Receiver.cpp
 *
 *  Created on: Jan 24, 2016
 *      Author: lieven
 */

#include <Receiver.h>

IROM Receiver::Receiver(Tcp* tcp) :
		Handler("Receiver") {
	_tcp = tcp;
	_slip =new Slip(256);
}
IROM void Receiver::init() {

}

IROM  Erc Receiver::write(uint8_t b){
	return E_OK;
}
IROM  Erc Receiver::write(uint8_t* pb, uint32_t length){
	Stream* _upStream;
	uint32_t i;
	for(i=0;i<length;i++)
	{
		if (_slip->fill(pb[i])){
			_upStream->write(*_slip);
		}
	}
	return E_OK;
}
IROM  Erc Receiver::write(Bytes& bytes){
	return E_OK;
}
//	IROM  ~Stream(){};
IROM  bool Receiver::hasData(){
	return E_OK;
}
IROM  bool Receiver::hasSpace(){
	return E_OK;
}
IROM  uint8_t Receiver::read(){
	return E_OK;
}
IROM  bool Receiver::isConnected(){
	return true;
}
IROM  void Receiver::connect(){
	return ;
}
IROM  void Receiver::disconnect(){
	return ;
}
IROM bool Receiver::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	init();
	while (true) {
		timeout(20000);
		PT_YIELD_UNTIL(msg.is(_tcp, SIG_RXD) || timeout());
		if (msg.is(_tcp, SIG_RXD)) {
//				msg.rewind().scanf("B",&bytes);
			_tcp->write((uint8_t*) "HI", 2);
		}
	}
PT_END()
;
return false;
}
