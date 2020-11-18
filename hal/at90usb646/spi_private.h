/*
 * spi_private.h
 *
 *  Created on: Nov 14, 2020
 *      Author: Doug
 */

#ifndef HAL_AT90USB646_SPI_PRIVATE_H_
#define HAL_AT90USB646_SPI_PRIVATE_H_

/// Private data for an SPI device on the AT90USB646
typedef struct SPIDevicePrivate
{
	/// Value to write into the SPCR register (contains timing/mode info)
	uint8_t spcr;
	/// Value to write into the SPSR register (contains timing info)
	uint8_t spsr;
} SPIDevicePrivate;

#endif /* HAL_AT90USB646_SPI_PRIVATE_H_ */
