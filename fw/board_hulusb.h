/*
 * fw/board_hulusb.h - Busware HUL Board-specific functions (for boot loader and application)
 *
 * Written 2017 by Filzmaier Josef
 * Based on fw/board_rzusb written and Copyright 2016 Stefan Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef BOARD_HULUSB_H
#define	BOARD_HULUSB_H

#include <stdbool.h>
#include <stdint.h>

#define LED_RED_PORT		A
#define LED_GREEN_PORT		A
#define LED_RED_BIT		3
#define LED_GREEN_BIT 		4
#define LED_PORT		LED_RED_PORT
#define LED_BIT		  	LED_RED_BIT

#define nRST_RF_PORT		B
#define nRST_RF_BIT	  	5
#define SLP_TR_PORT		B
#define SLP_TR_BIT	  	4

#define SCLK_PORT		B
#define SCLK_BIT	  	1
#define MOSI_PORT		B
#define MOSI_BIT	  	2

#define MISO_PORT		B
#define MISO_BIT	  	3
#define nSS_PORT		B
#define nSS_BIT		  	0
#define IRQ_RF_PORT		D
#define IRQ_RF_BIT	  	4

#define SR_TX_AUTO_CRC_ON	0x04, 0x20, 5
#define SR_CHANNEL		0x08, 0x1f, 0

#define RG_CC_CTRL_1		(0x14)

#define SPI_WAIT_DONE()	while ((SPSR & (1 << SPIF)) == 0)
#define SPI_DATA	SPDR

void set_clkm(void);
void board_init(void);

void led_red(bool on);
void led_green(bool on);

void spi_begin(void);
void spi_off(void);
void spi_init(void);

#ifdef DEBUG
void printStatus(void);
#define PRINT_STATUS() printStatus()
#endif

#endif /* !BOARD_HULUSB_H */
