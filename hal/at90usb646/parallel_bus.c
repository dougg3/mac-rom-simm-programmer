/*
 * parallel_bus.c
 *
 *  Created on: Nov 26, 2011
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
 * -----------------------------------------------------------------------------
 *
 * For some reason, avr-gcc is super inefficient dealing with uint32_t
 * variables. Looking at the individual bytes using a union results in much
 * more optimized code. Every cycle counts for this. So several of these
 * functions may seem weird with the unions, but it's faster than operating
 * directly with the uint32_t variables.
 *
 * There are also a few time-critical places where I had to bypass the GPIO and
 * SPI drivers, so it's not 100% clean. Oh well...
 */

#include "../parallel_bus.h"
#include "../../drivers/mcp23s17.h"
#include "../../util.h"
#include "gpio_hw.h"
#include <avr/io.h>

/// The port object where the OE, WE, and CS pins are connected
/// (This also happens to be where the MCP control pins are connected)
#define FLASH_CONTROL_PORT					PORTB
/// The index of the MCP23S17 chip select pin
#define MCP_CS_PIN							0
/// The index of the CS pin
#define FLASH_CS_PIN						4
/// The index of the OE pin
#define FLASH_OE_PIN						5
/// The index of the WE pin
#define FLASH_WE_PIN						6
/// The index of the MCP23S17 reset pin
#define MCP_RESET_PIN						7
/// The index of the highest address line on the parallel bus
#define PARALLEL_BUS_HIGHEST_ADDRESS_LINE	20

static ALWAYS_INLINE uint8_t SPITransfer(uint8_t byte);
static ALWAYS_INLINE void SPITransferNoRead(uint8_t byte);
static ALWAYS_INLINE void AssertControl(uint8_t pin);
static ALWAYS_INLINE void DeassertControl(uint8_t pin);

/// The MCP23S17 device
static MCP23S17 mcp23s17 = {
	.spi = {
		.csPin = {GPIOB, MCP_CS_PIN}
	}
};

/// The reset pin for the MCP23S17
static const GPIOPin mcpReset = {GPIOB, MCP_RESET_PIN};
/// The /WE pin for the parallel bus
static const GPIOPin flashWEPin = {GPIOB, FLASH_WE_PIN};
/// The /OE pin for the parallel bus
static const GPIOPin flashOEPin = {GPIOB, FLASH_OE_PIN};
/// The /CS pin for the flash chip
static const GPIOPin flashCSPin = {GPIOB, FLASH_CS_PIN};
/// Whether or not data pins are outputs
static bool dataIsOutput;

/** Initializes the 32-bit data/21-bit address parallel bus.
 *
 */
void ParallelBus_Init(void)
{
	static bool mcpInited = false;
	if (!mcpInited)
	{
		// Set up the MCP23S17
		SPI_InitController(SPI_Controller(0));
		mcp23s17.spi.controller = SPI_Controller(0);
		MCP23S17_Init(&mcp23s17, mcpReset);
		// Go ahead and let the MCP23S17 take over the SPI bus forever.
		// There's nothing else attached to it.
		MCP23S17_Begin(&mcp23s17);
		mcpInited = true;
	}

	// Configure all address lines as outputs, outputting address 0
	ParallelBus_SetAddressDir((1UL << (PARALLEL_BUS_HIGHEST_ADDRESS_LINE + 1)) - 1);
	ParallelBus_SetAddress(0);

	// Set all data lines to pulled-up inputs
	ParallelBus_SetDataDir(0);
	ParallelBus_SetDataPullups(0xFFFFFFFF);
	dataIsOutput = false;
	// Note: During normal operation of read/write cycles, the pullups in the
	// MCP23S17 will remember they are enabled, so we can do an optimization
	// when using ParallelBus_ReadCycle/WriteCycle and assume they are already
	// pulled up. This means we'll bypass ParallelBus_SetDataPullups.

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
	// For efficiency, we talk directly to the PORT registers in this function,
	// rather than going through the GPIO class.

	// NOTE: If any of PORTA or PORTC or PORTD pins 0, 1, 4, 5, or 6 are set as
	// inputs, this function might mess with their pull-up resistors.
	// Only use it under normal operation when all the address pins are being
	// used as outputs.
    union {
        uint32_t addr;
        uint8_t addrBytes[4];
    } u;

    u.addr = address;
	PORTA = u.addrBytes[0]; // A0-A7
	PORTC = u.addrBytes[1]; // A8-A15
	// A16-A20 are special because they are split up...(We use PORTD pins 0, 1, 4, 5, 6)
	u.addrBytes[2] = (u.addrBytes[2] & 0x03) | (uint8_t)((u.addrBytes[2] & 0x1C) << 2) | (PORTD & 0x8C);
    PORTD = u.addrBytes[2];
}

/** Sets the output data on the 32-bit data bus
 *
 * @param data The data
 */
void ParallelBus_SetData(uint32_t data)
{
	// For efficiency, we talk directly to the PORT registers in this function,
	// rather than going through the GPIO class.

	// NOTE: If any pins of PORTE or PORTF are set as inputs, this
	// function might mess with their pull-up resistors.
	// Only use it under normal operation when all the data pins are being
	// used as outputs
    union {
        uint32_t data;
        uint16_t dataShorts[2];
        uint8_t dataBytes[4];
    } u;
    u.data = data;

    // Doing the AVR registers first makes it so we don't have to use the stack
    // (at least according to my testing with avr-gcc)
	PORTE = u.dataBytes[1]; // D16-D23
	PORTF = u.dataBytes[0]; // D24-D31

	// D0-D15 are part of the MCP23S17
	MCP23S17_SetOutputs(&mcp23s17, u.dataShorts[1]);
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
	DDRA = (outputs & 0xFF); // A0-A7
	DDRC = ((outputs >> 8) & 0xFF); // A8-A15

	// A16-A20 are special because they are split up...(We use PORTD pins 0, 1, 4, 5, 6)
	uint8_t tmp = (outputs >> 16) & 0xFF;
	tmp = (tmp & 0x03) | ((tmp & 0x1C) << 2);

	// Now, turn off the DDR bits we have to turn off,
	// and turn on the DDR bits we have to turn on
	// (without affecting other bits [2, 3, and 7]
	// that we aren't supposed to touch)
	DDRD &= (0x8C | tmp); // This should turn off all '0' bits in tmp.
	DDRD |= tmp; // This should turn on all '1' bits in tmp
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
    union {
        uint32_t data;
        uint16_t dataShorts[2];
        uint8_t dataBytes[4];
    } u;
    u.data = outputs;

    // Doing the AVR registers first makes it so we don't have to use the stack
    DDRE = u.dataBytes[1]; // D16-D23
	DDRF = u.dataBytes[0]; // D24-D31

	// D0-D15 are part of the MCP23S17
	MCP23S17_SetDDR(&mcp23s17, u.dataShorts[1]);

	// If none of the pins are outputs, ensure dataIsOutput is false
	if (outputs == 0)
	{
		dataIsOutput = false;
	}
	// If all of the pins are outputs, ensure dataIsOutput is true
	else if (outputs == 0xFFFFFFFFUL)
	{
		dataIsOutput = true;
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
	// Pull-ups are set by writing to the data register when in input mode.
	// MAKE SURE THE PINS ARE SET AS INPUTS FIRST!  This is a cheat only
	// possible on the AVR. Some places like SIMMElectricalTest call SetAddress
	// followed by SetAddressPullups, which is kinda weird because it sets the
	// same registers. But the way it's called doesn't hurt anything...
	ParallelBus_SetAddress(pullups);
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
	// NOTE: If any pins of PORTE or PORTF are set as outputs, this
	// function might mess with their output values.
	// Only use it when all the data pins are being used as inputs
    union {
        uint32_t data;
        uint16_t dataShorts[2];
        uint8_t dataBytes[4];
    } u;
    u.data = pullups;

	PORTE = u.dataBytes[1]; // D16-D23
	PORTF = u.dataBytes[0]; // D24-D31

	// D0-D15 are part of the MCP23S17
	MCP23S17_SetPullups(&mcp23s17, u.dataShorts[1]);
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
	uint32_t result = PINA;
	result |= (((uint32_t)PINC) << 8);
	uint8_t tmp = (PIND & 0x03) | ((PIND & 0x70) >> 2);
	result |= (((uint32_t)tmp) << 16);

	return result;
}

/** Reads the current data on the 32-bit data bus.
 *
 * @return The 32-bit data readback
 */
uint32_t ParallelBus_ReadData(void)
{
    union {
        uint32_t data;
        uint16_t dataShorts[2];
        uint8_t dataBytes[4];
    } u;

	u.dataShorts[1] = MCP23S17_ReadInputs(&mcp23s17);

	// Grab the other two bytes...
	u.dataBytes[1] = PINE;
	u.dataBytes[0] = PINF;

	return u.data;
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
 *
 * Because this function is used a lot during programming, it is super
 * optimized and bypasses the GPIO and SPI drivers. It's a necessary evil.
 * It makes a big difference in programming time.
 */
void ParallelBus_WriteCycle(uint32_t address, uint32_t data)
{
	// Using this union surprisingly speeds things up when assembling or
	// interpreting a uint32_t on the AVR.
    union {
        uint32_t word;
        uint8_t bytes[4];
    } u;

    // We should currently be in a state of "CS is asserted, OE/WE not asserted".
    // As an optimization, operate under that assumption.

	// Set address. This is basically the exact same code as ParallelBus_SetAddress,
	// but repeated in here so we don't have any function call overhead.
    u.word = address;
	PORTA = u.bytes[0];
	PORTC = u.bytes[1];
	u.bytes[2] = (u.bytes[2] & 0x03) | (uint8_t)((u.bytes[2] & 0x1C) << 2) | (PORTD & 0x8C);
	PORTD = u.bytes[2];

	// If the data port is not already set as outputs, set it to be outputs now
	if (!dataIsOutput)
	{
		// Set data as outputs. Bypass the SPI/GPIO drivers for this for efficiency.
		DDRE = 0xFF;
		DDRF = 0xFF;
		AssertControl(MCP_CS_PIN);
		SPITransferNoRead(MCP23S17_CONTROL_WRITE(0));
		SPITransferNoRead(MCP23S17_IODIRA);
		SPITransferNoRead(0);
		SPITransferNoRead(0);
		DeassertControl(MCP_CS_PIN);
		dataIsOutput = true;
	}

	// Set data. Bypass the SPI/GPIO drivers again...
    u.word = data;
	PORTE = u.bytes[1];
	PORTF = u.bytes[0];
	AssertControl(MCP_CS_PIN);
	SPITransferNoRead(MCP23S17_CONTROL_WRITE(0));
	SPITransferNoRead(MCP23S17_GPIOA);
	SPITransferNoRead(u.bytes[3]);
	SPITransferNoRead(u.bytes[2]);
	DeassertControl(MCP_CS_PIN);

	// Assert and then deassert WE to actually do the write cycle.
	AssertControl(FLASH_WE_PIN);
	DeassertControl(FLASH_WE_PIN);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.
}

/** Performs a read cycle on the parallel bus.
 *
 * @param address The address to read from
 * @return The returned 32-bit data
 *
 * Because this function is used a lot during programming, it is super
 * optimized and bypasses the GPIO and SPI drivers. It's a necessary evil.
 * It makes a big difference in programming time.
 */
uint32_t ParallelBus_ReadCycle(uint32_t address)
{
	// Using this union surprisingly speeds things up when assembling or
	// interpreting a uint32_t on the AVR.
    union {
        uint32_t word;
        uint8_t bytes[4];
    } u;

    // We should currently be in a state of "CS is asserted, OE/WE not asserted".
    // As an optimization, operate under that assumption.

    // If the data pins are set as outputs, change them to inputs
	if (dataIsOutput)
	{
		// Set data as inputs. Bypass the SPI/GPIO drivers for this for efficiency.
		DDRE = 0;
		DDRF = 0;
		AssertControl(MCP_CS_PIN);
		SPITransferNoRead(MCP23S17_CONTROL_WRITE(0));
		SPITransferNoRead(MCP23S17_IODIRA);
		SPITransferNoRead(0xFF);
		SPITransferNoRead(0xFF);
		DeassertControl(MCP_CS_PIN);

		// Set pull-ups on the AVR data pins so we get a default value if a chip
		// isn't responding. We can assume the MCP23S17 has already been configured
		// to have its inputs pulled up. On the AVR we can't assume because its
		// pull-up state is shared by the same register used for data output.
		PORTE = 0xFF;
		PORTF = 0xFF;

		dataIsOutput = false;
	}

	// Assert OE so we start reading from the chip. Safe to do now that
	// the data pins have been set as inputs.
	AssertControl(FLASH_OE_PIN);

	// Set address. This is basically the exact same code as ParallelBus_SetAddress,
	// but repeated in here so we don't have any function call overhead.
    u.word = address;
	PORTA = u.bytes[0];
	PORTC = u.bytes[1];
	u.bytes[2] = (u.bytes[2] & 0x03) | (uint8_t)((u.bytes[2] & 0x1C) << 2) | (PORTD & 0x8C);
	PORTD = u.bytes[2];

	// Start the SPI read. Each clock cycle at 16 MHz is 62.5 nanoseconds. We don't want to
	// immediately read back the data bus until the address has settled, so do some SPI
	// preparation in the meantime.
	AssertControl(MCP_CS_PIN);
	SPITransferNoRead(MCP23S17_CONTROL_READ(0));
	SPITransferNoRead(MCP23S17_GPIOA);

	// Read data. Bypass the GPIO/SPI drivers again...
	u.bytes[1] = PINE;
	u.bytes[0] = PINF;
	u.bytes[3] = SPITransfer(0);
	u.bytes[2] = SPITransfer(0);
	DeassertControl(MCP_CS_PIN);

	// Deassert OE, and we're done.
	DeassertControl(FLASH_OE_PIN);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.

	// Return the final value
	return u.word;
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

	// Using this union surprisingly speeds things up when assembling or
	// interpreting a uint32_t on the AVR.
    union {
        uint32_t word;
        uint8_t bytes[4];
    } u;

    // If the data pins are set as outputs, change them to inputs
	if (dataIsOutput)
	{
		// Set data as inputs. Bypass the SPI/GPIO drivers for this for efficiency.
		DDRE = 0;
		DDRF = 0;
		AssertControl(MCP_CS_PIN);
		SPITransferNoRead(MCP23S17_CONTROL_WRITE(0));
		SPITransferNoRead(MCP23S17_IODIRA);
		SPITransferNoRead(0xFF);
		SPITransferNoRead(0xFF);
		DeassertControl(MCP_CS_PIN);

		// Set pull-ups on the AVR data pins so we get a default value if a chip
		// isn't responding. We can assume the MCP23S17 has already been configured
		// to have its inputs pulled up. On the AVR we can't assume because its
		// pull-up state is shared by the same register used for data output.
		PORTE = 0xFF;
		PORTF = 0xFF;

		dataIsOutput = false;
	}

	// Assert OE, now the chip will start spitting out data.
	AssertControl(FLASH_OE_PIN);

	while (len--)
	{
		// Set address. This is basically the exact same code as ParallelBus_SetAddress,
		// but repeated in here so we don't have any function call overhead.
	    u.word = startAddress++;
		PORTA = u.bytes[0];
		PORTC = u.bytes[1];
		u.bytes[2] = (u.bytes[2] & 0x03) | (uint8_t)((u.bytes[2] & 0x1C) << 2) | (PORTD & 0x8C);
		PORTD = u.bytes[2];

		// Start the SPI read. Each clock cycle at 16 MHz is 62.5 nanoseconds. We don't want to
		// immediately read back the data bus until the address has settled, so do some SPI
		// preparation in the meantime.
		AssertControl(MCP_CS_PIN);
		SPITransferNoRead(MCP23S17_CONTROL_READ(0));
		SPITransferNoRead(MCP23S17_GPIOA);

		// Read data. Bypass the GPIO/SPI drivers again...
		u.bytes[1] = PINE;
		u.bytes[0] = PINF;
		u.bytes[3] = SPITransfer(0);
		u.bytes[2] = SPITransfer(0);
		DeassertControl(MCP_CS_PIN);
		*buf++ = u.word;
	}

	// Deassert OE once we are done
	DeassertControl(FLASH_OE_PIN);

	// Control lines are left as "CS asserted, OE/WE not asserted" here.
}

/** Writes/reads a byte to/from the MCP23S17. More optimal than using the driver.
 *
 * @param byte The byte to write
 * @return The byte read back
 */
static ALWAYS_INLINE uint8_t SPITransfer(uint8_t byte)
{
	SPDR = byte;
	// Crazy optimization. Instead of waiting for the status register
	// (see the commented-out "while" statement below), wait for 17 clock
	// cycles instead. We know that our SPI bit rate is half the CPU clock.
	// After 17 clock cycles, the entire byte has been written out.
	__asm__ __volatile__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
	__asm__ __volatile__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
	__asm__ __volatile__ ("nop\n");
	//while (!(SPSR & (1 << SPIF)));
	return SPDR;
}

/** Writes a byte to the MCP23S17 without reading back the result. More optimal
 *  than using the driver.
 *
 * @param byte The byte to write
 */
static ALWAYS_INLINE void SPITransferNoRead(uint8_t byte)
{
	SPDR = byte;
	// Crazy optimization. Instead of waiting for the status register
	// (see the commented-out "while" statement below), wait for 17 clock
	// cycles instead. We know that our SPI bit rate is half the CPU clock.
	// After 17 clock cycles, the entire byte has been written out.
	__asm__ __volatile__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
	__asm__ __volatile__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
	__asm__ __volatile__ ("nop\n");
	//while (!(SPSR & (1 << SPIF)));
}

/** Asserts a control pin
 *
 * @param pin Pin number of the control pin to assert
 *
 * This is slightly faster than using the GPIO driver because it inlines directly
 * to a port RMW operation. Just a small optimization for performance.
 */
static ALWAYS_INLINE void AssertControl(uint8_t pin)
{
	FLASH_CONTROL_PORT &= ~(1 << pin);
}

/** Deasserts a control pin
 *
 * @param pin Pin number of the control pin to deassert
 *
 * This is slightly faster than using the GPIO driver because it inlines directly
 * to a port RMW operation. Just a small optimization for performance.
 */
static ALWAYS_INLINE void DeassertControl(uint8_t pin)
{
	FLASH_CONTROL_PORT |= (1 << pin);
}
