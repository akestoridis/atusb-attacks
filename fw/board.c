/*
 * fw/board.c - Board-specific functions (for boot loader and application)
 *
 * Written 2011, 2013 by Werner Almesberger
 * Copyright 2011, 2013 Werner Almesberger
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


uint8_t board_sernum[42] = { 42, USB_DT_STRING };

/* ----- Register access --------------------------------------------------- */

void change_state(uint8_t new)
{
	while ((reg_read(REG_TRX_STATUS) & TRX_STATUS_MASK) ==
		TRX_STATUS_TRANSITION);
	reg_write(REG_TRX_STATE, new);
}


uint8_t reg_read(uint8_t reg)
{
	uint8_t value;

	spi_begin();
	spi_send(AT86RF230_REG_READ | reg);
	value = spi_recv();
	spi_end();

	return value;
}


uint8_t subreg_read(uint8_t address, uint8_t mask, uint8_t position)
{
	/* Read current register value and mask out subregister. */
	uint8_t register_value = reg_read(address);
	register_value &= mask;
	register_value >>= position; /* Align subregister value. */

	return register_value;
}


void reg_write(uint8_t reg, uint8_t value)
{
	spi_begin();
	spi_send(AT86RF230_REG_WRITE | reg);
	spi_send(value);
	spi_end();
}


void subreg_write(uint8_t address, uint8_t mask, uint8_t position, uint8_t value)
{
	/* Read current register value and mask area outside the subregister. */
	uint8_t register_value = reg_read(address);
	register_value &= ~mask;

	/* Start preparing the new subregister value. shift in place and mask. */
	value <<= position;
	value &= mask;

	value |= register_value; /* Set the new subregister value. */

	/* Write the modified register value. */
	reg_write(address, value);
}


void panic(void)
{
	cli();
	while (1) {
		SET(LED);
		_delay_ms(100);
		CLR(LED);
		_delay_ms(100);
	}
}


static char hex(uint8_t nibble)
{
	return nibble < 10 ? '0'+nibble : 'a'+nibble-10;
}


void get_sernum(void)
{
	uint8_t sig;
	uint8_t i;

	for (i = 0; i != 10; i++) {
		sig = boot_signature_byte_get(i+0xe);
		board_sernum[(i << 2)+2] = hex(sig >> 4);
		board_sernum[(i << 2)+4] = hex(sig & 0xf);
	}
}
