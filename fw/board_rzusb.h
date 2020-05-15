/*
 * fw/board_rzusb.h - RZUSB Board-specific functions and definitions
 *
 * Written 2016 by Stefan Schmidt
 * Copyright 2016 Stefan Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef BOARD_RZUSB_H
#define	BOARD_RZUSB_H

#include <stdbool.h>
#include <stdint.h>

#define	LED_PORT	D
#define	LED_BIT		  7
#define	nRST_RF_PORT	B
#define	nRST_RF_BIT	  5
#define	SLP_TR_PORT	B
#define	SLP_TR_BIT	  4

#define SCLK_PORT	B
#define SCLK_BIT	  1
#define	MOSI_PORT	B
#define	MOSI_BIT	  2

#define	MISO_PORT	B
#define	MISO_BIT	  3
#define	nSS_PORT	B
#define	nSS_BIT		  0
#define	IRQ_RF_PORT	D
#define	IRQ_RF_BIT	  4

#define SPI_WAIT_DONE()	while ((SPSR & (1 << SPIF)) == 0)
#define SPI_DATA	SPDR

void set_clkm(void);
void board_init(void);

void spi_begin(void);
void spi_off(void);
void spi_init(void);

#endif /* !BOARD_H */
