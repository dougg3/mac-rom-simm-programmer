/*
 * gpio_sim.c
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

#include "gpio_sim.h"
#include <stdlib.h>

static GPIOSimDevice *gpioSims = NULL;

void GPIOSim_AddDevice(GPIOSimDevice *device)
{
	// We keep a super simple linked list, ordering doesn't matter.
	// For simplicity, prepend this device to the linked list.
	device->next = gpioSims;
	gpioSims = device;
}

void GPIOSim_WritePin(GPIOPin pin, bool high)
{
	// Give each simulated device a chance to handle this event
	GPIOSimDevice *device = gpioSims;
	while (device)
	{
		if (device->functions && device->functions->drivePin)
		{
			device->functions->drivePin(device, pin, high);
		}
		device = device->next;
	}
}

GPIOSimValue GPIOSim_ReadPin(GPIOPin pin)
{
	GPIOSimValue retval = GPIOSimNotDriving;

	// Give each simulated device a chance to handle this read
	GPIOSimDevice *device = gpioSims;
	while (device)
	{
		if (device->functions && device->functions->readPin)
		{
			GPIOSimValue val = device->functions->readPin(device, pin);
			switch (val)
			{
			case GPIOSimNotDriving:
			default:
				// Do nothing if we're not driving this pin
				break;
			case GPIOSimDrivingLow:
				// If we haven't found anything driving it yet, we can be the driver
				if (retval == GPIOSimNotDriving)
				{
					retval = GPIOSimDrivingLow;
				}
				// If we already found something driving it high, mark the error condition
				else if (retval == GPIOSimDrivingHigh)
				{
					retval = GPIOSimDrivingConflict;
				}
				break;
			case GPIOSimDrivingHigh:
				// If we haven't found anything driving it yet, we can be the driver
				if (retval == GPIOSimNotDriving)
				{
					retval = GPIOSimDrivingHigh;
				}
				// If we already found something driving it low, mark the error condition
				else if (retval == GPIOSimDrivingLow)
				{
					retval = GPIOSimDrivingConflict;
				}
				break;
			}
		}
		device = device->next;
	}

	// All done, return retval
	return retval;
}
