/*
 * fw/ep0.c - EP0 extension protocol
 *
 * Written 2008-2011, 2013 by Werner Almesberger
 * Copyright 2008-2011, 2013 Werner Almesberger
 * Copyright 2015-2016 Stefan Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include <avr/eeprom.h>

#define F_CPU   8000000UL
#include <util/delay.h>

#ifndef NULL
#define NULL 0
#endif

#include "usb.h"
#include "dfu.h"

#include "at86rf230.h"
#include "atusb/ep0.h"
#include "version.h"
#include "board.h"
#include "sernum.h"
#include "spi.h"
#include "mac.h"

#ifdef ATUSB
#define	HW_TYPE		ATUSB_HW_TYPE_110131
#endif

#ifdef RZUSB
#define	HW_TYPE		ATUSB_HW_TYPE_RZUSB
#endif

#ifdef HULUSB
#define HW_TYPE		ATUSB_HW_TYPE_HULUSB
#endif

#ifdef DEBUG
#include "uart.h"
#include <stdio.h>
#define debug(FORMAT,args...) printf(FORMAT,##args)
#define error(FORMAT,args...) printf(FORMAT,##args)
#else
#define debug(...)
#define error(...)
#endif


static const uint8_t id[] = { EP0ATUSB_MAJOR, EP0ATUSB_MINOR, HW_TYPE };
static uint8_t buf[MAX_PSDU+3]; /* command, PHDR, and LQI */
static uint8_t size;


static void do_eeprom_write(void *user)
{
	int i;

	for (i = 0; i < size; i++)
		eeprom_update_byte((uint8_t*)i, buf[i]);
}

static void do_buf_write(void *user)
{
	uint8_t i;

	spi_begin();
	for (i = 0; i != size; i++)
		spi_send(buf[i]);
	spi_end();
}


#define	BUILD_OFFSET	7	/* '#' plus "65535" plus ' ' */


static bool my_setup(const struct setup_request *setup)
{
	uint16_t req = setup->bmRequestType | setup->bRequest << 8;
	unsigned tmp;
	uint8_t i;
	uint64_t tmp64;

	switch (req) {
	case ATUSB_FROM_DEV(ATUSB_ID):
		debug("ATUSB_ID\n");
		if (setup->wLength > 3)
			return 0;
		usb_send(&eps[0], id, setup->wLength, NULL, NULL);
		return 1;
	case ATUSB_FROM_DEV(ATUSB_BUILD):
		debug("ATUSB_BUILD\n");
		tmp = build_number;
		for (i = BUILD_OFFSET-2; tmp; i--) {
			buf[i] = (tmp % 10)+'0';
			tmp /= 10;
		}
		buf[i] = '#';
		buf[BUILD_OFFSET-1] = ' ';
		for (size = 0; build_date[size]; size++)
			buf[BUILD_OFFSET+size] = build_date[size];
		size += BUILD_OFFSET-i;
		if (size > setup->wLength)
			return 0;
		usb_send(&eps[0], buf+i, size, NULL, NULL);
		return 1;

	case ATUSB_TO_DEV(ATUSB_RESET):
		debug("ATUSB_RESET\n");
		reset_cpu();
		while (1);

	case ATUSB_TO_DEV(ATUSB_RF_RESET):
		debug("ATUSB_RF_RESET\n");
		reset_rf();
		mac_reset();
		//ep_send_zlp(EP_CTRL);
		return 1;

	case ATUSB_FROM_DEV(ATUSB_POLL_INT):
		debug("ATUSB_POLL_INT\n");
		if (setup->wLength < 1)
			return 0;
		*buf = read_irq();
		usb_send(&eps[0], buf, 1, NULL, NULL);
		return 1;

	case ATUSB_FROM_DEV(ATUSB_TIMER):
		debug("ATUSB_TIMER\n");
		size = setup->wLength;
		if (size > sizeof(tmp64))
			size = sizeof(tmp64);
		tmp64 = timer_read();
		memcpy(buf, &tmp64, sizeof(tmp64));
		usb_send(&eps[0], buf, size, NULL, NULL);
		return 1;

	case ATUSB_FROM_DEV(ATUSB_GPIO):
		debug("ATUSB_GPIO\n");
		if (setup->wLength < 3)
			return 0;
		if (!gpio(setup->wIndex, setup->wValue, setup->wValue >> 8,
		    setup->wIndex >> 8, buf))
			return 0;
		usb_send(&eps[0], buf, 3, NULL, NULL);
		return 1;
	case ATUSB_TO_DEV(ATUSB_GPIO_CLEANUP):
		gpio_cleanup();
		return 1;

	case ATUSB_TO_DEV(ATUSB_SLP_TR):
		debug("ATUSB_SLP_TR\n");
		slp_tr();
		return 1;

	case ATUSB_TO_DEV(ATUSB_REG_WRITE):
		debug("ATUSB_REG_WRITE\n");
		spi_begin();
		spi_send(AT86RF230_REG_WRITE | setup->wIndex);
		spi_send(setup->wValue);
		spi_end();
		//ep_send_zlp(EP_CTRL);
		return 1;
	case ATUSB_FROM_DEV(ATUSB_REG_READ):
		debug("ATUSB_REG_READ\n");
		spi_begin();
		spi_send(AT86RF230_REG_READ | setup->wIndex);
		*buf = spi_recv();
		spi_end();
		usb_send(&eps[0], buf, 1, NULL, NULL);
		return 1;

	case ATUSB_TO_DEV(ATUSB_BUF_WRITE):
		debug("ATUSB_BUF_WRITE\n");
		if (setup->wLength < 1)
			return 0;
		if (setup->wLength > MAX_PSDU)
			return 0;
		buf[0] = AT86RF230_BUF_WRITE;
		buf[1] = setup->wLength;
		size = setup->wLength+2;
		usb_recv(&eps[0], buf+2, setup->wLength, do_buf_write, NULL);
		return 1;
	case ATUSB_FROM_DEV(ATUSB_BUF_READ):
		debug("ATUSB_BUF_READ\n");
		if (setup->wLength < 2)			/* PHR+LQI */
			return 0;
		if (setup->wLength > MAX_PSDU+2)	/* PHR+PSDU+LQI */
			return 0;
		spi_begin();
		spi_send(AT86RF230_BUF_READ);
		size = spi_recv();
		if (size >= setup->wLength)
			size = setup->wLength-1;
		for (i = 0; i != size+1; i++)
			buf[i] = spi_recv();
		spi_end();
		usb_send(&eps[0], buf, size+1, NULL, NULL);
		return 1;

	case ATUSB_TO_DEV(ATUSB_SRAM_WRITE):
		debug("ATUSB_SRAM_WRITE\n");
		if (setup->wIndex > SRAM_SIZE)
			return 0;
		if (setup->wIndex+setup->wLength > SRAM_SIZE)
			return 0;
		buf[0] = AT86RF230_SRAM_WRITE;
		buf[1] = setup->wIndex;
		size = setup->wLength+2;
		usb_recv(&eps[0], buf+2, setup->wLength, do_buf_write, NULL);
		return 1;
	case ATUSB_FROM_DEV(ATUSB_SRAM_READ):
		debug("ATUSB_SRAM_READ\n");
		if (setup->wIndex > SRAM_SIZE)
			return 0;
		if (setup->wIndex+setup->wLength > SRAM_SIZE)
			return 0;
		spi_begin();
		spi_send(AT86RF230_SRAM_READ);
		spi_send(setup->wIndex);
		for (i = 0; i != setup->wLength; i++)
			buf[i] = spi_recv();
		spi_end();
		usb_send(&eps[0], buf, setup->wLength, NULL, NULL);
		return 1;

	case ATUSB_TO_DEV(ATUSB_SPI_WRITE):
		size = setup->wLength+2;
		if (size > sizeof(buf))
			return 0;
		buf[0] = setup->wValue;
		buf[1] = setup->wIndex;
		if (setup->wLength)
			usb_recv(&eps[0], buf+2, setup->wLength,
			    do_buf_write, NULL);
		else
			do_buf_write(NULL);
		return 1;
	case ATUSB_FROM_DEV(ATUSB_SPI_WRITE2_SYNC):
		spi_begin();
		spi_send(setup->wValue);
		spi_send(setup->wIndex);
		spi_end();
		buf[0] = irq_serial;
		if (setup->wLength)
			usb_send(&eps[0], buf, 1, NULL, NULL);
		return 1;

	case ATUSB_FROM_DEV(ATUSB_SPI_READ1):
	case ATUSB_FROM_DEV(ATUSB_SPI_READ2):
		spi_begin();
		spi_send(setup->wValue);
		if (req == ATUSB_FROM_DEV(ATUSB_SPI_READ2))
			spi_send(setup->wIndex);
		for (i = 0; i != setup->wLength; i++)
			buf[i] = spi_recv();
		spi_end();
		usb_send(&eps[0], buf, setup->wLength, NULL, NULL);
		return 1;

	case ATUSB_TO_DEV(ATUSB_RX_MODE):
		return mac_rx(setup->wValue);
	case ATUSB_TO_DEV(ATUSB_TX):
		return mac_tx(setup->wValue, setup->wIndex, setup->wLength);
	case ATUSB_TO_DEV(ATUSB_EUI64_WRITE):
		debug("ATUSB_EUI64_WRITE\n");
		usb_recv(&eps[0], buf, setup->wLength, do_eeprom_write, NULL);
		_delay_ms(100);
		reset_cpu();
		return 1;

	case ATUSB_FROM_DEV(ATUSB_EUI64_READ):
		debug("ATUSB_EUI64_READ\n");
		eeprom_read_block(buf, (const void*)0, 8);
		usb_send(&eps[0], buf, 8, NULL, NULL);
		return 1;

	default:
		error("Unrecognized SETUP: 0x%02x 0x%02x ...\n",
		    setup->bmRequestType, setup->bRequest);
		return 0;
	}
}


static bool my_dfu_setup(const struct setup_request *setup)
{
	switch (setup->bmRequestType | setup->bRequest << 8) {
	case DFU_TO_DEV(DFU_DETACH):
		/* @@@ should use wTimeout */
		dfu.state = appDETACH;
		return 1;
	default:
		return dfu_setup_common(setup);
	}
}


static void my_set_interface(int nth)
{
	if (nth) {
		user_setup = my_dfu_setup;
		user_get_descriptor = dfu_my_descr;
		dfu.state = appIDLE;
	} else {
		user_setup = my_setup;
		user_get_descriptor = sernum_get_descr;
	}
}


static void my_reset(void)
{
	if (dfu.state == appDETACH)
		reset_cpu();
}


void ep0_init(void)
{
	user_setup = my_setup;
	user_set_interface = my_set_interface;
	my_set_interface(0);
	user_reset = my_reset;
}
