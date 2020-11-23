/*
 * gpio.h
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

#ifndef HAL_GPIO_H_
#define HAL_GPIO_H_

#include <stdint.h>
#include <stdbool.h>

/// Creates a temporary GPIOPin struct. Used when assigning to a GPIOPin variable.
#define GPIO_PIN(port, pin)		((GPIOPin){port, pin})
/// A NULL GPIO pin
#define GPIO_PIN_NULL			((GPIOPin){0xFF, 0xFF})

/// The GPIO pin struct
typedef struct GPIOPin
{
	/// The port the pin belongs to
	uint8_t port;
	/// The index of the pin on the port
	uint8_t pin;
} GPIOPin;

void GPIO_SetDirection(GPIOPin pin, bool output);
void GPIO_SetPullup(GPIOPin pin, bool pullup);
void GPIO_SetOn(GPIOPin pin);
void GPIO_SetOff(GPIOPin pin);
void GPIO_Toggle(GPIOPin pin);
bool GPIO_Read(GPIOPin pin);

/** Sets whether a GPIO pin is outputting high or low
 *
 * @param pin The pin
 * @param on True if it's high, false if it's low
 */
static inline void GPIO_Set(GPIOPin pin, bool on)
{
	on ? GPIO_SetOn(pin) : GPIO_SetOff(pin);
}

/** Determines if a GPIO pin is null
 *
 * @param pin The pin
 * @return True if it's null, false if not
 */
static inline bool GPIO_IsNull(GPIOPin pin)
{
	return pin.pin == 0xFF && pin.port == 0xFF;
}

#endif /* HAL_GPIO_H_ */
