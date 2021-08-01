/*
 * board.c
 *
 *  Created on: Jul 17, 2021
 *      Author: Doug
 *
 * Copyright (C) 2011-2021 Doug Brown
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "board_hw.h"
#include "flash_4mbit.h"

/** Initializes any board hardware-specific stuff
 *
 */
void Board_Init(void)
{
	// Nothing to do on PC

	// TODO: Figure out a way to make this dynamically configurable. But for now...
	GPIOPin cs = {GPIOMISC, 4};
	GPIOPin oe = {GPIOMISC, 5};
	GPIOPin we = {GPIOMISC, 6};
	static Flash4MBit ics[4];
	for (uint8_t i = 0; i < 4; i++)
	{
		Flash4MBit_Init(&ics[i], Flash_SST39SF040);
		GPIOSim_AddDevice(&ics[i].base);
		Flash4MBit_SetControlPins(&ics[i], cs, oe, we);
		for (uint8_t j = 0; j < FLASH_4MBIT_DATA_PINS; j++)
		{
			GPIOPin dataPin = {GPIODATA, i * 8 + j};
			Flash4MBit_SetDataPin(&ics[i], j, dataPin);
		}
		for (uint8_t j = 0; j < FLASH_4MBIT_ADDR_PINS; j++)
		{
			GPIOPin addrPin = {GPIOADDR, j};
			Flash4MBit_SetAddressPin(&ics[i], j, addrPin);
		}
	}
}

/** Determines if a brownout was detected at startup
 *
 * @return True if a brownout was detected
 */
bool Board_BrownoutDetected(void)
{
	// There is no such thing on the PC simulator
	return false;
}
