/*
 * parallel_bus.c
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

#include "../parallel_bus.h"
#include "../../util.h"
#include "../gpio.h"
#include "gpio_hw.h"

/// The index of the CS pin
#define FLASH_CS_PIN						4
/// The index of the OE pin
#define FLASH_OE_PIN						5
/// The index of the WE pin
#define FLASH_WE_PIN						6
/// The index of the highest address line on the parallel bus
#define PARALLEL_BUS_HIGHEST_ADDRESS_LINE	20

/// The /WE pin for the parallel bus
static const GPIOPin flashWEPin = {GPIOMISC, FLASH_WE_PIN};
/// The /OE pin for the parallel bus
static const GPIOPin flashOEPin = {GPIOMISC, FLASH_OE_PIN};
/// The /CS pin for the flash chip
static const GPIOPin flashCSPin = {GPIOMISC, FLASH_CS_PIN};

/** Initializes the 32-bit data/21-bit address parallel bus.
 *
 */
void ParallelBus_Init(void)
{
	// Currently we don't emulate the MCP23S17 here like the AVR build.
	// We could theoretically change this to emulate it if we wanted...

	// Configure all address lines as outputs, outputting address 0
	ParallelBus_SetAddressDir((1UL << (PARALLEL_BUS_HIGHEST_ADDRESS_LINE + 1)) - 1);
	ParallelBus_SetAddress(0);

	// Set all data lines to pulled-up inputs
	ParallelBus_SetDataDir(0);
	ParallelBus_SetDataPullups(0xFFFFFFFFUL);

	// Control lines
	ParallelBus_SetCSDir(true);
	ParallelBus_SetOEDir(true);
	ParallelBus_SetWEDir(true);

	// Default to only CS asserted
	ParallelBus_SetWE(true);
	ParallelBus_SetOE(true);
	ParallelBus_SetCS(false);
}

/** Sets the address being output on the 21-bit address bus
 *
 * @param address The address
 */
void ParallelBus_SetAddress(uint32_t address)
{
	for (int i = 0; i <= PARALLEL_BUS_HIGHEST_ADDRESS_LINE; i++)
	{
		GPIOPin addrPin = {GPIOADDR, i};
		GPIO_Set(addrPin, address & (1UL << i));
	}
}

/** Sets the output data on the 32-bit data bus
 *
 * @param data The data
 */
void ParallelBus_SetData(uint32_t data)
{
	for (int i = 0; i < 32; i++)
	{
		GPIOPin dataPin = {GPIODATA, i};
		GPIO_Set(dataPin, data & (1UL << i));
	}
}

/** Sets the output value of the CS pin
 *
 * @param high True if it should be high, false if low
 */
void ParallelBus_SetCS(bool high)
{
	GPIO_Set(flashCSPin, high);
}

/** Sets the output value of the OE pin
 *
 * @param high True if it should be high, false if low
 */
void ParallelBus_SetOE(bool high)
{
	GPIO_Set(flashOEPin, high);
}

/** Sets the output value of the WE pin
 *
 * @param high True if it should be high, false if low
 */
void ParallelBus_SetWE(bool high)
{
	GPIO_Set(flashWEPin, high);
}

/** Sets which pins on the 21-bit address bus should be outputs
 *
 * @param outputs Mask of pins that should be outputs. 1 = output, 0 = input
 *
 * Typically the address pins will be outputs. This flexibility is provided in
 * case we want to do electrical testing.
 */
void ParallelBus_SetAddressDir(uint32_t outputs)
{
	for (int i = 0; i <= PARALLEL_BUS_HIGHEST_ADDRESS_LINE; i++)
	{
		GPIOPin addrPin = {GPIOADDR, i};
		GPIO_SetDirection(addrPin, outputs & (1UL << i));
	}
}

/** Sets which pins on the 32-bit data bus should be outputs
 *
 * @param outputs Mask of pins that should be outputs. 1 = output, 0 = input
 *
 * Typically all pins would be set as inputs or outputs, and it's automatically
 * handled by the read/write cycle functions. This function exists mainly for
 * test purposes.
 */
void ParallelBus_SetDataDir(uint32_t outputs)
{
	for (int i = 0; i < 32; i++)
	{
		GPIOPin dataPin = {GPIODATA, i};
		GPIO_SetDirection(dataPin, outputs & (1UL << i));
	}
}

/** Sets the direction of the CS pin
 *
 * @param output True if it's an output, false if it's an input
 *
 * Typically this pin will be an output. This flexibility is provided in case
 * we want to do electrical testing.
 */
void ParallelBus_SetCSDir(bool output)
{
	GPIO_SetDirection(flashCSPin, output);
}

/** Sets the direction of the OE pin
 *
 * @param output True if it's an output, false if it's an input
 *
 * Typically this pin will be an output. This flexibility is provided in case
 * we want to do electrical testing.
 */
void ParallelBus_SetOEDir(bool output)
{
	GPIO_SetDirection(flashOEPin, output);
}

/** Sets the direction of the WE pin
 *
 * @param output True if it's an output, false if it's an input
 *
 * Typically this pin will be an output. This flexibility is provided in case
 * we want to do electrical testing.
 */
void ParallelBus_SetWEDir(bool output)
{
	GPIO_SetDirection(flashWEPin, output);
}

/** Sets which pins on the 21-bit address bus should be pulled up (if inputs)
 *
 * @param pullups Mask of pins that should be pullups.
 *
 * This would typically only be used for testing. Under normal operation, the
 * address bus will be outputting, so the pullups are irrelevant.
 */
void ParallelBus_SetAddressPullups(uint32_t pullups)
{
	for (int i = 0; i <= PARALLEL_BUS_HIGHEST_ADDRESS_LINE; i++)
	{
		GPIOPin addrPin = {GPIOADDR, i};
		GPIO_SetPullup(addrPin, pullups & (1UL << i));
	}
}

/** Sets which pins on the 32-bit data bus should be pulled up (if inputs)
 *
 * @param pullups Mask of pins that should be pullups.
 *
 * Typically these will be enabled in order to provide a default value if a
 * chip isn't responding properly. Sometimes it's useful to customize it during
 * testing though.
 */
void ParallelBus_SetDataPullups(uint32_t pullups)
{
	for (int i = 0; i < 32; i++)
	{
		GPIOPin dataPin = {GPIODATA, i};
		GPIO_SetPullup(dataPin, pullups & (1UL << i));
	}
}

/** Sets whether the CS pin is pulled up, if it's an input.
 *
 * @param pullup True if the CS pin should be pulled up, false if not
 *
 * This would typically only be used for testing. Under normal operation, this
 * pin will be set as an output, so the pullup state is irrelevant.
 */
void ParallelBus_SetCSPullup(bool pullup)
{
	GPIO_SetPullup(flashCSPin, pullup);
}

/** Sets whether the OE pin is pulled up, if it's an input.
 *
 * @param pullup True if the OE pin should be pulled up, false if not
 *
 * This would typically only be used for testing. Under normal operation, this
 * pin will be set as an output, so the pullup state is irrelevant.
 */
void ParallelBus_SetOEPullup(bool pullup)
{
	GPIO_SetPullup(flashOEPin, pullup);
}

/** Sets whether the WE pin is pulled up, if it's an input.
 *
 * @param pullup True if the WE pin should be pulled up, false if not
 *
 * This would typically only be used for testing. Under normal operation, this
 * pin will be set as an output, so the pullup state is irrelevant.
 */
void ParallelBus_SetWEPullup(bool pullup)
{
	GPIO_SetPullup(flashWEPin, pullup);
}

/** Reads the current data on the address bus.
 *
 * @return The address bus readback
 *
 * This would typically only be used for testing. Under normal operation, the
 * address bus will be outputting, so the readback is irrelevant.
 */
uint32_t ParallelBus_ReadAddress(void)
{
	uint32_t readback = 0;
	for (int i = 0; i <= PARALLEL_BUS_HIGHEST_ADDRESS_LINE; i++)
	{
		GPIOPin addrPin = {GPIOADDR, i};
		if (GPIO_Read(addrPin))
		{
			readback |= (1UL << i);
		}
	}
	return readback;
}

/** Reads the current data on the 32-bit data bus.
 *
 * @return The 32-bit data readback
 */
uint32_t ParallelBus_ReadData(void)
{
	uint32_t readback = 0;
	for (int i = 0; i < 32; i++)
	{
		GPIOPin dataPin = {GPIODATA, i};
		if (GPIO_Read(dataPin))
		{
			readback |= (1UL << i);
		}
	}
	return readback;
}

/** Reads the status of the CS pin, if it's set as an input.
 *
 * @return True if the CS pin is high, false if it's low
 *
 * This would typically only be used for testing. Under normal operation, this
 * pin will be set as an output, so the readback is irrelevant.
 */
bool ParallelBus_ReadCS(void)
{
	return GPIO_Read(flashCSPin);
}

/** Reads the status of the OE pin, if it's set as an input.
 *
 * @return True if the OE pin is high, false if it's low
 *
 * This would typically only be used for testing. Under normal operation, this
 * pin will be set as an output, so the readback is irrelevant.
 */
bool ParallelBus_ReadOE(void)
{
	return GPIO_Read(flashOEPin);
}

/** Reads the status of the WE pin, if it's set as an input.
 *
 * @return True if the WE pin is high, false if it's low
 *
 * This would typically only be used for testing. Under normal operation, this
 * pin will be set as an output, so the readback is irrelevant.
 */
bool ParallelBus_ReadWE(void)
{
	return GPIO_Read(flashWEPin);
}

/** Performs a write cycle on the parallel bus.
 *
 * @param address The address to write to
 * @param data The 32-bit data to write to the bus
 */
void ParallelBus_WriteCycle(uint32_t address, uint32_t data)
{
	// Simple "naive" version of ParallelBus_WriteCycle for PC simulator.

	// We should currently be in a state of "CS is asserted, OE/WE not asserted".
	// As an optimization, operate under that assumption.

	ParallelBus_SetAddress(address);
	ParallelBus_SetDataDir(0xFFFFFFFF);
	ParallelBus_SetData(data);

	// Assert and then deassert WE to actually do the write cycle.
	ParallelBus_SetWE(false);
	ParallelBus_SetWE(true);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.
}

/** Performs a read cycle on the parallel bus.
 *
 * @param address The address to read from
 * @return The returned 32-bit data
 */
uint32_t ParallelBus_ReadCycle(uint32_t address)
{
	uint32_t readback;

	// Simple "naive" version of ParallelBus_ReadCycle for PC simulator.

	// We should currently be in a state of "CS is asserted, OE/WE not asserted".
	// As an optimization, operate under that assumption.

	// Set data as inputs with pullups
	ParallelBus_SetDataDir(0);
	ParallelBus_SetDataPullups(0xFFFFFFFF);

	// Assert OE so we start reading from the chip. Safe to do now that
	// the data pins have been set as inputs.
	ParallelBus_SetOE(false);

	// Set the address to read from
	ParallelBus_SetAddress(address);

	// Read data
	readback = ParallelBus_ReadData();

	// Deassert OE, and we're done.
	ParallelBus_SetOE(true);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.

	// Return the final value
	return readback;
}

/** Reads a bunch of consecutive data from the parallel bus
 *
 * @param startAddress The address to start reading from
 * @param buf Buffer to store the readback
 * @param len The number of 32-bit words to read
 *
 * This function is just a time saver if we know we will be reading a big block
 * of data. It doesn't bother playing with the control lines between each byte.
 */
void ParallelBus_Read(uint32_t startAddress, uint32_t *buf, uint16_t len)
{
	// We should currently be in a state of "CS is asserted, OE/WE not asserted".
	// As an optimization, operate under that assumption.

	// Set data as inputs with pullups
	ParallelBus_SetDataDir(0);
	ParallelBus_SetDataPullups(0xFFFFFFFF);

	// Assert OE, now the chip will start spitting out data.
	ParallelBus_SetOE(false);

	while (len--)
	{
		// Set the address to read from
		ParallelBus_SetAddress(startAddress++);

		// Read data
		*buf++ = ParallelBus_ReadData();
	}

	// Deassert OE once we are done
	ParallelBus_SetOE(true);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.
}
