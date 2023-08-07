/*
 * parallel_bus.c
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

#include "../parallel_bus.h"
#include "../../util.h"
#include "../gpio.h"
#include "gpio_hw.h"
#include "nuvoton/NuMicro.h"

// Borrowed from Nuvoton's sample code
#define GPIO_PIN_DATA(port, pin)	(*((volatile uint32_t *)((GPIO_PIN_DATA_BASE+(0x40*(port))) + ((pin)<<2))))
#define PC10						GPIO_PIN_DATA(2, 10)
#define PC11						GPIO_PIN_DATA(2, 11)
#define PC12						GPIO_PIN_DATA(2, 12)

/// Defines for speedier toggle of these pins
#define FLASH_WE_PIN				PC12
#define FLASH_OE_PIN				PC11
#define FLASH_CS_PIN				PC10

/// The index of the highest address line on the parallel bus
#define PARALLEL_BUS_HIGHEST_ADDRESS_LINE	20

// Data pins:
// PB0-7: D8-D15
// PB8-15: D0-D7
// PE0-7: D24-D31
// PE8-15: D16-D23

// Address pins:
// PA0-11: A0-A11
// PC0-8: A12-A20

static inline void ParallelBus_SetDataAllInput(void);
static inline void ParallelBus_SetDataAllOutput(void);

/// The /WE pin for the parallel bus
static const GPIOPin flashWEPin = {GPIOC, 12};
/// The /OE pin for the parallel bus
static const GPIOPin flashOEPin = {GPIOC, 11};
/// The /CS pin for the flash chip
static const GPIOPin flashCSPin = {GPIOC, 10};

// Macros for asserting and deasserting pins
#define AssertControl(p) p = 0
#define DeassertControl(p) p = 1

/** Initializes the 32-bit data/21-bit address parallel bus.
 *
 */
void ParallelBus_Init(void)
{
	// Configure all address lines as outputs, outputting address 0
	ParallelBus_SetAddressDir((1UL << (PARALLEL_BUS_HIGHEST_ADDRESS_LINE + 1)) - 1);
	ParallelBus_SetAddress(0);

	// Set all data lines to pulled-up inputs
	ParallelBus_SetDataAllInput();
	ParallelBus_SetDataPullups(0xFFFFFFFFUL);

	// Control lines
	ParallelBus_SetCSDir(true);
	ParallelBus_SetOEDir(true);
	ParallelBus_SetWEDir(true);

	// Default to only CS asserted
	DeassertControl(FLASH_WE_PIN);
	DeassertControl(FLASH_OE_PIN);
	AssertControl(FLASH_CS_PIN);
}

/** Sets the address being output on the 21-bit address bus
 *
 * @param address The address
 */
void ParallelBus_SetAddress(uint32_t address)
{
	const uint32_t addrMaskA = 0xFFFUL;
	const uint32_t addrMaskC = 0x1FFUL;

	const uint32_t addrA = address & addrMaskA;
	const uint32_t addrC = (address >> 12) & addrMaskC;

	// For efficiency, we talk directly to the registers in this function,
	// rather than going through the GPIO class.

	uint32_t tmpA = PA->DOUT;
	uint32_t tmpC = PC->DOUT;
	tmpA &= ~addrMaskA;
	tmpA |= addrA;
	tmpC &= ~addrMaskC;
	tmpC |= addrC;
	PA->DOUT = tmpA;
	PC->DOUT = tmpC;
}

/** Sets the output data on the 32-bit data bus
 *
 * @param data The data
 */
void ParallelBus_SetData(uint32_t data)
{
	const uint32_t dataMaskB = 0xFFFFUL;
	const uint32_t dataMaskE = 0xFFFFUL;

	const uint32_t dataB = (data >> 16) & dataMaskB;
	const uint32_t dataE = (data >> 0) & dataMaskE;

	// For efficiency, we talk directly to the registers in this function,
	// rather than going through the GPIO class.

	PB->DOUT = dataB;
	PE->DOUT = dataE;
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

/**
 * @brief Interleaves zeros between each bit in a 16-bit value
 * @param value The original value to interleave, 16 bits
 * @return Interleaved version of value, 32 bits
 *
 * For example, if value = 0b0000111100110101, then it will return:
 * 0b00000000010101010000010100010001
 */
static uint32_t InterleaveZeros(uint16_t value)
{
	uint32_t x = value;

	// https://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
	static const uint32_t B[] = {0x55555555UL, 0x33333333UL, 0x0F0F0F0FUL, 0x00FF00FFUL};
	static const uint32_t S[] = {1UL, 2UL, 4UL, 8UL};

	x = (x | (x << S[3])) & B[3];
	x = (x | (x << S[2])) & B[2];
	x = (x | (x << S[1])) & B[1];
	x = (x | (x << S[0])) & B[0];

	return x;
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
	const uint32_t addrMaskA = 0xFFFUL;
	const uint32_t addrMaskC = 0x1FFUL;

	const uint32_t addrA = outputs & addrMaskA;
	const uint32_t addrC = (outputs >> 12) & addrMaskC;

	// For efficiency, we talk directly to the PORT registers in this function,
	// rather than going through the GPIO class.

	// There are actually 2 bits per pin. We want to write 00 if input,
	// 01 if output. So we need to interleave a zero in between each bit
	// to match the register layout.

	const uint32_t regMaskA = 0xFFFFFFUL;
	const uint32_t regMaskC = 0x3FFFFUL;

	const uint32_t regA = InterleaveZeros(addrA);
	const uint32_t regC = InterleaveZeros(addrC);

	uint32_t tmpA = PA->MODE;
	uint32_t tmpC = PC->MODE;
	tmpA &= ~regMaskA;
	tmpA |= regA;
	tmpC &= ~regMaskC;
	tmpC |= regC;
	PA->MODE = tmpA;
	PC->MODE = tmpC;
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
	const uint32_t dataMaskB = 0xFFFFUL;
	const uint32_t dataMaskE = 0xFFFFUL;

	const uint32_t dataB = (outputs >> 16) & dataMaskB;
	const uint32_t dataE = (outputs >> 0) & dataMaskE;

	// For efficiency, we talk directly to the registers in this function,
	// rather than going through the GPIO class.

	// There are actually 2 bits per pin. We want to write 00 if input,
	// 01 if output. So we need to interleave a zero in between each bit
	// to match the register layout.

	const uint32_t regB = InterleaveZeros(dataB);
	const uint32_t regE = InterleaveZeros(dataE);

	PB->MODE = regB;
	PE->MODE = regE;
}

/**
 * @brief Sets all data pins as inputs
 *
 * This is an optimized version for the read cycle functions
 */
static inline void ParallelBus_SetDataAllInput(void)
{
	PB->MODE = 0;
	PE->MODE = 0;
}

/**
 * @brief Sets all data pins as outputs
 *
 * This is an optimized version for the write cycle functions
 */
static inline void ParallelBus_SetDataAllOutput(void)
{
	PB->MODE = 0x55555555UL;
	PE->MODE = 0x55555555UL;
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
	const uint32_t addrMaskA = 0xFFFUL;
	const uint32_t addrMaskC = 0x1FFUL;

	const uint32_t addrA = pullups & addrMaskA;
	const uint32_t addrC = (pullups >> 12) & addrMaskC;

	// For efficiency, we talk directly to the registers in this function,
	// rather than going through the GPIO class.

	// There are actually 2 bits per pin. We want to write 00 if disabled,
	// 01 if enabled. So we need to interleave a zero in between each bit
	// to match the register layout.

	const uint32_t regMaskA = 0xFFFFFFUL;
	const uint32_t regMaskC = 0x3FFFFUL;

	const uint32_t regA = InterleaveZeros(addrA);
	const uint32_t regC = InterleaveZeros(addrC);

	uint32_t tmpA = PA->PUSEL;
	uint32_t tmpC = PC->PUSEL;
	tmpA &= ~regMaskA;
	tmpA |= regA;
	tmpC &= ~regMaskC;
	tmpC |= regC;
	PA->PUSEL = tmpA;
	PC->PUSEL = tmpC;
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
	const uint32_t dataMaskB = 0xFFFFUL;
	const uint32_t dataMaskE = 0xFFFFUL;

	const uint32_t dataB = (pullups >> 16) & dataMaskB;
	const uint32_t dataE = (pullups >> 0) & dataMaskE;

	// For efficiency, we talk directly to the registers in this function,
	// rather than going through the GPIO class.

	// There are actually 2 bits per pin. We want to write 00 if input,
	// 01 if output. So we need to interleave a zero in between each bit
	// to match the register layout.

	const uint32_t regB = InterleaveZeros(dataB);
	const uint32_t regE = InterleaveZeros(dataE);

	PB->PUSEL = regB;
	PE->PUSEL = regE;
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
	const uint32_t addrMaskA = 0xFFFUL;
	const uint32_t addrMaskC = 0x1FFUL;

	// For efficiency, we talk directly to the registers in this function,
	// rather than going through the GPIO class.

	const uint32_t readA = PA->PIN;
	const uint32_t readC = PC->PIN;

	uint32_t result = readA & addrMaskA;
	result |= ((readC & addrMaskC) << 12);
	return result;
}

/** Reads the current data on the 32-bit data bus.
 *
 * @return The 32-bit data readback
 */
uint32_t ParallelBus_ReadData(void)
{
	const uint32_t dataMaskB = 0xFFFFUL;
	const uint32_t dataMaskE = 0xFFFFUL;

	const uint32_t readB = PB->PIN;
	const uint32_t readE = PE->PIN;

	uint32_t result = readE & dataMaskE;
	result |= ((readB & dataMaskB) << 16);
	return result;
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
	// We should currently be in a state of "CS is asserted, OE/WE not asserted".
	// As an optimization, operate under that assumption.

	// Set address
	ParallelBus_SetAddress(address);

	// Ensure the data pins are all outputs
	ParallelBus_SetDataAllOutput();

	// Set data
	ParallelBus_SetData(data);

	// Assert and then deassert WE to actually do the write cycle.
	AssertControl(FLASH_WE_PIN);
	DeassertControl(FLASH_WE_PIN);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.
}

/** Performs a read cycle on the parallel bus.
 *
 * @param address The address to read from
 * @return The returned 32-bit data
 */
uint32_t ParallelBus_ReadCycle(uint32_t address)
{
	uint32_t ret;

	// We should currently be in a state of "CS is asserted, OE/WE not asserted".
	// As an optimization, operate under that assumption.

	// Ensure the data pins are all inputs
	ParallelBus_SetDataAllInput();

	// Assert OE so we start reading from the chip. Safe to do now that
	// the data pins have been set as inputs.
	AssertControl(FLASH_OE_PIN);

	// Set address
	ParallelBus_SetAddress(address);

	// Read data
	ret = ParallelBus_ReadData();

	// Deassert OE, and we're done.
	DeassertControl(FLASH_OE_PIN);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.

	// Return the final value
	return ret;
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

	// Ensure the data pins are all inputs
	ParallelBus_SetDataAllInput();

	// Assert OE, now the chip will start spitting out data.
	AssertControl(FLASH_OE_PIN);

	while (len--)
	{
		// Set address
		ParallelBus_SetAddress(startAddress++);

		// Read data
		*buf++ = ParallelBus_ReadData();
	}

	// Deassert OE once we are done
	DeassertControl(FLASH_OE_PIN);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.
}
