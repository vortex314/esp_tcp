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
/*
 *
> © DecaWave the DW1000
> -  The octets are physically presented on the SPI interface data lines with the high order bit sent first in time.
> - The octets of a multi-octet value are transferred on the SPI interface in octet order beginning with the low-order octet.
void spi_tx_byte_order(uint8 spi_no,
SPI_BYTE_ORDER_LOW_TO_HIGH (0))
spi_rx_byte_order(HSPI,SPI_BYTE_ORDER_HIGH_TO_LOW)

> - PHA , POL : 0,0
> - POL=0 : clock idle is high
> - PHA=0 : data is available at rising clock edge ( when pol=0, otherwise at falling edge )
// Function Name: spi_mode
	// Description: Configures SPI mode parameters for clock edge and clock polarity.
	// Parameters: spi_no - SPI (0) or HSPI (1)
	// spi_cpha - (0) Data is valid on clock leading edge
	// (1) Data is valid on clock trailing edge
	// spi_cpol - (0) Clock is low when inactive
	// (1) Clock is high when inactive
	//
	////////////////////////////////////////////////////////////////////////////////

	void spi_mode(HSPI, 0,1){
 */

#include "spi.h"
#include <gpio_c.h>
void spiTest() {
	INFO("HSPI");

	int pin = 5;	// RESET PIN
	pinMode(pin, 1); // OUTPUT
	digitalWrite(pin, 0); // PULL LOW
	os_delay_us(10000);	// 10ms
	digitalWrite(pin, 1); // PUT HIGH

	spi_init(HSPI);
	spi_mode(HSPI, 0, 1);
//	spi_clock(HSPI, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
	spi_clock(HSPI, 20, 40); //
	spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
	spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);

	uint32_t rxd;
	int i, order;
	for (order = 0; order < 2; order++) {
		for (i = 0; i < 2; i++) {
			spi_set_bit_order(order);
			rxd = spi_transaction(HSPI, 0, 0, 0, 0, 8, 0, 32, 0);
			INFO("RXD SPI : %X", rxd);
			rxd = spi_transaction(HSPI, 0, 0, 0, 0, 8, 0x1D, 32, 0);
			INFO("RXD SPI : %X", rxd);

			os_delay_us(1000);
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
	os_delay_us(100000);

	INFO("*****************************************");
	INFO("Starting version : " __DATE__ " " __TIME__);
	INFO("*****************************************");
	spiTest();
	system_init_done_cb(MsgInit);
}

