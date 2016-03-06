/*
 * Flash.h
 *
 *  Created on: Oct 1, 2015
 *      Author: lieven
 */

#ifndef FLASH_H_
#define FLASH_H_
#include <Sys.h>
#include <Erc.h>

#include "Config.h"

#define PAGE_SIGNATURE 0xDEADBEEF
#define PAGE_SIZE 0x1000	// 16 * 256 = 4K

#ifdef K512
#define PAGE_START	0x78000
#define PAGE_COUNT 4		// 78000,79000,7A000,7B000
#endif

#define M4
#ifdef M4
#define PAGE_START	0x3F8000
#define PAGE_COUNT 4		// 3F8000,3F9000,3FA000,3FB000
#endif

typedef struct {
	union {
		uint32_t w;
		struct {
			uint16_t index;
			uint16_t length;
		};
		uint8_t b[4];
	};
} Quad;

class Flash: public Config {
private:
	uint32_t _pageIdx;
	uint32_t _sequence;
	uint32_t _freePos;
	uint16_t _keyMax;

	void findOrCreateActivePage();
	bool initializePage(uint32_t pageIdx, uint32_t sequence);
	bool scanPage(uint32_t pageIdx);

	bool isValidPage(uint32_t pageIdx, uint32_t& sequence);
	uint32_t nextPage(uint32_t pageIdx);
	bool loadItem(uint16_t& offset, uint16_t& index, uint16_t& length);
	uint16_t findItem(uint16_t index);
	bool writeItem(uint16_t sequence, uint8_t* start, uint32_t length);
	uint16_t findFreeBegin();

	int findKey(const char*s);
	int findOrCreateKey(const char*s);

	bool loadItem(uint16_t offset, uint8_t* start, uint16_t& length);

public:
	Flash();
	~Flash();
	void init();
	static Erc read(uint32_t address, uint8_t* dest);
	static Erc read(uint32_t address, uint32_t* dest);
	static Erc write(uint32_t address, uint32_t w);
	bool set(const char* key, const char*s);
	void get(int& value, const char* key, int dflt);
	Erc get(char* value, int length, const char* key, const char* dflt);
	Erc get(char* value, int maxLength,uint32_t index);
	static uint32_t pageAddress(uint32_t pageIdx);
};

#endif /* FLASH_H_ */
