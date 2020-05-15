/*
 * fw/usb/atu2.c - Chip-specific driver for Atmel ATxxxU2 USB chips
 *
 * Written 2008-2011, 2013-2014 by Werner Almesberger
 * Copyright 2008-2011, 2013-2014 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Known issues:
 * - no suspend/resume
 * - we don't call back after failed transmissions,
 * - we don't reset the EP buffer after failed receptions
 * - enumeration often encounters an error -71 (from which it recovers)
 */

#include <stdbool.h>
#include <stdint.h>

#define F_CPU   8000000UL
#include <util/delay.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include "usb.h"
#include "board.h"


#ifndef NULL
#define NULL 0
#endif

#if 1
#define BUG_ON(cond)    do { if (cond) panic(); } while (0)
#else
#define BUG_ON(cond)
#endif


struct ep_descr eps[NUM_EPS];


static uint16_t usb_read_word(void)
{
	uint8_t low;

	low = UEDATX;
	return low | UEDATX << 8;
}


static void enable_addr(void *user)
{
	while (!(UEINTX & (1 << TXINI)));
	UDADDR |= 1 << ADDEN;
}


void set_addr(uint8_t addr)
{
	UDADDR = addr;
	usb_send(&eps[0], NULL, 0, enable_addr, NULL);
}


void usb_ep_change(struct ep_descr *ep)
{
	if (ep->state == EP_TX) {
		UENUM = ep-eps;
		UEIENX |= 1 << TXINE;
	}
}


static bool ep_setup(void)
{
	struct setup_request setup;

	BUG_ON(UEBCLX < 8);

	setup.bmRequestType = UEDATX;
	setup.bRequest = UEDATX;
	setup.wValue = usb_read_word();
	setup.wIndex = usb_read_word();
	setup.wLength = usb_read_word();

	if (!handle_setup(&setup))
		return 0;
	if (!(setup.bmRequestType & 0x80) && eps[0].state == EP_IDLE)
		usb_send(&eps[0], NULL, 0, NULL, NULL);
	return 1;
}


static bool ep_rx(struct ep_descr *ep)
{
	uint8_t size;

	size = UEBCLX;
	if (size > ep->end-ep->buf)
		return 0;
	while (size--)
		*ep->buf++ = UEDATX;
	if (ep->buf == ep->end) {
		ep->state = EP_IDLE;
		if (ep->callback)
			ep->callback(ep->user);
//		if (ep == &eps[0])
			usb_send(ep, NULL, 0, NULL, NULL);
	}
	return 1;
}


static void ep_tx(struct ep_descr *ep)
{
	uint8_t size = ep->end-ep->buf;
	uint8_t left;

	if (size > ep->size)
		size = ep->size;
	for (left = size; left; left--)
		UEDATX = *ep->buf++;
	if (size == ep->size)
		return;
	ep->state = EP_IDLE;
}


static void handle_ep(int n)
{
	struct ep_descr *ep = eps+n;
	uint8_t mask;

	UENUM = n;
	if (UEINTX & (1 << RXSTPI)) {
		/* @@@ EP_RX. EP_TX: cancel */
		ep->state = EP_IDLE;
		if (!ep_setup())
			goto stall;
		UEINTX = ~(1 << RXSTPI);
	}
	if (UEINTX & (1 << RXOUTI)) {
		/* @@ EP_TX: cancel */
		if (ep->state != EP_RX)
			goto stall;
		if (!ep_rx(ep))
			goto stall;
		/* @@@ gcc 4.5.2 wants this cast */
		UEINTX = (uint8_t) ~(1 << RXOUTI | 1 << FIFOCON);
	}
	if (UEINTX & (1 << STALLEDI)) {
		ep->state = EP_IDLE;
		UEINTX = ~(1 << STALLEDI);
	}
	if (UEINTX & (1 << TXINI)) {
		/* @@ EP_RX: cancel (?) */
		if (ep->state == EP_TX) {
			ep_tx(ep);
			mask = 1 << TXINI;
			if (n)
				mask |= 1 << FIFOCON;
			UEINTX = ~mask;
			if (ep->state == EP_IDLE && ep->callback)
				ep->callback(ep->user);
		} else {
			UEIENX &= ~(1 << TXINE);
		}
	}
	return;

stall:
	UEINTX = ~(1 << RXSTPI | 1 << RXOUTI | 1 << STALLEDI);
	ep->state = EP_IDLE;
	UECONX |= 1 << STALLRQ;
}


void ep_init(void)
{
	UENUM = 0;
	UECONX = (1 << RSTDT) | (1 << EPEN);	/* enable */
	UECFG0X = 0;	/* control, direction is ignored */
	UECFG1X = 3 << EPSIZE0;	/* 64 bytes */
	UECFG1X |= 1 << ALLOC;

	while (!(UESTA0X & (1 << CFGOK)));

	UEIENX =
	    (1 << RXSTPE) | (1 << RXOUTE) | (1 << STALLEDE) | (1 << TXINE);

	eps[0].state = EP_IDLE;
	eps[0].size = 64;

#ifndef BOOT_LOADER

	UENUM = 1;
	UECONX = (1 << RSTDT) | (1 << EPEN);	/* enable */
	UECFG0X = (1 << EPTYPE1) | (1 << EPDIR); /* bulk IN */
	UECFG1X = 3 << EPSIZE0;	/* 64 bytes */
	UECFG1X |= 1 << ALLOC;

	while (!(UESTA0X & (1 << CFGOK)));

	UEIENX = (1 << STALLEDE) | (1 << TXINE);

	eps[1].state = EP_IDLE;
	eps[1].size = 64;

#endif
}


ISR(USB_GEN_vect)
{
	uint8_t flags;

	flags = UDINT;
	if (flags & (1 << EORSTI)) {
		if (user_reset)
			user_reset();
		ep_init();
		UDINT = ~(1 << EORSTI);
	}
}


ISR(USB_COM_vect)
{
	uint8_t flags, i;

	flags = UEINT;
	for (i = 0; i != NUM_EPS; i++)
		if (flags & (1 << i))
			handle_ep(i);
}


void usb_reset(void)
{
	UDCON |= 1 << DETACH;	/* detach the pull-up */
	_delay_ms(1);
}
