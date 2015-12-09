/*
 * Stm32Cmd.cpp
 *
 *  Created on: Dec 8, 2015
 *      Author: lieven
 */

#include <Stm32Cmd.h>

Stm32Cmd::Stm32Cmd(uint32_t size) :
		Cbor(size) {

}

Stm32Cmd::~Stm32Cmd() {

}

Stm32Cmd& Stm32Cmd::cmd(Cmd cmd) {
	addKey(CMD);
	add(cmd);
	return *this;
}

bool Stm32Cmd::getCmd(Cmd& cmd) {
	uint32_t _cmd;
	if (gotoKey(CMD) && get(_cmd)) {
		cmd = (Cmd) _cmd;
		return true;
	};
	return false;
}

Stm32Cmd& Stm32Cmd::msgId(uint16_t msgId) {
	addKey(MSG_ID);
	add(msgId);
	return *this;
}

bool Stm32Cmd::getMsgId(uint16_t& id) {
	uint32_t _id;
	if (gotoKey(MSG_ID) && get(_id)) {
		id = _id;
		return true;
	};
	return false;
}

Stm32Cmd& Stm32Cmd::erc(uint32_t err) {
	addKey(ERC);
	add(err);
	return *this;
}

bool Stm32Cmd::getErc(uint32_t& err) {
	uint32_t _err;
	if (gotoKey(ERC) && get(_err)) {
		err = _err;
		return true;
	};
	return false;
}

