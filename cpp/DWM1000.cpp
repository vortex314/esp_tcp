/*
 * DWM1000.cpp
 *
 *  Created on: Feb 12, 2016
 *      Author: lieven
 */

#include <DWM1000.h>

extern "C" {
#include <spi.h>
#include <gpio_c.h>
#include <espmissingincludes.h>
#include <osapi.h>
}
;

DWM1000::DWM1000() :
		Handler("DWM1000") {
	_count=0;
}

DWM1000::~DWM1000() {

}

void DWM1000::init() {
	INFO("HSPI");

	int pin = 5;	// RESET PIN == D1 == GPIO5
	pinMode(pin, 1); // OUTPUT
	digitalWrite(pin, 0); // PULL LOW
	os_delay_us(10000);	// 10ms
	digitalWrite(pin, 1); // PUT HIGH

	spi_init(HSPI);
	spi_mode(HSPI, 0, 0);
	//	spi_clock(HSPI, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
	spi_clock(HSPI, 20, 40); //
	spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
	spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
	spi_set_bit_order(0);

	_count = 0;
}

uint32_t readDW1000(uint32_t offset, uint32_t length, uint32_t dummy) {
	return spi_transaction(HSPI, 8,offset, 0,0,0,0, length, 32-length);

}

void DWM1000::mode(uint32_t m) {
	WRITE_PERI_REG(SPI_CTRL2(HSPI),
			(( m &SPI_CS_DELAY_MODE)<<SPI_CS_DELAY_MODE_S) | //
			(1<<SPI_MOSI_DELAY_MODE_S) | //
			((0xF & SPI_CS_DELAY_NUM) << SPI_CS_DELAY_NUM_S) |//
			((0xF & SPI_MOSI_DELAY_NUM) << SPI_MOSI_DELAY_NUM_S) |//
			((0xF & SPI_SETUP_TIME) << SPI_SETUP_TIME_S) |//
			((0xF & SPI_MISO_DELAY_NUM) << SPI_MISO_DELAY_NUM_S));
}

bool DWM1000::dispatch(Msg& msg) {
	uint32_t rxd;
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	init();
	while (true) {
		timeout(100);
		PT_YIELD_UNTIL(timeout());
//		spi_set_hw_cs(false);
		uint32_t i, j;
		for (i = 0; i < 32; i++) {
//			mode(j);
//			for (j = 0; j < 3; j++) {
//				spi_mode(HSPI, j >> 1, j & 1);
//				spi_cs_deselect();
				rxd = readDW1000(0, i, 0);
//				spi_cs_deselect();
				INFO(" 0x%X : %X",i, rxd);
				os_delay_us(100);
				rxd=0;
//			}
		}

	}
PT_END()
;
return false;
}