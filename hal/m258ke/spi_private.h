/*
 * spi_private.h
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

#ifndef HAL_M258KE_SPI_PRIVATE_H_
#define HAL_M258KE_SPI_PRIVATE_H_

/// Private data for an SPI device on the M258KE
typedef struct SPIDevicePrivate
{
	/// There's nothing needed. We don't need SPI on this device.
} SPIDevicePrivate;

#endif /* HAL_M258KE_SPI_PRIVATE_H_ */
