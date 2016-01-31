/*
 * watchdog.c
 *
 *  Created on: Oct 26, 2015
 *      Author: lieven
 */
#include "Sys.h"
#include "stdint.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "espmissingincludes.h"

#define US_TO_RTC_TIMER_TICKS(t)          \
    ((t) ?                                   \
     (((t) > 0x35A) ?                   \
      (((t)>>2) * ((APB_CLK_FREQ>>4)/250000) + ((t)&0x3) * ((APB_CLK_FREQ>>4)/1000000))  :    \
      (((t) *(APB_CLK_FREQ>>4)) / 1000000)) :    \
     0)

#define FRC1_ENABLE_TIMER  BIT7
#define FRC1_AUTO_LOAD  BIT6

//TIMER PREDIVED MODE
typedef enum {
	DIVDED_BY_1 = 0, //timer clock
	DIVDED_BY_16 = 4, //divided by 16
	DIVDED_BY_256 = 8, //divided by 256
} TIMER_PREDIVED_MODE;

typedef enum { //timer interrupt mode
	TM_LEVEL_INT = 1, // level interrupt
	TM_EDGE_INT = 0, //edge interrupt
} TIMER_INT_MODE;

typedef enum {
	FRC1_SOURCE = 0, NMI_SOURCE = 1,
} FRC1_TIMER_SOURCE_TYPE;

/******************************************************************************
 * FunctionName : hw_timer_arm
 * Description  : set a trigger timer delay for this timer.
 * Parameters   : uint32 val :
 in autoload mode
 50 ~ 0x7fffff;  for FRC1 source.
 100 ~ 0x7fffff;  for NMI source.
 in non autoload mode:
 10 ~ 0x7fffff;
 * Returns      : NONE
 *******************************************************************************/
void hw_timer_arm(u32 val) {
	RTC_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(val));
}

static void (*user_hw_timer_cb)(void) = NULL;
/******************************************************************************
 * FunctionName : hw_timer_set_func
 * Description  : set the func, when trigger timer is up.
 * Parameters   : void (* user_hw_timer_cb_set)(void):
 timer callback function,
 * Returns      : NONE
 *******************************************************************************/
void hw_timer_set_func(void (*user_hw_timer_cb_set)(void)) {
	user_hw_timer_cb = user_hw_timer_cb_set;
}

static IRAM void hw_timer_isr_cb(void) {
	if (user_hw_timer_cb != NULL) {
		(*(user_hw_timer_cb))();
	}
}

/******************************************************************************
 * FunctionName : hw_timer_init
 * Description  : initilize the hardware isr timer
 * Parameters   :
 FRC1_TIMER_SOURCE_TYPE source_type:
 FRC1_SOURCE,    timer use frc1 isr as isr source.
 NMI_SOURCE,     timer use nmi isr as isr source.
 u8 req:
 0,  not autoload,
 1,  autoload mode,
 * Returns      : NONE
 *******************************************************************************/
void hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req) {
	if (req == 1) {
		RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
				FRC1_AUTO_LOAD | DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
	} else {
		RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
				DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
	}

	if (source_type == NMI_SOURCE) {
		ETS_FRC_TIMER1_NMI_INTR_ATTACH(hw_timer_isr_cb);
	} else {
		ETS_FRC_TIMER1_INTR_ATTACH(hw_timer_isr_cb, NULL);
	}

	TM1_EDGE_INT_ENABLE();
	ETS_FRC1_INTR_ENABLE();
}

/*
 NOTE:
 1 if use nmi source, for autoload timer , the timer setting val can't be less than 100.
 2 if use nmi source, this timer has highest priority, can interrupt other isr.
 3 if use frc1 source, this timer can't interrupt other isr.

 */

uint64_t SysUpTime = 0UL;
uint64_t SysWatchDog = 1000UL;

void IRAM dump_stack(uint32_t* lv) {
	uint32_t* start = lv;
	uint32_t* end = 0x40000000;
	uint32_t* ptr = start;
	os_printf_plus("@(#):STACK_START 0x%X\n", (uint32_t)start);
	while (ptr < end) {
		if ((*ptr > 0x40000000 && *ptr < 0x60000000) // only print CODE locations
//		|| (*ptr > 0x3ff00000 && *ptr < 0x40000000) // data
		)
			os_printf_plus("@(#):%8X:%8X\n", (uint32_t)ptr, *ptr);
		ptr += sizeof(uint32_t);
	}
	os_printf_plus("@(#):STACK_END\n");
}

void IRAM hw_test_timer_cb(void) {

	uint32_t lv = 0;
	SysUpTime++;
	if (SysUpTime > SysWatchDog) {
		ets_wdt_disable();
		system_uart_de_swap();
		os_printf_plus("\nWATCHDOG\n");
		dump_stack(&lv);
		SysWatchDog = SysUpTime + 500;
		ets_wdt_enable();
	}

}

void IROM feedWatchDog() {
	SysWatchDog = SysUpTime+500;
}

void IROM initWatchDog(void) {
	hw_timer_init(FRC1_SOURCE, 1);
	hw_timer_set_func(hw_test_timer_cb);
	hw_timer_arm(1000);
	SysWatchDog = SysUpTime+1000;
}

