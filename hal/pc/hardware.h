/*
 * hardware.h
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

#ifndef HAL_PC_HARDWARE_H_
#define HAL_PC_HARDWARE_H_

#include <stdint.h>
#include <unistd.h>

/** Disables interrupts
 *
 */
static inline void DisableInterrupts(void)
{
	// Nothing to do on the PC build, it doesn't matter.
}

/** Enables interrupts
 *
 */
static inline void EnableInterrupts(void)
{
	// Nothing to do on the PC build, it doesn't matter.
}

/** Blocks for the specified number of milliseconds
 *
 * @param ms The number of milliseconds to wait
 */
static inline void DelayMS(uint16_t ms)
{
	usleep(ms * 1000);
}

/** Blocks for the specified number of microseconds
 *
 * @param us The number of microseconds to wait
 */
static inline void DelayUS(uint16_t us)
{
	usleep(us);
}

/** Jumps to the bootloader
 *
 */
static inline void Hardware_EnterBootloader(void)
{
	// Do nothing; the PC simulator doesn't have a bootloader
}

#endif /* HAL_PC_HARDWARE_H_ */
