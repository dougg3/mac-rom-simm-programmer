/*
 * gpio_sim.h
 *
 *  Created on: Jul 31, 2021
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

#ifndef GPIO_SIM_H
#define GPIO_SIM_H

#include "../gpio.h"

/// Enum describing the various ways a simulated GPIO device can be driving a GPIO pin
typedef enum GPIOSimValue
{
	/// The simulated GPIO is not driving this pin
	GPIOSimNotDriving,
	/// The simulated GPIO is driven low
	GPIOSimDrivingLow,
	/// The simulated GPIO is driven high
	GPIOSimDrivingHigh,
	/// It's being driven both high and low, uh oh.
	/// Actual devices don't return this value, but it's used as a return value for GPIOSim_ReadPin
	GPIOSimDrivingConflict
} GPIOSimValue;

typedef struct GPIOSimDevice GPIOSimDevice;

typedef struct GPIOSimDeviceFunctions
{
	void (*drivePin)(GPIOSimDevice *device, GPIOPin pin, bool high);
	GPIOSimValue (*readPin)(GPIOSimDevice *device, GPIOPin pin);
} GPIOSimDeviceFunctions;

struct GPIOSimDevice
{
	GPIOSimDeviceFunctions const *functions;
	GPIOSimDevice *next;
};

void GPIOSim_AddDevice(GPIOSimDevice *device);
void GPIOSim_WritePin(GPIOPin pin, bool high);
GPIOSimValue GPIOSim_ReadPin(GPIOPin pin);

#endif // GPIO_SIM_H
