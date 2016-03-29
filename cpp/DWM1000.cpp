/*
 * DWM1000.cpp
 *
 *  Created on: Feb 12, 2016
 *      Author: lieven
 */

#include <DWM1000.h>
#include <Logger.h>

extern "C" {
#include <spi.h>
#include <gpio_c.h>
#include <espmissingincludes.h>
#include <osapi.h>
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_sleep.h"
}
;
/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 1000

/* Default communication configuration. We use here EVK1000's default mode (mode 3). */
static dwt_config_t config = { 2, /* Channel number. */
DWT_PRF_64M, /* Pulse repetition frequency. */
DWT_PLEN_1024, /* Preamble length. */
DWT_PAC32, /* Preamble acquisition chunk size. Used in RX only. */
9, /* TX preamble code. Used in TX only. */
9, /* RX preamble code. Used in RX only. */
1, /* Use non-standard SFD (Boolean) */
DWT_BR_110K, /* Data rate. */
DWT_PHRMODE_STD, /* PHY header mode. */
(1025 + 64 - 32) /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* Default antenna delay values for 64 MHz PRF. See NOTE 1 below. */
#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8 tx_poll_msg[] = { 0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E',
		0x21, 0, 0 };
static uint8 rx_resp_msg[] = { 0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A',
		0x10, 0x02, 0, 0, 0, 0 };
static uint8 tx_final_msg[] = { 0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E',
		0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX 2
#define FINAL_MSG_POLL_TX_TS_IDX 10
#define FINAL_MSG_RESP_RX_TS_IDX 14
#define FINAL_MSG_FINAL_TX_TS_IDX 18
#define FINAL_MSG_TS_LEN 4
/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 20
static uint8 rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference, so reader can examine it at a breakpoint. */
static uint32 status_reg = 0;

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion factor.
 * 1 uus = 512 / 499.2 µs and 1 µs = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW1000's wait for response feature. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 150
/* This is the delay from Frame RX timestamp to TX reply timestamp used for calculating/setting the DW1000's delayed TX function. This includes the
 * frame length of approximately 2.66 ms with above configuration. */
#define RESP_RX_TO_FINAL_TX_DLY_UUS 3100
/* Receive response timeout. See NOTE 5 below. */
#define RESP_RX_TIMEOUT_UUS 2700

/* Time-stamps of frames transmission/reception, expressed in device time units.
 * As they are 40-bit wide, we need to define a 64-bit int type to handle them. */
typedef unsigned long long uint64;
static uint64 poll_tx_ts;
static uint64 resp_rx_ts;
static uint64 final_tx_ts;

/* Declaration of static functions. */
static uint64 get_tx_timestamp_u64(void);
static uint64 get_rx_timestamp_u64(void);
static void final_msg_set_ts(uint8 *ts_field, uint64 ts);

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
	mode(0);

	uint64_t eui = 0xF1F2F3F4F5F6F7F8;
	dwt_seteui((uint8_t*) &eui);
	dwt_geteui((uint8_t*) &eui);

	if (dwt_configure(&config) == DWT_ERROR)
		INFO("dwt_configure failed."); // Configure DW1000. See NOTE 2 below.
	else
		INFO("dwt_configure succeeded.");
	INFO("device id : %X ", dwt_readdevid());
	INFO("part   id : %X ", dwt_getpartid());

	/* Apply default antenna delay value. See NOTE 1 below. */
	dwt_setrxantennadelay(RX_ANT_DLY);
	dwt_settxantennadelay(TX_ANT_DLY);

	/* Set expected response's delay and timeout. See NOTE 4 and 5 below.
	 * As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
	dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
	dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

//	dwt_softreset();
	dwt_seteui((uint8_t*) &eui);
	dwt_geteui((uint8_t*) &eui);
	INFO("EUID   id : %lX ", eui);

	_count = 0;
}

uint32_t readDW1000(uint32_t offset, uint32_t length) {
	return spi_transaction(HSPI, 0, 0, 0, 0, 8, offset, length, 0);

}

uint32_t writeDW1000(uint32_t offset, uint32_t length, uint32_t value) {
	return spi_transaction(HSPI, 0, 0, 8, offset, length, value, 0, 0);

}

void DWM1000::mode(uint32_t m) {
	WRITE_PERI_REG(SPI_CTRL2(HSPI), 0xFFFFFFFF);

	WRITE_PERI_REG(SPI_CTRL2(HSPI),
			(( 0xF & SPI_CS_DELAY_NUM ) << SPI_CS_DELAY_NUM_S) | //
			(( 0x1 & SPI_CS_DELAY_MODE) << SPI_CS_DELAY_MODE_S) |//
			(( 0xF & SPI_SETUP_TIME )<< SPI_SETUP_TIME_S ) |//
			(( 0xF & SPI_HOLD_TIME )<< SPI_HOLD_TIME_S ) |//
			(( 0xF & SPI_CK_OUT_LOW_MODE )<< SPI_CK_OUT_LOW_MODE_S ) |//
			(( 0xF & SPI_CK_OUT_HIGH_MODE )<< SPI_CK_OUT_HIGH_MODE_S ) |//
			(( 0x7 & SPI_MOSI_DELAY_NUM ) << SPI_MOSI_DELAY_NUM_S) |//
			(( 0x7 & SPI_MISO_DELAY_NUM ) << SPI_MISO_DELAY_NUM_S) |//
			(( 0x1 & SPI_MOSI_DELAY_MODE )<< SPI_MOSI_DELAY_MODE_S ) |//
			(( 0x1 & SPI_MISO_DELAY_MODE )<< SPI_MISO_DELAY_MODE_S ) |//
			0);
}
extern "C" char* bytesToHex(uint8_t* pb, uint32_t len);
bool DWM1000::dispatch(Msg& msg) {
	PT_BEGIN()
	PT_WAIT_UNTIL(msg.is(0, SIG_INIT));
	init();
	while (true) {
		timeout(100);
		PT_YIELD_UNTIL(timeout());
		/* Start transmission. */
		dwt_starttx(DWT_START_TX_IMMEDIATE);
		/* Poll DW1000 until TX frame sent event set. See NOTE 4 below.
		 * STATUS register is 5 bytes long but, as the event we are looking at is in the first byte of the register, we can use this simplest API
		 * function to access it.*/
		while (1) {
			/* Write frame data to DW1000 and prepare transmission. See NOTE 7 below. */
			tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
			dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
			dwt_writetxfctrl(sizeof(tx_poll_msg), 0);

			/* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay
			 * set by dwt_setrxaftertxdelay() has elapsed. */
			dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

			/* We assume that the transmission is achieved correctly, poll for reception of a frame or error/timeout. See NOTE 8 below. */
			while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID))
					& (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR))) {
			};
			if ( status_reg & SYS_STATUS_ALL_RX_ERR ) INFO(" RX error ");

			/* Increment frame sequence number after transmission of the poll message (modulo 256). */
			frame_seq_nb++;

			if (status_reg & SYS_STATUS_RXFCG) {
				uint32 frame_len;

				/* Clear good RX frame event and TX frame sent in the DW1000 status register. */
				dwt_write32bitreg(SYS_STATUS_ID,
						SYS_STATUS_RXFCG | SYS_STATUS_TXFRS);

				/* A frame has been received, read it into the local buffer. */
				frame_len =
						dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
				if (frame_len <= RX_BUF_LEN) {
					dwt_readrxdata(rx_buffer, frame_len, 0);
				}

				/* Check that the frame is the expected response from the companion "DS TWR responder" example.
				 * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
				rx_buffer[ALL_MSG_SN_IDX] = 0;
				if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0) {
					uint32 final_tx_time;

					/* Retrieve poll transmission and response reception timestamp. */
					poll_tx_ts = get_tx_timestamp_u64();
					resp_rx_ts = get_rx_timestamp_u64();

					/* Compute final message transmission time. See NOTE 9 below. */
					final_tx_time = (resp_rx_ts
							+ (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME))
							>> 8;
					dwt_setdelayedtrxtime(final_tx_time);

					/* Final TX timestamp is the transmission time we programmed plus the TX antenna delay. */
					final_tx_ts = (((uint64) (final_tx_time & 0xFFFFFFFE)) << 8)
							+ TX_ANT_DLY;

					/* Write all timestamps in the final message. See NOTE 10 below. */
					final_msg_set_ts(&tx_final_msg[FINAL_MSG_POLL_TX_TS_IDX],
							poll_tx_ts);
					final_msg_set_ts(&tx_final_msg[FINAL_MSG_RESP_RX_TS_IDX],
							resp_rx_ts);
					final_msg_set_ts(&tx_final_msg[FINAL_MSG_FINAL_TX_TS_IDX],
							final_tx_ts);

					/* Write and send final message. See NOTE 7 below. */
					tx_final_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
					dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0);
					dwt_writetxfctrl(sizeof(tx_final_msg), 0);
					dwt_starttx(DWT_START_TX_DELAYED);

					/* Poll DW1000 until TX frame sent event set. See NOTE 8 below. */
					while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS)) {
					};

					/* Clear TXFRS event. */
					dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

					/* Increment frame sequence number after transmission of the final message (modulo 256). */
					frame_seq_nb++;
				} else {
					INFO(" not expected response ");
				}
			} else {
				/* Clear RX error events in the DW1000 status register. */
				dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
			}

			/* Execute a delay between ranging exchanges. */
			timeout(RNG_DELAY_MS);
			PT_YIELD_UNTIL(timeout());
//			deca_sleep(RNG_DELAY_MS);
		}
	}

PT_END()
;
return false;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn get_tx_timestamp_u64()
 *
 * @brief Get the TX time-stamp in a 64-bit variable.
 *        /!\ This function assumes that length of time-stamps is 40 bits, for both TX and RX!
 *
 * @param  none
 *
 * @return  64-bit value of the read time-stamp.
 */
static uint64 get_tx_timestamp_u64(void) {
uint8 ts_tab[5];
uint64 ts = 0;
int i;
dwt_readtxtimestamp(ts_tab);
for (i = 4; i >= 0; i--) {
	ts <<= 8;
	ts |= ts_tab[i];
}
return ts;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn get_rx_timestamp_u64()
 *
 * @brief Get the RX time-stamp in a 64-bit variable.
 *        /!\ This function assumes that length of time-stamps is 40 bits, for both TX and RX!
 *
 * @param  none
 *
 * @return  64-bit value of the read time-stamp.
 */
static uint64 get_rx_timestamp_u64(void) {
uint8 ts_tab[5];
uint64 ts = 0;
int i;
dwt_readrxtimestamp(ts_tab);
for (i = 4; i >= 0; i--) {
	ts <<= 8;
	ts |= ts_tab[i];
}
return ts;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn final_msg_get_ts()
 *
 * @brief Read a given timestamp value from the final message. In the timestamp fields of the final message, the least
 *        significant byte is at the lower address.
 *
 * @param  ts_field  pointer on the first byte of the timestamp field to read
 *         ts  timestamp value
 *
 * @return none
 */
static void final_msg_get_ts(const uint8 *ts_field, uint32 *ts) {
int i;
*ts = 0;
for (i = 0; i < FINAL_MSG_TS_LEN; i++) {
	*ts += ts_field[i] << (i * 8);
}
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn final_msg_set_ts()
 *
 * @brief Fill a given timestamp field in the final message with the given value. In the timestamp fields of the final
 *        message, the least significant byte is at the lower address.
 *
 * @param  ts_field  pointer on the first byte of the timestamp field to fill
 *         ts  timestamp value
 *
 * @return none
 */
static void final_msg_set_ts(uint8 *ts_field, uint64 ts)
{
    int i;
    for (i = 0; i < FINAL_MSG_TS_LEN; i++)
    {
        ts_field[i] = (uint8) ts;
        ts >>= 8;
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The sum of the values is the TX to RX antenna delay, experimentally determined by a calibration process. Here we use a hard coded typical value
 *    but, in a real application, each device should have its own antenna delay properly calibrated to get the best possible precision when performing
 *    range measurements.
 * 2. The messages here are similar to those used in the DecaRanging ARM application (shipped with EVK1000 kit). They comply with the IEEE
 *    802.15.4 standard MAC data frame encoding and they are following the ISO/IEC:24730-62:2013 standard. The messages used are:
 *     - a poll message sent by the initiator to trigger the ranging exchange.
 *     - a response message sent by the responder allowing the initiator to go on with the process
 *     - a final message sent by the initiator to complete the exchange and provide all information needed by the responder to compute the
 *       time-of-flight (distance) estimate.
 *    The first 10 bytes of those frame are common and are composed of the following fields:
 *     - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA).
 *     - byte 5/6: destination address, see NOTE 3 below.
 *     - byte 7/8: source address, see NOTE 3 below.
 *     - byte 9: function code (specific values to indicate which message it is in the ranging process).
 *    The remaining bytes are specific to each message as follows:
 *    Poll message:
 *     - no more data
 *    Response message:
 *     - byte 10: activity code (0x02 to tell the initiator to go on with the ranging exchange).
 *     - byte 11/12: activity parameter, not used for activity code 0x02.
 *    Final message:
 *     - byte 10 -> 13: poll message transmission timestamp.
 *     - byte 14 -> 17: response message reception timestamp.
 *     - byte 18 -> 21: final message transmission timestamp.
 *    All messages end with a 2-byte checksum automatically set by DW1000.
 * 3. Source and destination addresses are hard coded constants in this example to keep it simple but for a real product every device should have a
 *    unique ID. Here, 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only
 *    after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.
 * 4. Delays between frames have been chosen here to ensure proper synchronisation of transmission and reception of the frames between the initiator
 *    and the responder and to ensure a correct accuracy of the computed distance. The user is referred to DecaRanging ARM Source Code Guide for more
 *    details about the timings involved in the ranging process.
 * 5. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive the complete final frame sent by the responder at the
 *    110k data rate used (around 3.5 ms).
 * 6. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 7. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW1000 User Manual for more details on "interrupts". It is also to be noted that STATUS register is 5 bytes long but, as the event we
 *    use are all in the first bytes of the register, we can use the simple dwt_read32bitreg() API call to access it instead of reading the whole 5
 *    bytes.
 * 8. Timestamps and delayed transmission time are both expressed in device time units so we just have to add the desired response delay to poll RX
 *    timestamp to get response transmission time. The delayed transmission time resolution is 512 device time units which means that the lower 9 bits
 *    of the obtained value must be zeroed. This also allows to encode the 40-bit value in a 32-bit words by shifting the all-zero lower 8 bits.
 * 9. dwt_writetxdata() takes the full size of the message as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW1000. This means that our variable could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()). It is also to be noted that, when using
 *    delayed send, the time set for transmission must be far enough in the future so that the DW1000 IC has the time to process and start the
 *    transmission of the frame at the wanted time. If the transmission command is issued too late compared to when the frame is supposed to be sent,
 *    this is indicated by an error code returned by dwt_starttx() API call. Here it is not tested, as the values of the delays between frames have
 *    been carefully defined to avoid this situation.
 * 10. The high order byte of each 40-bit time-stamps is discarded here. This is acceptable as, on each device, those time-stamps are not separated by
 *     more than 2**32 device time units (which is around 67 ms) which means that the calculation of the round-trip delays can be handled by a 32-bit
 *     subtraction.
 * 11. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *     DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/
