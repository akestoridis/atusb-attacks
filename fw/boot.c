/*
 * fw/boot.c - DFU boot loader for ATUSB
 *
 * Written 2008-2011 by Werner Almesberger
 * Copyright 2008-2011 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU   8000000UL
#include <util/delay.h>

#include "usb.h"
#include "dfu.h"

#include "board.h"
#include "spi.h"
#include "atusb/ep0.h"


#define	MS_TO_LOOPS(ms) ((uint32_t) (ms)*335)


static void (*run_payload)(void) = 0;


int main(void)
{
	/*
	 * pgm_read_byte gets cached and there doesn't seem to be any other
	 * way to dissuade gcc from doing this.
	 */
	volatile int zero = 0;
	uint32_t loop = 0;

	board_init();
	reset_rf();

	/* now we should be at 8 MHz */

	usb_init();
	dfu_init();

	/* move interrupt vectors to the boot loader */
	MCUCR = 1 << IVCE;
	MCUCR = 1 << IVSEL;

	sei();

	led(1);

	while (loop != MS_TO_LOOPS(2500)) {
		if (dfu.state == dfuIDLE && pgm_read_byte(zero) != 0xff)
			loop++;
		else
			loop = 0;
	}

	led(0);

	cli();

	usb_reset();
	run_payload();

	while (1);	/* not reached */
}
