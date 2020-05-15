/*
 * fw/atusb.c - ATUSB initialization and main loop
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
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "usb.h"

#include "board.h"
#include "sernum.h"
#include "spi.h"
#include "atusb/ep0.h"

#ifdef DEBUG
#include "uart.h"
#endif


int main(void)
{
	board_init();
	board_app_init();
	reset_rf();

	user_get_descriptor = sernum_get_descr;

	/* now we should be at 8 MHz */

#ifdef DEBUG
	uart_init();
	static FILE atben_stdout = FDEV_SETUP_STREAM(uart_write_char, NULL,
						     _FDEV_SETUP_WRITE);
	stdout = &atben_stdout;
#endif

	usb_init();
	ep0_init();
#ifdef ATUSB
	timer_init();

	/* move interrupt vectors to 0 */
	MCUCR = 1 << IVCE;
	MCUCR = 0;
#endif

	sei();

	while (1)
		sleep_mode();
}
