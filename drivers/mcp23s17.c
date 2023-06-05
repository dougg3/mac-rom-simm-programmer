/*
 * mcp23s17.c
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 *
 * Copyright (C) 2011-2023 Doug Brown
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

#include "mcp23s17.h"
#include "hardware.h"

/// Maximum SPI clock rate = 10 MHz
#define MCP23S17_MAX_CLOCK		10000000UL

static void MCP23S17_WriteBothRegs(MCP23S17 *mcp, uint8_t addrA, uint16_t value);
static uint16_t MCP23S17_ReadBothRegs(MCP23S17 *mcp, uint8_t addrA);

/** Initializes an MCP23S17 object
 *
 * @param mcp The MCP23S17 object to initialize
 * @param resetPin The GPIO pin hooked to the reset input
 */
void MCP23S17_Init(MCP23S17 *mcp, GPIOPin resetPin)
{
	SPI_InitDevice(&mcp->spi, MCP23S17_MAX_CLOCK, SPI_MODE_0);

	// Do a reset pulse if we need to. No need to save the reset pin
	// after that.
	if (!GPIO_IsNull(resetPin))
	{
		GPIO_SetDirection(resetPin, true);
		GPIO_SetOff(resetPin);
		DelayUS(1);
		GPIO_SetOn(resetPin);
		DelayUS(1);
	}
}

/** Begins a set of transactions with the MCP23S17.
 *
 * @param mcp The MCP23S17 to talk with
 *
 * You have to call this before using any of the MCP23S17 functions other than
 * init. This takes control of the SPI bus. If the MCP23S17 is the only device
 * on the bus, you can just call this once in the program. Otherwise, you should
 * release it as soon as you're done so other SPI devices can use the bus instead.
 */
void MCP23S17_Begin(MCP23S17 *mcp)
{
	SPI_RequestBus(&mcp->spi);
}

/** Ends a set of transactions with the MCP23S17.
 *
 * @param mcp The MCP23S17 to release
 *
 * You should call this when you're done talking to the MCP23S17 to free up the
 * SPI bus for other devices.
 */
void MCP23S17_End(MCP23S17 *mcp)
{
	SPI_ReleaseBus(&mcp->spi);
}

/** Sets the data direction register in the MCP23S17
 *
 * @param mcp The MCP23S17
 * @param ddr Bitmask representing direction of the 16 GPIO pins (1 = output, 0 = input)
 */
void MCP23S17_SetDDR(MCP23S17 *mcp, uint16_t ddr)
{
	// The MCP23S17's DDR is backwards from most other chips. I like dealing
	// with it so it behaves like other MCUs, so I invert any DDR values in this
	// driver. In other words, when you set or get the DDR through this driver,
	// the 1s and 0s are backwards from what the MCP23S17's datasheet says, but
	// they are consistent with most MCUs. I value the consistency more.
	MCP23S17_WriteBothRegs(mcp, MCP23S17_IODIRA, ~ddr);
}

/** Reads the data direction register in the MCP23S17
 *
 * @param mcp The MCP23S17
 * @return Bitmask representing direction of the 16 GPIO pins (1 = output, 0 = input)
 */
uint16_t MCP23S17_DDR(MCP23S17 *mcp)
{
	// As I mentioned above, DDR bits are inverted from what the MCP23S17's
	// datasheet says, but consistent with most MCUs
	return ~MCP23S17_ReadBothRegs(mcp, MCP23S17_IODIRA);
}

/** Sets the output values in the MCP23S17
 *
 * @param mcp The MCP23S17
 * @param data Bitmask representing output state of the 16 GPIO pins (1 = high, 0 = low)
 */
void MCP23S17_SetOutputs(MCP23S17 *mcp, uint16_t data)
{
	MCP23S17_WriteBothRegs(mcp, MCP23S17_GPIOA, data);
}

/** Gets the current output values in the MCP23S17
 *
 * @param mcp The MCP23S17
 * @return Bitmask representing output state of the 16 GPIO pins (1 = high, 0 = low)
 */
uint16_t MCP23S17_Outputs(MCP23S17 *mcp)
{
	return MCP23S17_ReadBothRegs(mcp, MCP23S17_OLATA);
}

/** Reads the current input values in the MCP23S17
 *
 * @param mcp The MCP23S17
 * @return Bitmask representing input state of the 16 GPIO pins (1 = high, 0 = low)
 */
uint16_t MCP23S17_ReadInputs(MCP23S17 *mcp)
{
	return MCP23S17_ReadBothRegs(mcp, MCP23S17_GPIOA);
}

/** Enables/disables pullups in the MCP23S17
 *
 * @param mcp The MCP23S17
 * @param pullups Bitmask representing which input pins should have pullups enabled.
 */
void MCP23S17_SetPullups(MCP23S17 *mcp, uint16_t pullups)
{
	MCP23S17_WriteBothRegs(mcp, MCP23S17_GPPUA, pullups);
}

/** Reads the current pullup state in the MCP23S17
 *
 * @param mcp The MCP23S17
 * return Bitmask representing which input pins have pullups enabled.
 */
uint16_t MCP23S17_Pullups(MCP23S17 *mcp)
{
	return MCP23S17_ReadBothRegs(mcp, MCP23S17_GPPUA);
}

/** Helper function that writes two consecutive registers in the MCP23S17
 *
 * @param mcp The MCP23S17
 * @param addrA The address of the first ("A") register
 * @param value The value to write to the A and B registers. High byte = A, low byte = B
 */
static void MCP23S17_WriteBothRegs(MCP23S17 *mcp, uint8_t addrA, uint16_t value)
{
	// addrA should contain the address of the "A" register.
	// the chip should also be in "same bank" mode.
	SPI_Assert(&mcp->spi);

	// Start off the communication by telling the MCP23S17 that we are writing to a register
	SPI_RWByte(&mcp->spi, MCP23S17_CONTROL_WRITE(0));

	// Tell it the first register we're writing to (the "A" register)
	SPI_RWByte(&mcp->spi, addrA);

	// Write the first byte of the register
	SPI_RWByte(&mcp->spi, (uint8_t)((value >> 8) & 0xFF));

	// It should auto-increment to the "B" register, now write that
	SPI_RWByte(&mcp->spi, (uint8_t)(value & 0xFF));

	SPI_Deassert(&mcp->spi);
}

/** Helper function that reads two consecutive registers in the MCP23S17
 *
 * @param mcp The MCP23S17
 * @param addrA The address of the first ("A") register
 * @return The value read back from the A and B registers. High byte = A, low byte = B
 */
static uint16_t MCP23S17_ReadBothRegs(MCP23S17 *mcp, uint8_t addrA)
{
	uint16_t returnVal;

	SPI_Assert(&mcp->spi);

	// Start off the communication by telling the MCP23S17 that we are reading from a register
	SPI_RWByte(&mcp->spi, MCP23S17_CONTROL_READ(0));

	// Tell it which register we're reading from (the "A" register)
	SPI_RWByte(&mcp->spi, addrA);

	// Read the first byte of the register
	returnVal = (((uint16_t)SPI_RWByte(&mcp->spi, 0)) << 8);

	// It should auto-increment to the "B" register, now read that
	returnVal |= SPI_RWByte(&mcp->spi, 0);

	SPI_Deassert(&mcp->spi);

	return returnVal;
}
