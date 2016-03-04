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

bool Flash::set(const char* key, const char*s) {
	int idx = findOrCreateKey(key);
	writeItem(idx + 1, (uint8_t*) s, strlen(s));
	return true;
}

bool Flash::set(const char* key, int value) {
	int idx = findOrCreateKey(key);
	writeItem(idx + 1, (uint8_t*) value, 4);
	return true;
}

void Flash::get(int& value, const char* key, int dflt) {
	int idx = findKey(key);
	uint16_t length = 4;
	if (idx >= 0)
		loadItem(idx + 1, (uint8_t*) &value, length);
	else
		value = dflt;
}

void Flash::get(char* value, int length, const char* key, const char* dflt) {
	int idx = findKey(key);
	uint16_t len = 4;
	if (idx >= 0) {
		if (!loadItem(idx + 1, (uint8_t*) value, len)) {
			os_strncpy(value, dflt, length);
		} else {
			value[len] = '\0';
		}
	} else
		os_strncpy(value, dflt, length);
}

void Flash::init() {
//		INFO("");
	findOrCreateActivePage();
	INFO("page Index : %d ", _pageIdx);
	INFO("free Pos  : %d", _freePos);
	INFO("sequence   : %d", _sequence);

//		writeItem(13,(uint8_t*)"iot.eclipse.org",os_strlen("iot.eclipse.org"));
//		INFO(" findKey ( 'mqtt/host') : %d ",findKey("mqtt/host"));
}

bool Flash::isValidPage(uint32_t pageIdx, uint32_t& sequence) {
	uint32_t magic;
	uint32_t index = 0;
	if (spi_flash_read(pageAddress(pageIdx), &magic, sizeof(magic))
			!= SPI_FLASH_RESULT_OK) {
		return false;
	}
	if (magic != PAGE_SIGNATURE) {
		return false;
	}
	if (spi_flash_read(pageAddress(pageIdx) + 4, &index, sizeof(index))
			!= SPI_FLASH_RESULT_OK) {
		return false;
	}
	sequence = index;
	return true;
}

uint16_t Flash::findFreeBegin() {
	Quad temp;
	uint16_t offset = 8; // after signature and sequence
	while (true) {
		if (spi_flash_read(pageAddress(_pageIdx) + offset, &temp.w, 4)
				!= SPI_FLASH_RESULT_OK)
			return 0;
		if (temp.w == 0xFFFFFFFF)
			break;
		if (((temp.index & 1) == 0) && (temp.index > _keyMax))
			_keyMax = temp.index;
		offset = offset + roundQuad(temp.length) + 4;
		if (offset > PAGE_SIZE)
			return 0;
	}
	_freePos = offset;
	return offset;
}

int Flash::findOrCreateKey(const char*s) {
	int idx = findKey(s);
	if (idx < 0) {
		_keyMax += 2;
		writeItem(_keyMax, (uint8_t*) s, strlen(s));
		return _keyMax;
	}
	return idx;
}

int Flash::findKey(const char*s) {
	Quad temp;
	uint16_t strLen = os_strlen(s);
	char* szKey[40];
	uint16_t offset = 8; // after signature and sequence
	while (true) {
		temp.w = flashReadQuad(offset);
		INFO(" temp.index %u temp.length %u", temp.index, temp.length);
		if (temp.index == 0xFFFF)
			return -1;
		if (temp.length == strLen) {
			uint16_t length = 40;
			loadItem(offset, (uint8_t*) szKey, length);
			szKey[length] = '\0';
			INFO(" strcmp %s : %s : %u ", szKey, s, length);
			if ( os_strncmp((const char*) szKey, s, (int) length) == 0)
				return temp.index;

		}

		offset = offset + roundQuad(temp.length) + 4;
		if (offset > PAGE_SIZE)
			return -1;
	}
	return -1;
}

void Flash::findOrCreateActivePage() {
//		INFO("");
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
//		INFO(" pageIdx : %d freePos : %u ",_pageIdx,_freePos);
	/*		writeItem(2,(uint8_t*)"mqtt/host",10);
	 writeItem(4,(uint8_t*)"mqtt/port",10);*/
}
#define FIRST "FIRST"

bool Flash::initializePage(uint32_t pageIdx, uint32_t sequence) {
//		INFO(" pageIdx : %d sequence : %d ",pageIdx,sequence);
	uint32_t sector = PAGE_START / PAGE_SIZE + pageIdx;
	if (spi_flash_erase_sector(sector) != SPI_FLASH_RESULT_OK)
		return false;
	uint32_t magic = PAGE_SIGNATURE;
	if (spi_flash_write(pageAddress(pageIdx), &magic, sizeof(magic))
			!= SPI_FLASH_RESULT_OK)
		return false;
	if (spi_flash_write(pageAddress(pageIdx) + 4, &sequence, sizeof(sequence))
			!= SPI_FLASH_RESULT_OK)
		return false;
	return true;
}

uint32_t Flash::pageAddress(uint32_t pageIdx) {
	return PAGE_START + (pageIdx * PAGE_SIZE);
}

uint32_t Flash::nextPage(uint32_t pageIdx) {
	return (pageIdx + 1) % PAGE_COUNT;
}

bool Flash::writeItem(uint16_t index, uint8_t* start, uint32_t length) {
//		INFO(" index : %u , length : %u ",index,length);
	uint32_t address = pageAddress(_pageIdx) + _freePos;
	Quad W;
	W.index = index;
	W.length = length;
//		INFO(" address 0x%x length %d",address,length);
	if (spi_flash_write(address, &W.w, 4) != SPI_FLASH_RESULT_OK)
		return false;
	for (uint32_t i = 0; i < length + 3; i += 4) {
		W.w = 0xFFFFFFFF;
		for (uint32_t j = 0; i + j < length && j < 4; j++)
			W.b[j] = start[i + j];
		if (spi_flash_write(address + 4 + i, &W.w, 4) != SPI_FLASH_RESULT_OK)
			return false;
	}
	_freePos += 4 + roundQuad(length);
	return true;
}

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
}

uint16_t Flash::findItem(uint16_t idx) {
	Quad temp;
	uint16_t offset = 8; // after signature and sequence
	uint16_t lastOffset = 0;
	while (true) {
		if (spi_flash_read(pageAddress(_pageIdx) + offset, &temp.w, 4)
				!= SPI_FLASH_RESULT_OK)
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

/*	bool Flash::scanPage(uint32_t pageIdx) {
 Quad item;
 _lastPos=8;
 uint16_t offset=8;
 uint32_t temp;
 while ( offset < PAGE_SIZE ) {
 spi_flash_read(pageAddress(_pageIdx)+item.offset,&temp,4);
 if ( temp == 0xFFFFFFFF ) break;
 item.length = temp & 0xFFFF;
 item.index = temp >> 16;
 item.offset += 4 + item.length;
 }

 return true;
 }*/
/*
 <magic><sequence><idx+len><data..len mod 4 ><break>
 idx==even : key
 idx+1==uneven : value of idx+1
 */

#define CACHE_SIZE	16
uint32_t Flash::flashReadQuad(uint32_t location) { // read QUad at quad boundary and cache
	static uint32_t w;
	static uint32_t lastLocation = 0;
	if (lastLocation != location) {
		spi_flash_read(pageAddress(_pageIdx) + location, &w, 4);
		lastLocation = location;
	}
	return w;
}

int Flash::flashReadByte(uint32_t location) {
	Quad W;
	W.w = flashReadQuad(location & 0xFFFFFFFC); // take start int32
	return W.b[location & 0x03];
}
/*
 * Bin
 Download address
 512KB 1024KB
 2048KB 4096KB
 512KB+512KB 1024KB
 +1024KB 512KB+512KB 1024KB
 +1024KB
 esp_init_data_default.b
 in 0x7C000 0xFC000 0x1FC000 0x1FC000 0x3FC000 0x3FC000
 blank.bin 0x7E000 0xFE000 0x1FE000 0x1FE000 0x3FE000 0x3FE000
 boot.bin 0x00000 0x00000 0x00000 0x00000 0x00000 0x00000
 */

