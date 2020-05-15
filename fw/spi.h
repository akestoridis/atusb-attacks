/*
 * fw/spi.h - ATmega8 family SPI I/O
 *
 * Written 2011, 2013 by Werner Almesberger
 * Copyright 2011, 2013 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SPI_H
#define	SPI_H

#include <stdint.h>


void spi_begin(void);
uint8_t spi_io(uint8_t v);
void spi_end(void);
void spi_off(void);
void spi_init(void);

#define	spi_send(v)	(void) spi_io(v)
#define	spi_recv(v)	spi_io(0)

void spi_recv_block(uint8_t *buf, uint8_t n);

#endif /* !SPI_H */
