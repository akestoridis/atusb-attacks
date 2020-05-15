/*
 * fw/flash.c - Board-specific flash functions
 *
 * Written 2011, 2013-2015 by Werner Almesberger
 * Copyright 2011, 2013-2015 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <stdbool.h>
#include <stdint.h>

#include <avr/boot.h>
#include <avr/pgmspace.h>

#include "dfu.h"
#include "board.h"


static uint32_t payload;


static void flash_start(void)
{
	payload = 0;
}


static bool flash_can_write(uint16_t size)
{
	return payload+size <= BOOT_ADDR;
}


static void flash_write(const uint8_t *buf, uint16_t size)
{
	static uint8_t last;
	const uint8_t *p;

	for (p = buf; p != buf+size; p++) {
		if (!(payload & (SPM_PAGESIZE-1))) {
			boot_page_erase(payload);
			boot_spm_busy_wait();
		}

		if (payload & 1)
			boot_page_fill(payload, last | (*p << 8));
		else
			last = *p;
		payload++;

		if (!(payload & (SPM_PAGESIZE-1))) {
			boot_page_write(payload-SPM_PAGESIZE);
			boot_spm_busy_wait();
		}
	}
}


static void flash_end_write(void)
{
	if (payload & (SPM_PAGESIZE-1)) {
		boot_page_write(payload & ~(SPM_PAGESIZE-1));
		boot_spm_busy_wait();
	}
	boot_rww_enable();
}


static uint16_t flash_read(uint8_t *buf, uint16_t size)
{
	uint16_t got = 0;

	while (size && payload != (uint32_t) FLASHEND+1) {
		*buf++ = pgm_read_byte(payload);
		payload++;
		size--;
		got++;
	}
	return got;
}


static const struct dfu_flash_ops flash_ops = {
	.start		= flash_start,
	.can_write	= flash_can_write,
	.write		= flash_write,
	.end_write	= flash_end_write,
	.read		= flash_read,
};


const struct dfu_flash_ops *dfu_flash_ops = &flash_ops;
