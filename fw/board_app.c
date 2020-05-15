/*
 * fw/board_app.c - Board-specific functions (for the application)
 *
 * Written 2011, 2013 by Werner Almesberger
 * Copyright 2011, 2013 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU   8000000UL
#include <util/delay.h>

#include "usb.h"
#include "at86rf230.h"
#include "spi.h"
#include "mac.h"
#include "board.h"


static volatile uint32_t timer_h = 0;	/* 2^(16+32) / 8 MHz = ~1.1 years */


void reset_cpu(void)
{
	WDTCSR = 1 << WDE;
}


uint8_t read_irq(void)
{
	return PIN(IRQ_RF);
}


void slp_tr(void)
{
	SET(SLP_TR);
	CLR(SLP_TR);
}


ISR(TIMER1_OVF_vect)
{
	timer_h++;
}


uint64_t timer_read(void)
{
	uint32_t high;
	uint8_t low, mid;

	do {
		if (TIFR1 & (1 << TOV1)) {
			TIFR1 = 1 << TOV1;
			timer_h++;
		}
		high = timer_h;
		low = TCNT1L;
		mid = TCNT1H;
	}
	while (TIFR1 & (1 << TOV1));

	/*
	 * We need all these casts because the intermediate results are handled
	 * as if they were signed and thus get sign-expanded. Sounds wrong-ish.
	 */
	return (uint64_t) high << 16 | (uint64_t) mid << 8 | (uint64_t) low;
}


void timer_init(void)
{
	/* configure timer 1 as a free-running CLK counter */

	TCCR1A = 0;
	TCCR1B = 1 << CS10;

	/* enable timer overflow interrupt */

	TIMSK1 = 1 << TOIE1;
}


bool gpio(uint8_t port, uint8_t data, uint8_t dir, uint8_t mask, uint8_t *res)
{
	EIMSK = 0; /* recover INT_RF to ATUSB_GPIO_CLEANUP or an MCU reset */

	switch (port) {
	case 1:
		DDRB = (DDRB & ~mask) | dir;
		PORTB = (PORTB & ~mask) | data;
		break;
	case 2:
		DDRC = (DDRC & ~mask) | dir;
		PORTC = (PORTC & ~mask) | data;
		break;
	case 3:
		DDRD = (DDRD & ~mask) | dir;
		PORTD = (PORTD & ~mask) | data;
		break;
	default:
		return 0;
	}

	/* disable the UART so that we can meddle with these pins as well. */
	spi_off();
	_delay_ms(1);

	switch (port) {
	case 1:
		res[0] = PINB;
		res[1] = PORTB;
		res[2] = DDRB;
		break;
	case 2:
		res[0] = PINC;
		res[1] = PORTC;
		res[2] = DDRC;
		break;
	case 3:
		res[0] = PIND;
		res[1] = PORTD;
		res[2] = DDRD;
		break;
	}

	return 1;
}


void gpio_cleanup(void)
{
	EIMSK = 1 << 0;
}


static void done(void *user)
{
	led(0);
}


uint8_t irq_serial;

#if defined(ATUSB) || defined(HULUSB)
ISR(INT0_vect)
#endif
#ifdef RZUSB
ISR(TIMER1_CAPT_vect)
#endif
{
	if (mac_irq) {
		if (mac_irq())
			return;
	}
	if (eps[1].state == EP_IDLE) {
		led(1);
		irq_serial = (irq_serial+1) | 0x80;
		usb_send(&eps[1], &irq_serial, 1, done, NULL);
	}
}
