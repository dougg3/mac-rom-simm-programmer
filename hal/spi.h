/*
 * spi.h
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

#ifndef HAL_SPI_H_
#define HAL_SPI_H_

#include "gpio.h"
#include "spi_private.h"

/// CPHA bit in the SPI mode parameter of SPI_InitDevice
#define SPI_CPHA				(1 << 0)
/// CPOL bit in the SPI mode parameter of SPI_InitDevice
#define SPI_CPOL				(1 << 1)

/// Friendly names for modes 0-3 when dealing with CPHA/CPOL in SPI_InitDevice
#define SPI_MODE_0				(0 | 0)
#define SPI_MODE_1				(0 | SPI_CPHA)
#define SPI_MODE_2				(SPI_CPOL | 0)
#define SPI_MODE_3				(SPI_CPOL | SPI_CPHA)

/// SPI controller
typedef struct SPIController
{
	/// The serial clock pin
	GPIOPin sckPin;
	/// The master out/slave in pin
	GPIOPin mosiPin;
	/// The master in/slave out pin
	GPIOPin misoPin;
} SPIController;

/// SPI device
typedef struct SPIDevice
{
	// These two members should be filled in the struct by hand
	/// The GPIO pin used for chip select
	GPIOPin csPin;
	/// The SPI controller this device belongs to
	SPIController *controller;

	// Everything below here is private
	SPIDevicePrivate private;
} SPIDevice;

SPIController *SPI_Controller(uint8_t index);
void SPI_InitController(SPIController *c);
bool SPI_InitDevice(SPIDevice *spi, uint32_t maxClock, uint8_t mode);
void SPI_RequestBus(SPIDevice *spi);
void SPI_ReleaseBus(SPIDevice *spi);
void SPI_Assert(SPIDevice *spi);
void SPI_Deassert(SPIDevice *spi);
uint8_t SPI_RWByte(SPIDevice *spi, uint8_t b);

#endif /* HAL_SPI_H_ */
