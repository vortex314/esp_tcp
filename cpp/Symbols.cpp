/*
 * Symbols.cpp
 *
 *  Created on: Jan 11, 2016
 *      Author: lieven
 */
#include <stdint.h>
#include <Sys.h>
enum Instance {
	UART0, UART1, SPI0, SPI1, TCP0, I2C0, WIFI, STM32

};

typedef struct {
	const char* name;
	const uint32_t value;
} Symbol;

Symbol symbol[] = { { "SPI1", SPI1 }, { "UART1", UART1 } };

class Symbols {
public:
	Symbol** _symbol;
	int _count;
public:
	IROM Symbols(Symbol** symbol, uint32_t count) {
		_symbol = symbol;
		_count = count;
	}
	int IROM findValueByname(const char* s);
	const char* findNameByValue(uint32_t value);
};
#include "string.h"
int Symbols::findValueByname(const char* s) {
	for (int i = 0; i < _count; i++)
		if (strcmp(symbol[i].name, s) == 0)
			return symbol[i].value;
	return -1;
}

const char* Symbols::findNameByValue(uint32_t value) {
	for (int i = 0; i < _count; i++)
		if (symbol[i].value == value)
			return symbol[i].name;
	return 0;
}

enum {
	PUT, GET, HEAD, POST
};

Symbol cmds[] = { { "PUT", PUT }, { "GET", GET }, { "HEAD", HEAD } };

void testSymbols() {
/*	Symbols cmd(cmds, sizeof(cmds) / sizeof(Symbol));
	cmd.findValueByname("GET");
	cmd.findNameByValue(PUT);*/
}

