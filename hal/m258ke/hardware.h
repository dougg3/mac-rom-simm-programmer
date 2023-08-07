/*
 * hardware.h
 *
 *  Created on: Jun 19, 2023
 *      Author: Doug
 *
 * Copyright (C) 2011-2023 Doug Brown
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef HAL_M258KE_HARDWARE_H_
#define HAL_M258KE_HARDWARE_H_

#include <stdint.h>
#include <stdbool.h>
#include "nuvoton/NuMicro.h"

/** Disables interrupts
 *
 */
static inline void DisableInterrupts(void)
{
	__disable_irq();
}

/** Enables interrupts
 *
 */
static inline void EnableInterrupts(void)
{
	__enable_irq();
}

/** Blocks for the specified number of microseconds
 *
 * @param us The number of microseconds to wait
 */
static inline void DelayUS(uint32_t us)
{
	const uint32_t startTime = TIMER0->CNT & 0xFFFFFFUL;
	uint32_t nowTime;
	do
	{
		nowTime = TIMER0->CNT & 0xFFFFFFUL;
	} while (((nowTime - startTime) & 0xFFFFFFUL) < us);
}

/** Blocks for the specified number of milliseconds
 *
 * @param ms The number of milliseconds to wait
 */
static inline void DelayMS(uint32_t ms)
{
	DelayUS(ms * 1000UL);
}

#endif /* HAL_M258KE_HARDWARE_H_ */
