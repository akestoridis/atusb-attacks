/*
 * fw/mac.c - HardMAC functions
 *
 * Written 2011, 2013 by Werner Almesberger
 * Copyright 2011, 2013 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "usb.h"

#include "at86rf230.h"
#include "spi.h"
#include "board.h"
#include "mac.h"

#define	RX_BUFS	3


bool (*mac_irq)(void) = NULL;


static uint8_t rx_buf[RX_BUFS][MAX_PSDU+2]; /* PHDR+payload+LQ */
static uint8_t tx_buf[MAX_PSDU];
static uint8_t tx_size = 0;
static bool txing = 0;
static bool queued_tx_ack = 0;
static uint8_t next_seq, this_seq, queued_seq;


/* ----- Receive buffer management ----------------------------------------- */


static uint8_t rx_in = 0, rx_out = 0;


static inline void next_buf(uint8_t *index)
{
	*index = (*index+1) % RX_BUFS;
}


/* ----- Interrupt handling ------------------------------------------------ */


static void rx_done(void *user);
static void tx_ack_done(void *user);


static void usb_next(void)
{
	const uint8_t *buf;

	if (rx_in != rx_out) {
		buf = rx_buf[rx_out];
		led(1);
		usb_send(&eps[1], buf, buf[0]+2, rx_done, NULL);
	}

	if (queued_tx_ack) {
		usb_send(&eps[1], &queued_seq, 1, tx_ack_done, NULL);
		queued_tx_ack = 0;	
	}
}


static void tx_ack_done(void *user)
{
	usb_next();
}

static void rx_done(void *user)
{
	led(0);
	next_buf(&rx_out);
	usb_next();
#ifdef AT86RF230
	/* slap at86rf230 - reduce fragmentation issue */
	change_state(TRX_STATUS_RX_AACK_ON);
#endif
}


static void receive_frame(void)
{
	uint8_t size;
	uint8_t *buf;

	spi_begin();
	spi_io(AT86RF230_BUF_READ);

	size = spi_recv();
	if (!size || (size & 0x80)) {
		spi_end();
		return;
	}

	buf = rx_buf[rx_in];
	spi_recv_block(buf+1, size+1);
	spi_end();

	buf[0] = size;
	next_buf(&rx_in);

	if (eps[1].state == EP_IDLE)
		usb_next();
}


static bool handle_irq(void)
{
	uint8_t irq;

	irq = reg_read(REG_IRQ_STATUS);
	if (!(irq & IRQ_TRX_END))
		return 1;

	if (txing) {
		if (eps[1].state == EP_IDLE) {
			usb_send(&eps[1], &this_seq, 1, tx_ack_done, NULL);
		} else {
			queued_tx_ack = 1;
			queued_seq = this_seq;
		}
		txing = 0;
		return 1;
	}

	/* likely */
	if (eps[1].state == EP_IDLE || rx_in != rx_out)
		receive_frame();

	return 1;
}


/* ----- TX/RX ------------------------------------------------------------- */


bool mac_rx(int on)
{
	if (on) {
		mac_irq = handle_irq;
		reg_read(REG_IRQ_STATUS);
		change_state(TRX_CMD_RX_AACK_ON);
	} else {
		mac_irq = NULL;
		change_state(TRX_CMD_FORCE_TRX_OFF);
		txing = 0;
	}
	return 1;
}


static void do_tx(void *user)
{
	uint16_t timeout = 0xffff;
	uint8_t status;
	uint8_t i;

	/*
	 * If we time out here, the host driver will time out waiting for the
	 * TRX_END acknowledgement.
	 */
	do {
		if (!--timeout)
			return;
		status = reg_read(REG_TRX_STATUS) & TRX_STATUS_MASK;
	}
	while (status != TRX_STATUS_RX_ON && status != TRX_STATUS_RX_AACK_ON);

#ifdef AT86RF231
	/*
	 * We use TRX_CMD_FORCE_PLL_ON instead of TRX_CMD_PLL_ON because a new
	 * reception may have begun while we were still working on the previous
	 * one.
	 */
	reg_write(REG_TRX_STATE, TRX_CMD_FORCE_PLL_ON);
#endif
#ifdef AT86RF230
	/*
	 * at86rf230 doesn't support force change, nevetherless this works
	 * somehow
	 */
	reg_write(REG_TRX_STATE, TRX_CMD_PLL_ON);
#endif
#ifdef AT86RF212
	/*
	* We use TRX_CMD_FORCE_PLL_ON instead of TRX_CMD_PLL_ON because a new
	* reception may have begun while we were still working on the previous
	* one.
	*/
	reg_write(REG_TRX_STATE, TRX_CMD_FORCE_PLL_ON);
#endif

	handle_irq();

	spi_begin();
	spi_send(AT86RF230_BUF_WRITE);
	spi_send(tx_size+2); /* CRC */
	for (i = 0; i != tx_size; i++)
		spi_send(tx_buf[i]);
	spi_end();

	change_state(TRX_STATUS_TX_ARET_ON);

	slp_tr();

	txing = 1;
	this_seq = next_seq;

	/*
	 * Wait until we reach BUSY_TX_ARET, so that we command the transition to
	 * RX_AACK_ON which will be executed upon TX completion.
	 */
	change_state(TRX_CMD_PLL_ON);
	change_state(TRX_CMD_RX_AACK_ON);
}


bool mac_tx(uint16_t flags, uint8_t seq, uint16_t len)
{
	if (len > MAX_PSDU)
		return 0;
	tx_size = len;
	next_seq = seq;
	usb_recv(&eps[0], tx_buf, len, do_tx, NULL);
	return 1;
}


void mac_reset(void)
{
	mac_irq = NULL;
	txing = 0;
	queued_tx_ack = 0;
	rx_in = rx_out = 0;
	next_seq = this_seq = queued_seq = 0;

	/* enable CRC and PHY_RSSI (with RX_CRC_VALID) in SPI status return */
	reg_write(REG_TRX_CTRL_1,
	    TX_AUTO_CRC_ON | SPI_CMD_MODE_PHY_RSSI << SPI_CMD_MODE_SHIFT);
}
