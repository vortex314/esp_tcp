/*
 * Flash.cpp
 *
 *  Created on: Oct 1, 2015
 *      Author: lieven
 *      xtensa-lx106-elf-g++ -nostartfiles -nodefaultlibs -nostdlib -L"/home/lieven/workspace/Common/Debug" -L/home/lieven/esp_iot_sdk_v1.4.0/lib -Lgcc -u call_user_start -Wl,-static -T../ld/link.ld  -Wl,--gc-sections -mlongcalls -Xlinker -L/home/lieven/esp-open-sdk/sdk/lib -Xlinker -lssl -lmain -lhal -lphy -lpp -lnet80211 -llwip -lwpa -lssl -lmain -lc -Xlinker --gc-sections -Xlinker --no-undefined -o "esp_cbor"  ./wifi/Tcp.o ./wifi/Wifi.o  ./mqtt/Mqtt.o ./mqtt/MqttFramer.o ./mqtt/MqttMsg.o ./mqtt/Topic.o  ./cpp/Flash.o ./cpp/Gpio.o ./cpp/LedBlink.o ./cpp/Pump.o ./cpp/Stm32.o ./cpp/Sys.o ./cpp/Topics.o ./cpp/UartEsp8266.o ./cpp/stubs.o  ./config.o ./gpio.o ./gpio16.o ./mutex.o ./uart.o ./user_main.o ./util.o ./utils.o ./watchdog.o   -lCommon -lm -lssl -llwip -lwpa -lnet80211 -lphy -lpp -lmain -lc -lhal -lgcc
 *
 *-nostartfiles -nodefaultlibs -nostdlib -L"/home/lieven/workspace/Common/Debug"
 *-nostartfiles -L/home/lieven/esp_iot_sdk_v1.4.0/lib -Lgcc -u call_user_start
 *-nostartfiles -Wl,-static -T../ld/link.ld  -Wl,--gc-sections -mlongcalls -Xlinker
 *-nostartfiles -L/home/lieven/esp-open-sdk/sdk/lib -Xlinker -lssl -lmain -lhal -lphy -lpp -lnet80211 -llwip -lwpa -lssl -lmain
 *-nostartfiles -lc -Xlinker --gc-sections -Xlinker --no-undefined
 *
 PAGE LAYOUT :
 <MAGIC:32><SEQUENCE;32><itemIndex:16+length:16><data..length rounded 4 ><itemIndex+length><data:n><0xFFFFFFFF>
 item idx==even : key
 item idx+1==uneven : value of idx+1
 Bin
 Download address
 mem size					512KB 		1024KB 		2048KB 		4096KB
 esp_init_data_default.bin 	0x7C000 	0xFC000 	0x1FC000 	0x3FC000
 blank.bin 					0x7E000 	0xFE000 	0x1FE000 	0x3FE000
 boot.bin 					0x00000 	0x00000 	0x00000 	0x00000

 */

extern "C" {
#include "espmissingincludes.h"
#include "esp8266.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "mqtt_msg.h"
#include "debug.h"
#include "user_config.h"
#include "mqtt.h"
#include "queue.h"
#include "ets_sys.h"
}
#include "stdint.h"
#include "Flash.h"
#include "Sys.h"

uint32_t roundQuad(uint32_t value) {
	if (value & 0x3)
		return (value & 0xFFFFFFFC) + 0x4;
	else
		return value;
}

Flash::Flash() {
	_keyMax = 0;
	_freePos = 0;
	_pageIdx = 0;
	_sequence = 0;
}

Flash::~Flash() {
}

void Flash::init() {
	findOrCreateActivePage();
	INFO("page Index : %d ", _pageIdx);
	INFO("free Pos  : %d", _freePos);
	INFO("sequence   : %d", _sequence);
}

Erc Flash::read(uint32_t addr, uint32_t* data) { // read QUad at quad boundary and cache
	static uint32_t w;			// last word read
	static uint32_t lastAddr = 0;	// last address where word was read
	if (lastAddr != addr) {
		if (spi_flash_read(addr, &w, 4) != SPI_FLASH_RESULT_OK)
			return EINVAL;
		lastAddr = addr;
	}
	*data = w;
//	INFO("@ 0x%X : 0x%X", addr, w);

	return E_OK;
}

Erc Flash::read(uint32_t address, uint8_t* pb) {
	Quad W;
	Erc erc;
	if ((erc = read(address & 0xFFFFFFFC, &W.w)) != E_OK)
		return erc; // take start int32
	*pb = W.b[address & 0x03];
	return E_OK;
}

Erc Flash::write(uint32_t address, uint32_t w) {
//	INFO("@ 0x%X : 0x%X", address, w);
	if (spi_flash_write(address, &w, 4) != SPI_FLASH_RESULT_OK)
		return EINVAL;
	return E_OK;
}

class Item {
	uint8_t _pageIndex;
	uint16_t _offset;	// points to start of item (index,length,data )
	uint16_t _length;
	uint16_t _itemIndex;
public:
	Item(uint8_t pageIndex, uint16_t offset, uint16_t length,uint16_t index) {
		_pageIndex = pageIndex;
		_offset = offset;
		_length = length;
		_itemIndex = index;
	}

	Item(uint8_t pageIndex) {
		_pageIndex = pageIndex;
		_offset = 8;
		_length = 0;
		_itemIndex = 0;
		load();
	}

	uint16_t getIndex() {
		return _itemIndex;
	}

	uint16_t getOffset() {
		return _offset;
	}

	uint16_t getLength() {
		return _length;
	}

	Erc read(uint8_t* data, uint32_t* maxLength) {
		Erc erc;
		if (erc = load())
			return erc;

		uint32_t length = _length < *maxLength ? _length : *maxLength;
		for (uint32_t i = 0; i < length; i++) {
			erc = Flash::read(Flash::pageAddress(_pageIndex) + _offset + 4 + i,
					&data[i]);
			if (erc)
				return erc;
		}
		*maxLength = length;
		return true;
	}

	bool firstOnPage(uint8_t pageIndex) {
		_offset = 8;
		_pageIndex = pageIndex;
		return load();
	}

	Erc findLatestItem(uint16_t index) {
		Item item(_pageIndex);
		uint32_t offset = 0;
		for (; !item.isLast(); item.next()) {
			item.load();
			if (item.getIndex() == index)
				offset = item.getOffset();
		}
		if (offset == 0)
			return ENODATA;
		_offset = offset;
		_itemIndex = index;
		load();
		return E_OK;
	}

	Erc load() {
		Quad temp;
		Erc erc = Flash::read(Flash::pageAddress(_pageIndex) + _offset,
				&temp.w);
		if (erc)
			return erc;

		_itemIndex = temp.index;
		_length = temp.length;
		if (temp.w == 0xFFFFFFFF)
			return ENODATA;
		return E_OK;
	}
	Erc next() {
		_offset = _offset + roundQuad(_length) + 4;
		return E_OK;
	}
	bool isLast() {
		return _length == 0xFFFF;
	}
	bool write(uint8_t* data, uint16_t length) {
		uint32_t address = Flash::pageAddress(_pageIndex) + _offset;
		Quad W;
		W.index = _itemIndex;
		W.length = length;
		//		INFO(" address 0x%x length %d",address,length);
		Erc erc = Flash::write(address, W.w);
		if (erc)
			return erc;
		for (uint16_t i = 0; i < length + 3; i += 4) {
			W.w = 0xFFFFFFFF;
			for (uint32_t j = 0; i + j < length && j < 4; j++)
				W.b[j] = data[i + j];
			erc = Flash::write(address + 4 + i, W.w);
			if (erc)
				return erc;
		}
		return true;
	}
}
;

bool Flash::set(const char* key, const char*s) {
	int idx = findOrCreateKey(key);
	uint32_t length=strlen(s);
	Item item(_pageIdx,_freePos,length,idx+1);
	item.write((uint8_t*)s,length);
	_freePos += 4 + roundQuad(length);
	return true;
}

void Flash::get(int& value, const char* key, int dflt) {
	int idx = findKey(key);
	Item item(_pageIdx);
	uint32_t length = 4;
	if (idx >= 0) {
		Erc erc = item.findLatestItem(idx + 1);
		if (erc == E_OK) {
			item.read((uint8_t*) &value, &length);
			return;
		}
	} else
		value = dflt;
}

Erc Flash::get(char* value, int maxLength, uint32_t index) {
	Item item(_pageIdx);
	uint32_t len = maxLength;
	Erc erc = item.findLatestItem(index);
	if (erc)
		return erc;
	item.read((uint8_t*) value, &len);
	value[len] = '\0';
	return E_OK;
}

Erc Flash::get(char* value, int length, const char* key, const char* dflt) {
	int idx = findKey(key);
	Item item(_pageIdx);
	uint32_t len = length;
	if (idx >= 0) {
		return get(value, length, idx + 1);
	} else {
		os_strncpy(value, dflt, length);
	}
	return E_OK;
}

bool Flash::isValidPage(uint32_t pageIdx, uint32_t& sequence) {
	uint32_t magic;
	uint32_t index = 0;
	Erc erc = read(pageAddress(pageIdx), &magic);
	if (erc)
		return false;
	if (magic != PAGE_SIGNATURE) {
		return false;
	}
	erc = read(pageAddress(pageIdx) + 4, &index);
	if (erc)
		return false;
	sequence = index;
	return true;
}

uint16_t Flash::findFreeBegin() {

	Item item(_pageIdx, 8, 0,0);
	while (true) {
		item.load();
		if (item.isLast())
			break;
		if ((item.getIndex() > _keyMax) && (item.getIndex() & 1) == 0) // even and bigger then keyMax
			_keyMax = item.getIndex();
		if (item.getOffset() > PAGE_SIZE)
			return EINVAL;
		item.next();
	}
	_freePos = item.getOffset();
	return _freePos;
}

int Flash::findOrCreateKey(const char*s) {
	int idx = findKey(s);
	if (idx < 0) {
		uint32_t length = strlen(s);
		_keyMax += 2;
		Item item(_pageIdx,_freePos,length,_keyMax);
		item.write((uint8_t*)s,length);
		_freePos += 4 + roundQuad(length);
		return _keyMax;
	}
	return idx;
}

int Flash::findKey(const char*s) {

	uint16_t strLen = os_strlen(s);
	char* szKey[40];
	Item item(_pageIdx, 8, 0,0); // after signature and sequence
	for (; !item.isLast(); item.next()) {
		item.load();
		if (item.getLength() == strLen) {
			uint32_t length = sizeof(szKey);
			item.read((uint8_t*) szKey, &length);
			szKey[length] = '\0';
			if ( os_strncmp((const char*) szKey, s, (int) length) == 0)
				return item.getIndex();
		}

		if (item.getOffset() > PAGE_SIZE)
			return -1;
	}
	return -1;
}

void Flash::findOrCreateActivePage() {
	_sequence = 0;
	uint32_t sequence;

	for (uint32_t i = 0; i < PAGE_COUNT; i++) {
		if (isValidPage(i, sequence) && sequence > _sequence) {
			_pageIdx = i;
			_sequence = sequence;
		}
	}
	if (_sequence == 0) {
		initializePage(0, 1);
		_pageIdx = 0;
		_sequence = 1;
	}
	findFreeBegin();
}
#define FIRST "FIRST"

bool Flash::initializePage(uint32_t pageIdx, uint32_t sequence) {
//		INFO(" pageIdx : %d sequence : %d ",pageIdx,sequence);
	uint32_t sector = PAGE_START / PAGE_SIZE + pageIdx;
	if (spi_flash_erase_sector(sector) != SPI_FLASH_RESULT_OK)
		return false;

	Erc erc = write(pageAddress(pageIdx), PAGE_SIGNATURE);
	if (erc)
		return false;
	erc = write(pageAddress(pageIdx), sequence);
	if (erc)
		return false;
	return true;
}

uint32_t Flash::pageAddress(uint32_t pageIdx) {
	return PAGE_START + (pageIdx * PAGE_SIZE);
}

uint32_t Flash::nextPage(uint32_t pageIdx) {
	return (pageIdx + 1) % PAGE_COUNT;
}
/*
bool Flash::writeItem(uint16_t index, uint8_t* start, uint32_t length) {
//		INFO(" index : %u , length : %u ",index,length);
	uint32_t address = pageAddress(_pageIdx) + _freePos;
	Quad W;
	W.index = index;
	W.length = length;
//		INFO(" address 0x%x length %d",address,length);
	if (write(address, W.w) != E_OK)
		return false;
	for (uint32_t i = 0; i < length + 3; i += 4) {roundQuad
		W.w = 0xFFFFFFFF;
		for (uint32_t j = 0; i + j < length && j < 4; j++)
			W.b[j] = start[i + j];
		if (write(address + 4 + i, W.w) != E_OK)
			return false;
	}
	_freePos += 4 + roundQuad(length);
	return true;
}*/
/*
 bool Flash::loadItem(uint16_t offset, uint8_t* start, uint16_t& length) {
 //				INFO("offset 0x%X maxLength %u",offset,length);
 Quad temp;
 temp.w = flashReadQuad(offset);
 if (temp.index == 0xFFFF)
 return false;
 length = temp.length < length ? temp.length : length;
 for (uint32_t i = 0; i < length; i++) {
 start[i] = flashReadByte(offset + 4 + i);
 //					INFO(" readByte : 0x%x",start[i]);
 }
 //		INFO("offset 0x%X length %u",offset,length);
 return true;
 }*/

uint16_t Flash::findItem(uint16_t idx) {
	Quad temp;
	uint16_t offset = 8; // after signature and sequence
	uint16_t lastOffset = 0;
	while (true) {
		if (read(pageAddress(_pageIdx) + offset, &temp.w) != E_OK)
			return 0;
		if (temp.w == 0xFFFFFFFF)
			break;
		if (temp.index == idx) {
			lastOffset = offset;
		}
		offset = offset + roundQuad(temp.length) + 4;
		if (offset > PAGE_SIZE)
			break;
	}
	return lastOffset;
}

