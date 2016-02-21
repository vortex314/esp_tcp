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
	_count = 0;
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
	spi_clock(HSPI, 10, 10); //
//	spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
//	spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
	spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
	spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
	spi_set_bit_order(0);
	union {
		uint32_t u32;
		uint8_t ui8[4];
	} v;
	v.u32=0x12345678;
	INFO("ENDIANNESS %X %x %x %x %x ",v.u32,v.ui8[0],v.ui8[1],v.ui8[2],v.ui8[3]);


	_count = 0;
}

uint32_t readDW1000(uint32_t offset, uint32_t length) {
	return spi_transaction(HSPI, 0, 0, 0,0, 8, offset, length, 0);

}

uint32_t writeDW1000(uint32_t offset, uint32_t length, uint32_t value) {
	return spi_transaction(HSPI, 0, 0, 8, offset, length, value, 0, 0);

}

void DWM1000::mode(uint32_t m) {
	WRITE_PERI_REG(SPI_CTRL2(HSPI),0xFFFFFFFF);

	/*
	 * #define SPI_CS_DELAY_NUM 0x0000000F
#define SPI_CS_DELAY_NUM_S 28
#define SPI_CS_DELAY_MODE 0x00000003
#define SPI_CS_DELAY_MODE_S 26
#define SPI_MOSI_DELAY_NUM 0x00000007
#define SPI_MOSI_DELAY_NUM_S 23
#define SPI_MOSI_DELAY_MODE 0x00000003  //mode 0 : posedge; data set at positive edge of clk
										//mode 1 : negedge + 1 cycle delay, only if freq<10MHz ; data set at negitive edge of clk
										//mode 2 : Do not use this mode.
#define SPI_MOSI_DELAY_MODE_S 21
#define SPI_MISO_DELAY_NUM 0x00000007
#define SPI_MISO_DELAY_NUM_S 18
#define SPI_MISO_DELAY_MODE 0x00000003
#define SPI_MISO_DELAY_MODE_S 16
#define SPI_CK_OUT_HIGH_MODE 0x0000000F
#define SPI_CK_OUT_HIGH_MODE_S 12
#define SPI_CK_OUT_LOW_MODE 0x0000000F
#define SPI_CK_OUT_LOW_MODE_S 8

	 */

	WRITE_PERI_REG(SPI_CTRL2(HSPI),
//			(( 0xF & SPI_SETUP_TIME )<<SPI_SETUP_TIME_S ) |
//			(( 0xF & SPI_HOLD_TIME )<<SPI_HOLD_TIME_S ) |
			(( 0xF & SPI_CK_OUT_LOW_MODE )<<SPI_CK_OUT_LOW_MODE_S ) |
			(( 0xF & SPI_CK_OUT_HIGH_MODE )<<SPI_CK_OUT_HIGH_MODE_S ) |
			(( 0x1 & SPI_MISO_DELAY_MODE )<<SPI_MISO_DELAY_MODE_S )
			);

/*	WRITE_PERI_REG(SPI_CTRL2(HSPI),
			(( m &SPI_CS_DELAY_MODE)<<SPI_CS_DELAY_MODE_S) | //
			(1<<SPI_MOSI_DELAY_MODE_S) |//
			((0xF & SPI_CS_DELAY_NUM) << SPI_CS_DELAY_NUM_S) |//
			((0xF & SPI_MOSI_DELAY_NUM) << SPI_MOSI_DELAY_NUM_S) |//
			((0xF & SPI_SETUP_TIME) << SPI_SETUP_TIME_S) |//
			((0xF & SPI_MISO_DELAY_NUM) << SPI_MISO_DELAY_NUM_S));*/
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
		for (i = 0; i < 35; i++) {
			mode(i);
//			for (j = 0; j < 3; j++) {
//				spi_mode(HSPI, j >> 1, j & 1);
//				spi_cs_deselect();
//			writeDW1000(0x84, 32, 0x1000);
			spi_clear();
			rxd = readDW1000(0x00, i);
//				spi_cs_deselect();
			INFO(" 0x%X : %X", i, rxd);
			os_delay_us(100);
//			}
		}

	}
PT_END()
;
return false;
}
