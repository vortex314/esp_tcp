/*
 * File	: uart.c
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * LMR REDUCED TO THE MAX
 */
#include "stdint.h"
#include "ets_sys.h"
#include "osapi.h"
#include "uart.h"
#include "osapi.h"
#include "uart_register.h"
#include "mem.h"
#include "os_type.h"

extern UartDevice UartDev;

#include "Sys.h"
#include <Logger.h>

extern void uart0RecvByte(uint8_t b);
extern void uart0Write(uint8_t b);

IRAM void uart0WriteWait(uint8 TxChar) {
	while (true) {
		uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(0))
				& (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);
		if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
			break;
		}
	}

	WRITE_PERI_REG(UART_FIFO(0), TxChar);
}

LOCAL void IRAM uart0_rx_intr_handler(void *para);

/******************************************************************************
 * FunctionName : uart_config
 * Description  : Internal used function
 *                UART0 used for data TX/RX, RX buffer size is 0x100, interrupt enabled
 *                UART1 just used for debug output
 * Parameters   : uart_no, use UART0 or UART1 defined ahead
 * Returns      : NONE
 *******************************************************************************/
void IROM
uart_config(uint32_t uart_no, uint32_t baudrate, char* mode) {
	INFO("uart_no %d baudrate %d mode %s", uart_no, baudrate, mode);
//	uart_rx_intr_disable(uart_no);
//	ETS_UART_INTR_DISABLE();
	UartDev.buff_uart_no = uart_no;
	UartDev.baut_rate = baudrate;

	uint32_t flags;

	UartDev.data_bits = EIGHT_BITS;

	if (mode[0] == '8') {
		UartDev.data_bits = EIGHT_BITS;
	} else if (mode[0] == '7') {
		UartDev.data_bits = SEVEN_BITS;
	};

	UartDev.parity = NONE_BITS;
	UartDev.exist_parity = STICK_PARITY_DIS;

	if (mode[1] == 'N') {
		UartDev.exist_parity = STICK_PARITY_DIS;
		UartDev.parity = NONE_BITS;
	} else if (mode[1] == 'E') {
		UartDev.exist_parity = STICK_PARITY_EN;
		UartDev.parity = EVEN_BITS;
	} else if (mode[1] == 'O') {
		UartDev.exist_parity = STICK_PARITY_EN;
		UartDev.parity = ODD_BITS;
	};

	UartDev.stop_bits = ONE_STOP_BIT;

	if (mode[2] == '1') {
		UartDev.stop_bits = ONE_STOP_BIT;
	} else if (mode[2] == '2') {
		UartDev.stop_bits = TWO_STOP_BIT;
	} else if (mode[2] == '-') {
		UartDev.stop_bits = ONE_HALF_STOP_BIT;
	};

	/* rcv_buff size if 0x100 */
	ETS_UART_INTR_ATTACH(uart0_rx_intr_handler, &(UartDev.rcv_buff));
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
	uart_div_modify(uart_no, UART_CLK_FREQ / baudrate); //SET BAUDRATE

	WRITE_PERI_REG(UART_CONF0(uart_no),  //
			UartDev.exist_parity //SET BIT AND PARITY MODE
			| UartDev.parity// parity
			| ((UartDev.stop_bits & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S)// stopbits
			| ((UartDev.data_bits & UART_BIT_NUM) << UART_BIT_NUM_S));// databits

	SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
	//clear rx and tx fifo,not ready
	CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
	//RESET FIFO

	WRITE_PERI_REG(UART_CONF1(uart_no),//set rx fifo trigger
			((100 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S)//
			| ((0x02 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S)//
			| UART_RX_TOUT_EN//
			| ((0x10 & UART_TXFIFO_EMPTY_THRHD)<<UART_TXFIFO_EMPTY_THRHD_S));//

	SET_PERI_REG_MASK(UART_INT_ENA(uart_no),
			UART_RXFIFO_TOUT_INT_ENA |UART_FRM_ERR_INT_ENA);

	//clear all interrupt
	WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
	//enable rx_interrupt
//	SET_PERI_REG_MASK(UART_INT_ENA(uart_no),
//			UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_OVF_INT_ENA|UART_RXFIFO_TOUT_INT_ENA|UART_TXFIFO_EMPTY_INT_ENA);
	SET_PERI_REG_MASK(UART_CONF1(uart_no),
			(UART_TX_EMPTY_THRESH_VAL & UART_TXFIFO_EMPTY_THRHD)<<UART_TXFIFO_EMPTY_THRHD_S);
//	SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
	SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA//
			|UART_RXFIFO_OVF_INT_ENA//
			|UART_RXFIFO_TOUT_INT_ENA);//
//	uart_tx_intr_enable(1);
	uart_rx_intr_enable(uart_no);
//	ETS_UART_INTR_ENABLE();
}

void IROM
uart1_config() {
	uint8 uart_no = UART1;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
	uart_div_modify(uart_no, UART_CLK_FREQ / (UartDev.baut_rate)); //SET BAUDRATE
	WRITE_PERI_REG(UART_CONF0(uart_no),
			((UartDev.exist_parity & UART_PARITY_EN_M) << UART_PARITY_EN_S) //SET BIT AND PARITY MODE
			| ((UartDev.parity & UART_PARITY_M) <<UART_PARITY_S ) | ((UartDev.stop_bits & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S) | ((UartDev.data_bits & UART_BIT_NUM) << UART_BIT_NUM_S));

	//clear rx and tx fifo,not ready
	SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
	//RESET FIFO
	CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);

	WRITE_PERI_REG(UART_CONF1(uart_no),
			((UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));

	//clear all interrupt
	WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
	//enable rx_interrupt

	SET_PERI_REG_MASK(UART_INT_ENA(uart_no),
			UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_OVF_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
//	uart_tx_intr_enable(1);
}

static IRAM unsigned int uart_rx_fifo_length(void) {
	return ((READ_PERI_REG(UART_STATUS(0)) >> UART_RXFIFO_CNT_S)
			& UART_RXFIFO_CNT);
}

static IRAM unsigned int uart_tx_fifo_length(void) {
	return ((READ_PERI_REG(UART_STATUS(0)) >> UART_TXFIFO_CNT_S)
			& UART_TXFIFO_CNT);
}

void IRAM uart_tx_intr_enable(uint32_t uartno) {
	SET_PERI_REG_MASK(UART_INT_ENA(uartno), UART_TXFIFO_EMPTY_INT_ENA);
}
void IRAM uart_tx_intr_disable(uint32_t uartno) {
	CLEAR_PERI_REG_MASK(UART_INT_ENA(uartno), UART_TXFIFO_EMPTY_INT_ENA);
}

void IRAM uart_rx_intr_disable(uint8 uart_no) {
	CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no),
			UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
}

void uart_rx_intr_enable(uint8 uart_no) {
	SET_PERI_REG_MASK(UART_INT_ENA(uart_no),
			UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
}

uint32_t uartRxdCount = 0;
uint32_t uartTxdCount = 0;
uint32_t uartErrorCount = 0;

/******************************************************************************
 * FunctionName : uart0_rx_intr_handler
 * Description  : Internal used function
 *                UART0 interrupt handler, add self handle code inside
 * Parameters   : void *para - point to ETS_UART_INTR_ATTACH's arg
 * Returns      : NONE
 *******************************************************************************/

LOCAL void IRAM uart0_rx_intr_handler(void *para) {
	uint32_t loop = 0;
//	ETS_UART_INTR_DISABLE();

	uint32_t int_status = READ_PERI_REG(UART_INT_ST(UART0));

	if (int_status & UART_FRM_ERR_INT_ST) { // FRAMING ERROR
		// clear rx fifo (apparently this is not optional at this point)
		SET_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST);
		CLEAR_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST);
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
		uartErrorCount++;

	};
	if (int_status & UART_RXFIFO_FULL_INT_ST) { // RXD FIFO FULL

		while (uart_rx_fifo_length()) {
			uart0RecvByte(READ_PERI_REG(UART_FIFO(0)));
			uartRxdCount++;
			loop++;
			if (loop > 100)
				break;
		}

		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);

	};

	if (int_status & UART_RXFIFO_TOUT_INT_ST) { // RXD FIFO TIMEOUT

		while (uart_rx_fifo_length()) {
			uart0RecvByte(READ_PERI_REG(UART_FIFO(0)));
			uartRxdCount++;
			loop++;
			if (loop > 100)
				break;
		}
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);

	};
	if (int_status & UART_TXFIFO_EMPTY_INT_ST) { // TXD FIFO EMPTY

		int b;
		while ((uart_tx_fifo_length() < 126) && ((b = uart0SendByte()) >= 0)) {
			uartTxdCount++;
			WRITE_PERI_REG(UART_FIFO(UART0), (uint8_t ) b);
			loop++;
			if (loop > 100)
				break;
		}
		if (b < 0) { // no more chars to send
			uart_tx_intr_disable(0);
		}
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_TXFIFO_EMPTY_INT_CLR);

	};
	if (int_status & UART_RXFIFO_OVF_INT_ST) {

		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_OVF_INT_CLR);
		uartErrorCount++;
	}
	WRITE_PERI_REG(UART_INT_CLR(UART0), 0xFFFF);
	if (READ_PERI_REG(UART_INT_ST(UART1))) {
		WRITE_PERI_REG(UART_INT_CLR(UART1), 0xFFFF);
	}
//	ETS_UART_INTR_ENABLE();

}

void IROM
uart_init(UartBautRate uart0_br, UartBautRate uart1_br) {

	UartDev.baut_rate = uart0_br;
	uart_config(0, uart0_br, "8N1");
	UartDev.baut_rate = uart1_br;
	uart1_config();
//	uart_config(1,uart1_br,"8N1");
	ETS_UART_INTR_ENABLE();

	os_install_putc1((void *) uart0Write);
}

