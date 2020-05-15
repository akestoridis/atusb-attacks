/*
 * fw/mac.h - HardMAC functions
 *
 * Written 2011, 2013 by Werner Almesberger
 * Copyright 2011, 2013 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef MAC_H
#define	MAC_H

#include <stdbool.h>
#include <stdint.h>


extern bool (*mac_irq)(void);

bool mac_rx(int on);
bool mac_tx(uint16_t flags, uint8_t seq, uint16_t len);
void mac_reset(void);

#endif /* !MAC_H */
