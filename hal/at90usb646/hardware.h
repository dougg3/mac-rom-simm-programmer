/*
 * hardware.h
 *
 *  Created on: Dec 4, 2011
 *      Author: Doug
 *
 * Copyright (C) 2011-2020 Doug Brown
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

#ifndef HAL_AT90USB646_HARDWARE_H_
#define HAL_AT90USB646_HARDWARE_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "../../util.h"
#include "../usbcdc.h"

/** Disables interrupts
 *
 */
static inline void DisableInterrupts(void)
{
	cli();
}

/** Enables interrupts
 *
 */
static inline void EnableInterrupts(void)
{
	sei();
}

/** Blocks for the specified number of milliseconds
 *
 * @param ms The number of milliseconds to wait
 */
static inline void DelayMS(uint16_t ms)
{
	_delay_ms(ms);
}

/** Blocks for the specified number of microseconds
 *
 * @param us The number of microseconds to wait
 */
static inline void DelayUS(uint16_t us)
{
	_delay_us(us);
}

/** Jumps to the bootloader
 *
 */
static inline void Hardware_EnterBootloader(void)
{
	// Insert a small delay to ensure that it arrives before rebooting.
	DelayMS(1000);

	// Done with the USB interface -- the bootloader will re-initialize it.
	USBCDC_Disable();

	// Disable interrupts so nothing weird happens...
	DisableInterrupts();

	// Wait a little bit to let everything settle and let the program
	// close the port after the USB disconnect
	DelayMS(2000);

	// And, of course, go into the bootloader.
	__asm__ __volatile__ ( "jmp 0xE000" );
}

#endif /* HAL_AT90USB646_HARDWARE_H_ */
