/*
 * fw/board_hulusb.c - Busware HUL Board-specific functions (for boot loader and application)
 *
 * Written 2017 by Filzmaier Josef
 * Based on fw/board_rzusb written and Copyright 2016 Stefan Schmidt
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
#include <avr/boot.h>

#define F_CPU   8000000UL
#include <util/delay.h>

#include "usb.h"
#include "at86rf230.h"
#include "board.h"
#include "spi.h"
#include "usb/usb.h"

static bool spi_initialized = 0;

void reset_rf(void)
{
	/* set up all the outputs; default port value is 0 */

	DDRB = 0;
	DDRC = 0;
	DDRD = 0;
	PORTB = 0;
	PORTC = 0;
	PORTD = 0;

	OUT(LED_RED);
	OUT(LED_GREEN);
	SET(LED_RED); /* Leds are active low on HULUSB board */
	CLR(LED_GREEN); /* Green Led indicates the dongle is running */
	OUT(nRST_RF);   /* this also resets the transceiver */
	OUT(SLP_TR);

	spi_init();

	/* AT86RF212 data sheet, Appendix B, p166 Power-On Reset procedure */
	/*-----------------------------------------------------------------*/
	CLR(SLP_TR);
	SET(nRST_RF);
	SET(nSS);
	_delay_us(400);

	CLR(nRST_RF);
	_delay_us(2);
	SET(nRST_RF);

	/* 5.1.4.5: Wait t10: 625 ns (min) */

	_delay_us(2);

	reg_write(REG_TRX_CTRL_0, 0x19);

	change_state(TRX_CMD_FORCE_TRX_OFF);
	/*-----------------------------------------------------------------*/

	/* we must restore TRX_CTRL_0 after each reset (7.7.4) */

	set_clkm();
}

void led_red(bool on) {
	if (on)
		CLR(LED_RED);
	else
		SET(LED_RED);
}

void led_green(bool on) {
	if (on)
		CLR(LED_GREEN);
	else
		SET(LED_GREEN);
}

void led(bool on)
{
	led_red(on);
}

void set_clkm(void)
{
	/* CLKM is not connected on BUSWARE HUL and therefore it is running in
	 * async mode. */
	reg_write(REG_TRX_CTRL_0, 0x00);

	/* TX_AUTO_CRC_ON, default disabled */
	subreg_write(SR_TX_AUTO_CRC_ON, 1);
}

void board_init(void)
{
	/* Disable the watchdog timer */

	MCUSR = 0;		/* Remove override */
	WDTCSR |= 1 << WDCE;	/* Enable change */
	WDTCSR = 1 << WDCE;	/* Disable watchdog while still enabling
	change */

	CLKPR = 1 << CLKPCE;
	/* We start with a 16 MHz/8 clock. Put the prescaler to 2. */
	CLKPR = 1 << CLKPS0;

	get_sernum();
}

void spi_begin(void)
{
	if (!spi_initialized)
		spi_init();
	CLR(nSS);
}

void spi_off(void)
{
	spi_initialized = 0;
	SPCR &= ~(1 << SPE);
}

void spi_init(void)
{
	SET(nSS);
	OUT(SCLK);
	OUT(MOSI);
	OUT(nSS);
	IN(MISO);

	SPCR = (1 << SPE) | (1 << MSTR);
	SPSR = (1 << SPI2X);

	spi_initialized = 1;
}

void usb_init(void)
{
	USBCON |= 1 << FRZCLK;		/* freeze the clock */

	/* enable the PLL and wait for it to lock */
	/* TODO sheet page 50 For Atmel AT90USB128x only. Do not use with Atmel AT90USB64x. */
	/*  FOR 8 XTAL Mhz only!!! */
	PLLCSR = ((1 << PLLP1) | (1 << PLLP0));
	PLLCSR |= 1 << PLLE;
	while (!(PLLCSR & (1 << PLOCK)));

	UHWCON |= (1 << UVREGE);

	USBCON &= ~((1 << USBE) | (1 << OTGPADE));		/* reset the controller */
	USBCON |= ((1 << USBE) | (1 << OTGPADE));

	USBCON &= ~(1 << FRZCLK);	/* thaw the clock */

	UDCON &= ~(1 << DETACH);	/* attach the pull-up */
	UDIEN = 1 << EORSTE;		/* enable device interrupts  */
	//	UDCON |= 1 << RSTCPU;		/* reset CPU on bus reset */

	ep_init();
}

void board_app_init(void)
{
	/* enable INT0, trigger on rising edge */
	EICRA = 1 << ISC01 | 1 << ISC00;
	EIMSK = 1 << INT0;
}
