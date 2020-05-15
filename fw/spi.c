/*
 * fw/spi.c - ATmega8 family SPI I/O
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

#include "board.h"
#include "spi.h"


uint8_t spi_io(uint8_t v)
{
//      while (!(UCSR1A & 1 << UDRE1));
	SPI_DATA = v;
	SPI_WAIT_DONE();
	return SPI_DATA;
}


void spi_end(void)
{
//      while (!(UCSR1A & 1 << TXC1));
	SET(nSS);
}


void spi_recv_block(uint8_t *buf, uint8_t n)
{
	if (!n)
		return;
	SPI_DATA = 0;
	while (--n) {
		SPI_WAIT_DONE();
		*buf++ = SPI_DATA;
		SPI_DATA = 0;
	}
	SPI_WAIT_DONE();
	*buf++ = SPI_DATA;
}
