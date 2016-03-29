/*
 * Sys.cpp
 *
 *  Created on: Sep 13, 2015
 *      Author: lieven
 */
#include "errno.h"
#include "string.h"
#include "Sys.h"

#ifdef __ESP8266__
#ifdef __cplusplus
extern "C" {
#endif

#include "user_interface.h"
#include "espmissingincludes.h"
#include "osapi.h"
#include "mem.h"
#include "util.h"

extern uint64_t SysUpTime;
#include "mutex.h"
mutex_t mallocMutex=1;
#include "Sys.h"
#include <Logger.h>


IRAM void* malloc(size_t size) {
	while(!GetMutex(&mallocMutex));
	void* pv = pvPortMalloc(size);
	ASSERT(pv);
	ReleaseMutex(&mallocMutex);
	return pv;
}

IRAM void free(void* ptr) {
//	INFO("free(0x%X)", ptr);
	vPortFree(ptr);
}


IRAM uint64_t SysMillis() {
	return SysUpTime;
}

IRAM uint64_t Sys::millis() {
	return SysMillis();
}

void Sys::delayUs(uint32_t delay){
	os_delay_us(delay);
}

#include "stdarg.h"


#ifdef __cplusplus
}
#endif

#endif

