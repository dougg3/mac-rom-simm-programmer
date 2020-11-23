/*
 * spi.c
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

#include "../spi.h"
#include "gpio_hw.h"
#include <stddef.h>
#include <avr/io.h>

/// Keep a struct of available dividers, calculate something at runtime.
typedef struct SPIDividerInfo
{
	/// The divider
	uint8_t divider;
	/// Bit 0 = SPR0, Bit 1 = SPR1, Bit 2 = SPI2X (in SPSR)
	uint8_t configBits;
} SPIDividerInfo;

/// List of possible SPI dividers. Must be in ascending order.
static const SPIDividerInfo dividers[] = {
	{2,   0b100},
	{4,   0b000},
	{8,   0b101},
	{16,  0b001},
	{32,  0b110},
	{64,  0b010},
	{128, 0b011}
};

/// The lone SPI controller available on this AVR
static SPIController controller =
{
	.sckPin = {GPIOB, 1},
	.mosiPin = {GPIOB, 2},
	.misoPin = {GPIOB, 3}
};

/** Gets the SPI hardware controller at the specified index
 *
 * @param index The index of the controller. Only 0 is valid on this AVR.
 * @return The SPI controller, or NULL if an invalid index is supplied
 */
SPIController *SPI_Controller(uint8_t index)
{
	// The AVR only has one SPI controller
	return (index == 0) ? &controller : NULL;
}

/** Initializes the supplied SPI controller
 *
 * @param c The controller
 */
void SPI_InitController(SPIController *c)
{
	GPIO_SetDirection(c->sckPin, true);
	GPIO_SetDirection(c->mosiPin, true);
	GPIO_SetDirection(c->misoPin, false);
	// Don't do anything with the rest of the registers.
	// SPI_RequestController will handle it.
}

/** Initializes the supplied SPI device
 *
 * @param spi The device
 * @param maxClock The maximum clock rate supported by the device in Hz
 * @param mode The SPI mode (see the SPI_MODE_*, SPI_CPHA, and SPI_CPOL defines)
 * @return True on success, false on failure
 */
bool SPI_InitDevice(SPIDevice *spi, uint32_t maxClock, uint8_t mode)
{
	GPIO_SetDirection(spi->csPin, true);
	SPI_Deassert(spi);

	// Calculate which SPI clock divider to use
	int8_t dividerIndex = -1;
	for (uint8_t i = 0; dividerIndex < 0 && i < sizeof(dividers)/sizeof(dividers[0]); i++)
	{
		if (F_CPU / (uint32_t)dividers[i].divider <= maxClock)
		{
			dividerIndex = (int8_t)i;
		}
	}

	// Fill in the SPI config registers according to the requested clock
	if (dividerIndex >= 0)
	{
		uint8_t dividerBits = dividers[dividerIndex].configBits;

		spi->private.spcr =
			(0 << SPIE) | // No SPI interrupts
			(1 << SPE) |  // Enable SPI
			(0 << DORD) | // MSB first
			(1 << MSTR) | // Master mode
			(((mode & SPI_CPOL) ? 1 : 0) << CPOL) |
			(((mode & SPI_CPHA) ? 1 : 0) << CPHA) |
			((dividerBits & 3) << SPR0);

		// The only writable bit in SPSR is the SPI2X bit.
		spi->private.spsr =
			((dividerBits >> 2) << SPI2X);

		return true;
	}
	else
	{
		// This SPI device requires a clock slower than what we can divide down to
		return true;
	}
}

/** Allows an SPI device to request control of the bus.
 *
 * @param spi The SPI device
 */
void SPI_RequestBus(SPIDevice *spi)
{
	(void)spi;

	// Set up the controller with the correct speed/mode config for this device
	SPCR = spi->private.spcr;
	SPSR = spi->private.spsr;
}

/** Allows an SPI device to relinquish control of the bus.
 *
 * @param spi The SPI device
 */
void SPI_ReleaseBus(SPIDevice *spi)
{
	(void)spi;
}

/** Asserts an SPI device's chip select pin
 *
 * @param spi The SPI device
 */
void SPI_Assert(SPIDevice *spi)
{
	GPIO_SetOff(spi->csPin);
}

/** Deasserts an SPI device's chip select pin
 *
 * @param spi The SPI device
 */
void SPI_Deassert(SPIDevice *spi)
{
	GPIO_SetOn(spi->csPin);
}

/** Transfers a single byte to/from an SPI device
 *
 * @param spi The SPI device
 * @param b The byte to send
 * @return The byte that was simultaneously received
 */
uint8_t SPI_RWByte(SPIDevice *spi, uint8_t b)
{
	// Since there's only one controller on this AVR, we don't actually care
	// about the SPI device pointer here
	(void)spi;

	// Write the byte, wait for the write to complete, read back the result
	SPDR = b;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}
