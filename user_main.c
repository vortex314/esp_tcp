/* main.c -- MQTT client example
 *
 * Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * * Neither the name of Redis nor the names of its contributors may be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Lieven : some thoughts
 * - Cllback routines can be invoked from interrupts and as such can forget to feed the watchdog
 * - timers are also likely to be in this case
 * - SPI flash loading can influence timing
 *
 */
#include "stdint.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "gpio16.h"
#include "Sys.h"
#include "util.h"

extern void MsgInit();

extern void initWatchDog(void);
extern void initExceptionHandler();
extern void feedWatchDog(void);

#include "spi.h"
void spiTest() {
	INFO("HSPI");
	spi_init_gpio(HSPI, 1);
	spi_mode(HSPI, 0, 0);
	spi_clock(HSPI, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
	spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
	spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
	/*	Command = 0b101 (3 bit write command)

	 Address = 0b111110011 or 0x1F3 (9 bit data address)

	 Data = 0b11001100 or 0xCC (8 bits of data)

	 SPI Transaction Packet = 0b10111111001111001100 or 0xBF3CC*/

	uint32_t rxd; //= spi_transaction(HSPI, 3, 0b101, 9, 0x1F3, 8, 0xCC, 32,0);
	int i, pha, pcol;
	for (i = 0; i < 5; i++) {
		for (pha = 0; pha < 2; pha++) {
			for (pcol = 0; pcol < 2; pcol++) {
				spi_mode(HSPI,pha,pcol);
				rxd  = spi_transaction(HSPI,8,0,0,0,0,0,32,0);
//				spi_tx8(HSPI, 0);
//				rxd = spi_rx32(HSPI);
				INFO("RXD SPI : %X", rxd);
			}
		}
	}
}

void user_init(void) {

	ThreadLockInit();
	SysLogInit();

	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	uart_config(0, 115200, "8E1");

	gpio_init();
	initWatchDog();
	initExceptionHandler();
	os_delay_us(1000000);

	INFO("*****************************************");
	INFO("Starting version : " __DATE__ " " __TIME__);
	INFO("*****************************************");
	spiTest();
	system_init_done_cb(MsgInit);
}

