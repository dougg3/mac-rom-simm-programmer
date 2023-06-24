/*
 * gpio.c
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

#include "../gpio.h"
#include "nuvoton/NuMicro.h"

/// The GPIO ports available on the M258KE
static GPIO_T * const gpioRegs[] = {
	PA,
	PB,
	PC,
	PD,
	PE,
	PF
};

/** Sets the direction of a GPIO pin.
 *
 * @param pin The pin
 * @param output True if it should be an output, false if it should be an input
 */
void GPIO_SetDirection(GPIOPin pin, bool output)
{
	if (output)
	{
		uint32_t tmp = gpioRegs[pin.port]->MODE;
		tmp &= ~(2UL << 2*pin.pin);
		tmp |= (1UL << 2*pin.pin);
		gpioRegs[pin.port]->MODE = tmp;
	}
	else
	{
		gpioRegs[pin.port]->MODE &= ~(3UL << 2*pin.pin);
	}
}

/** Sets whether an input GPIO pin is pulled up
 *
 * @param pin The pin
 * @param pullup True if it should be pulled up, false if not
 */
void GPIO_SetPullup(GPIOPin pin, bool pullup)
{
	if (pullup)
	{
		uint32_t tmp = gpioRegs[pin.port]->PUSEL;
		tmp &= ~(2UL << 2*pin.pin);
		tmp |= (1UL << 2*pin.pin);
		gpioRegs[pin.port]->PUSEL = tmp;
	}
	else
	{
		gpioRegs[pin.port]->PUSEL &= ~(3UL << 2*pin.pin);
	}
}

/** Turns a GPIO pin on (sets it high)
 *
 * @param pin The pin
 */
void GPIO_SetOn(GPIOPin pin)
{
	gpioRegs[pin.port]->DOUT |= (1 << pin.pin);
}

/** Turns a GPIO pin off (sets it low)
 *
 * @param pin The pin
 */
void GPIO_SetOff(GPIOPin pin)
{
	gpioRegs[pin.port]->DOUT &= ~(1 << pin.pin);
}

/** Toggles a GPIO pin
 *
 * @param pin The pin
 */
void GPIO_Toggle(GPIOPin pin)
{
	gpioRegs[pin.port]->DOUT ^= (1 << pin.pin);
}

/** Reads the input status of a GPIO pin
 *
 * @param pin The pin
 * @return True if it's high, false if it's low
 */
bool GPIO_Read(GPIOPin pin)
{
	return gpioRegs[pin.port]->PIN & (1 << pin.pin);
}
