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
#include "gpio_hw.h"
#include "gpio_sim.h"

static uint32_t directionReg[NUM_SIM_GPIO_PORTS];
static uint32_t pullupReg[NUM_SIM_GPIO_PORTS];
static uint32_t outputReg[NUM_SIM_GPIO_PORTS];

/** Sets the direction of a GPIO pin.
 *
 * @param pin The pin
 * @param output True if it should be an output, false if it should be an input
 */
void GPIO_SetDirection(GPIOPin pin, bool output)
{
	if (pin.port < NUM_SIM_GPIO_PORTS)
	{
		// Figure out if any state is actually changing, and if so, modify it
		bool alreadyOutput = directionReg[pin.port] & (1UL << pin.pin);
		if (output != alreadyOutput)
		{
			if (output)
			{
				directionReg[pin.port] |= (1UL << pin.pin);
				// We just became an output, so make sure any sim devices are updated with
				// the new driven value
				GPIOSim_WritePin(pin, outputReg[pin.port] & (1UL << pin.pin));
			}
			else
			{
				directionReg[pin.port] &= ~(1UL << pin.pin);
				// We just became an input, there's no state to update really
			}
		}
	}
}

/** Sets whether an input GPIO pin is pulled up
 *
 * @param pin The pin
 * @param pullup True if it should be pulled up, false if not
 */
void GPIO_SetPullup(GPIOPin pin, bool pullup)
{
	if (pin.port < NUM_SIM_GPIO_PORTS)
	{
		// Update the pullup register. No need to update any other state,
		// it simply affects the readback simulation
		if (pullup)
		{
			pullupReg[pin.port] |= (1UL << pin.pin);
		}
		else
		{
			pullupReg[pin.port] &= ~(1UL << pin.pin);
		}
	}
}

/** Turns a GPIO pin on (sets it high)
 *
 * @param pin The pin
 */
void GPIO_SetOn(GPIOPin pin)
{
	if (pin.port < NUM_SIM_GPIO_PORTS)
	{
		// Determine if we're actually changing the current state, which would
		// require us to update simulated devices
		bool isOutput = directionReg[pin.port] & (1UL << pin.pin);
		bool curOutputValue = outputReg[pin.port] & (1UL << pin.pin);
		bool needsNotification = isOutput && !curOutputValue;
		outputReg[pin.port] |= (1UL << pin.pin);
		if (needsNotification)
		{
			GPIOSim_WritePin(pin, true);
		}
	}
}

/** Turns a GPIO pin off (sets it low)
 *
 * @param pin The pin
 */
void GPIO_SetOff(GPIOPin pin)
{
	if (pin.port < NUM_SIM_GPIO_PORTS)
	{
		// Determine if we're actually changing the current state, which would
		// require us to update simulated devices
		bool isOutput = directionReg[pin.port] & (1UL << pin.pin);
		bool curOutputValue = outputReg[pin.port] & (1UL << pin.pin);
		bool needsNotification = isOutput && curOutputValue;
		outputReg[pin.port] &= ~(1UL << pin.pin);
		if (needsNotification)
		{
			GPIOSim_WritePin(pin, false);
		}
	}
}

/** Toggles a GPIO pin
 *
 * @param pin The pin
 */
void GPIO_Toggle(GPIOPin pin)
{
	if (pin.port < NUM_SIM_GPIO_PORTS)
	{
		// Figure out whether we are turning it on or off, and forward on
		if (outputReg[pin.port] & (1UL << pin.pin))
		{
			GPIO_SetOff(pin);
		}
		else
		{
			GPIO_SetOn(pin);
		}
	}
}

/** Reads the input status of a GPIO pin
 *
 * @param pin The pin
 * @return True if it's high, false if it's low
 */
bool GPIO_Read(GPIOPin pin)
{
	if (pin.port < NUM_SIM_GPIO_PORTS)
	{
		// If we are currently configured as an output, just read back the output value.
		// We'll pretend that's what our "simulated" hardware does.
		if (directionReg[pin.port] & (1UL << pin.pin))
		{
			return outputReg[pin.port] & (1UL << pin.pin);
		}
		else
		{
			// If we're configured as an input, read back the value from any simulators
			GPIOSimValue readback = GPIOSim_ReadPin(pin);
			switch (readback)
			{
			case GPIOSimNotDriving:
			default:
				if (pullupReg[pin.port] & (1UL << pin.pin))
				{
					// If the pull-up is active and nothing is driving the pin,
					// read back as high
					return true;
				}
				else
				{
					// If the pull-up is not active and nothing is driving the pin,
					// it's floating. For the purposes of our simulation, let's return low.
					// We could return random values if we wanted...
					// TODO: assertion with current GPIO state, stack trace?
					return false;
				}
			case GPIOSimDrivingLow:
				return false;
			case GPIOSimDrivingHigh:
				return true;
			case GPIOSimDrivingConflict:
				// If it's being driven both high and low, bad things will happen.
				// For the purposes of our simulation, read back as high.
				// TODO: assertion with current GPIO state, stack trace?
				return true;
			}
		}
	}
	else
	{
		// Read values as low if an invalid port is passed in
		return false;
	}
}
