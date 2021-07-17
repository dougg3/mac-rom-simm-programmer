/*
 * gpio.c
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

#include "../gpio.h"

/** Sets the direction of a GPIO pin.
 *
 * @param pin The pin
 * @param output True if it should be an output, false if it should be an input
 */
void GPIO_SetDirection(GPIOPin pin, bool output)
{
	// TODO: Modify direction
	(void)pin; (void)output;
}

/** Sets whether an input GPIO pin is pulled up
 *
 * @param pin The pin
 * @param pullup True if it should be pulled up, false if not
 */
void GPIO_SetPullup(GPIOPin pin, bool pullup)
{
	// TODO: Modify pullup
	(void)pin; (void)pullup;
}

/** Turns a GPIO pin on (sets it high)
 *
 * @param pin The pin
 */
void GPIO_SetOn(GPIOPin pin)
{
	// TODO: Turn on
	(void)pin;
}

/** Turns a GPIO pin off (sets it low)
 *
 * @param pin The pin
 */
void GPIO_SetOff(GPIOPin pin)
{
	// TODO: Turn off
	(void)pin;
}

/** Toggles a GPIO pin
 *
 * @param pin The pin
 */
void GPIO_Toggle(GPIOPin pin)
{
	// TODO: Toggle
	(void)pin;
}

/** Reads the input status of a GPIO pin
 *
 * @param pin The pin
 * @return True if it's high, false if it's low
 */
bool GPIO_Read(GPIOPin pin)
{
	// TODO: Read
	(void)pin;
	return false;
}
