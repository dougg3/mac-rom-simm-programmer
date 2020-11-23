/*
 * board_hw.h
 *
 *  Created on: Nov 14, 2020
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

#ifndef HAL_AT90USB646_BOARD_HW_H_
#define HAL_AT90USB646_BOARD_HW_H_

#include "gpio_hw.h"
#include "../gpio.h"

/** Gets the GPIO pin on the board that controls the status LED
 *
 * @return The status LED pin
 */
static inline GPIOPin Board_LEDPin(void)
{
	return GPIO_PIN(GPIOD, 7);
}

#endif /* HAL_AT90USB646_BOARD_HW_H_ */
