/*
 * fw/board.h - Board-specific functions and definitions
 *
 * Written 2008-2011, 2013, 2013 by Werner Almesberger
 * Copyright 2008-2011, 2013, 2013 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef BOARD_H
#define	BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include <atusb/atusb.h>

#ifdef ATUSB
#include "board_atusb.h"
#endif
#ifdef RZUSB
#include "board_rzusb.h"
#endif
#ifdef HULUSB
#include "board_hulusb.h"
#endif

#define	SET_2(p, b)	PORT##p |= 1 << (b)
#define	CLR_2(p, b)	PORT##p &= ~(1 << (b))
#define	IN_2(p, b)	DDR##p &= ~(1 << (b))
#define	OUT_2(p, b)	DDR##p |= 1 << (b)
#define	PIN_2(p, b)	((PIN##p >> (b)) & 1)

#define	SET_1(p, b)	SET_2(p, b)
#define	CLR_1(p, b)	CLR_2(p, b)
#define	IN_1(p, b)	IN_2(p, b)
#define	OUT_1(p, b)	OUT_2(p, b)
#define	PIN_1(p, b)	PIN_2(p, b)

#define	SET(n)		SET_1(n##_PORT, n##_BIT)
#define	CLR(n)		CLR_1(n##_PORT, n##_BIT)
#define	IN(n)		IN_1(n##_PORT, n##_BIT)
#define	OUT(n)		OUT_1(n##_PORT, n##_BIT)
#define	PIN(n)		PIN_1(n##_PORT, n##_BIT)


#define	USB_VENDOR	ATUSB_VENDOR_ID
#define	USB_PRODUCT	ATUSB_PRODUCT_ID

#define	DFU_USB_VENDOR	USB_VENDOR
#define	DFU_USB_PRODUCT	USB_PRODUCT


#define	BOARD_MAX_mA	40

#ifdef BOOT_LOADER
#define	NUM_EPS	1
#else
#define	NUM_EPS	2
#endif

#define	HAS_BOARD_SERNUM

extern uint8_t board_sernum[42];
extern uint8_t irq_serial;


void reset_rf(void);
void reset_cpu(void);
uint8_t read_irq(void);
void slp_tr(void);

void led(bool on);
void panic(void);

uint64_t timer_read(void);
void timer_init(void);

bool gpio(uint8_t port, uint8_t data, uint8_t dir, uint8_t mask, uint8_t *res);
void gpio_cleanup(void);

void get_sernum(void);

void board_app_init(void);

uint8_t reg_read(uint8_t reg);
uint8_t subreg_read(uint8_t address, uint8_t mask, uint8_t position);
void reg_write(uint8_t reg, uint8_t value);
void subreg_write(uint8_t address, uint8_t mask, uint8_t position, uint8_t value);
void change_state(uint8_t new);

#endif /* !BOARD_H */
