/*
 * fw/attacks/attack_00.c - Attack function with ID 00
 *
 * Written 2020 by Dimitrios-Georgios Akestoridis
 * Copyright 2020 Dimitrios-Georgios Akestoridis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdbool.h>

#include "attack.h"


/* Do not jam or spoof any packets; equivalent to the original firmware */
bool attack(void)
{
	return 0;
}
