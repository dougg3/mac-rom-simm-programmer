/*
 * spi.c
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

#include "../spi.h"
#include "gpio_hw.h"
#include <stddef.h>

/// The lone SPI controller available on this simulated PC build
static SPIController controller =
{
	.sckPin = {GPIOMISC, 1},
	.mosiPin = {GPIOMISC, 2},
	.misoPin = {GPIOMISC, 3}
};

/** Gets the SPI hardware controller at the specified index
 *
 * @param index The index of the controller. Only 0 is valid on this PC build.
 * @return The SPI controller, or NULL if an invalid index is supplied
 */
SPIController *SPI_Controller(uint8_t index)
{
	// The simulated PC build only has one SPI controller
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

	// TODO: Save the mode and clock?
	(void)maxClock; (void)mode;

	return true;
}

/** Allows an SPI device to request control of the bus.
 *
 * @param spi The SPI device
 */
void SPI_RequestBus(SPIDevice *spi)
{
	(void)spi;
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
	// Since there's only one controller on this simulated PC, we don't actually care
	// about the SPI device pointer here
	(void)spi;

	// TODO: Do something with the byte, return the readback
	(void)b;
	return 0;
}
