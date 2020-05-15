/*
 * fw/board_atusb.h - ATUSB Board-specific functions and definitions
 *
 * Written 2016 by Stefan Schmidt
 * Copyright 2016 Stefan Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef BOARD_ATUSB_H
#define	BOARD_ATUSB_H

#include <stdbool.h>
#include <stdint.h>

#define	LED_PORT	B
#define	LED_BIT		  6
#define	nRST_RF_PORT	C
#define	nRST_RF_BIT	  7
#define	SLP_TR_PORT	B
#define	SLP_TR_BIT	  4

#define SCLK_PORT	D
#define SCLK_BIT	  5
#define	MOSI_PORT	D
#define	MOSI_BIT	  3

#define	MISO_PORT	D
#define	MISO_BIT	  2
#define	nSS_PORT	D
#define	nSS_BIT		  1
#define	IRQ_RF_PORT	D
#define	IRQ_RF_BIT	  0

#define SPI_WAIT_DONE()	while (!(UCSR1A & 1 << RXC1))
#define SPI_DATA	UDR1

void set_clkm(void);
void board_init(void);

void spi_begin(void);
void spi_off(void);
void spi_init(void);

#endif /* !BOARD_H */
