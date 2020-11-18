/*
 * spi.h
 *
 *  Created on: Nov 14, 2020
 *      Author: Doug
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
