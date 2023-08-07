/*
 * spi.c
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

#include "../spi.h"
#include "gpio_hw.h"
#include <stddef.h>

/** Gets the SPI hardware controller at the specified index
 *
 * @param index The index of the controller. No SPI is available on the M258KE.
 * @return The SPI controller, or NULL if an invalid index is supplied
 */
SPIController *SPI_Controller(uint8_t index)
{
	(void)index;
	return NULL;
}

/** Initializes the supplied SPI controller
 *
 * @param c The controller
 */
void SPI_InitController(SPIController *c)
{
	(void)c;
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
	(void)spi;
	(void)maxClock;
	(void)mode;
	return false;
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
	(void)spi;
}

/** Deasserts an SPI device's chip select pin
 *
 * @param spi The SPI device
 */
void SPI_Deassert(SPIDevice *spi)
{
	(void)spi;
}

/** Transfers a single byte to/from an SPI device
 *
 * @param spi The SPI device
 * @param b The byte to send
 * @return The byte that was simultaneously received
 */
uint8_t SPI_RWByte(SPIDevice *spi, uint8_t b)
{
	(void)spi;
	(void)b;
	return 0;
}
