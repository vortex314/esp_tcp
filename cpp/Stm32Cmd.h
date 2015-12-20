/*
 * Stm32Cmd.h
 *
 *  Created on: Dec 8, 2015
 *      Author: lieven
 */

#ifndef STM32CMD_H_
#define STM32CMD_H_

#include "Cbor.h"

class Stm32Cmd: public Cbor {
public:
	IROM Stm32Cmd(uint32_t size);
	IROM virtual ~Stm32Cmd();
	typedef enum {
		CMD = 1, MSG_ID, ADDRESS, BYTES,ERC
	} Field;
	typedef enum {
		 INVALID,RESET, REQUEST, GO, READ_PAGE,GET
	} Cmd;
	IROM Stm32Cmd& cmd(Cmd cmd);
	IROM bool getCmd(Cmd& cmd);
	IROM Stm32Cmd& msgId(uint16_t id);
	IROM bool getMsgId(uint16_t& id);
	IROM Stm32Cmd& erc(uint32_t err);
	IROM bool getErc(uint32_t& err);

};

#endif /* STM32CMD_H_ */
