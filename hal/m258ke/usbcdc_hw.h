/*
 * usbcdc_hw.h
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
 * Portions of this code also came from Nuvoton's BSP, originally
 * licensed as Apache-2.0:
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 *
 */

#ifndef HAL_M258KE_USBCDC_HW_H_
#define HAL_M258KE_USBCDC_HW_H_

#include <stdint.h>
#include <stdbool.h>
#include "nuvoton/NuMicro.h"
#include "nuvoton/usbd.h"

/// Maximum packet sizes of each endpoint
#define EP0_MAX_PKT_SIZE    64
#define EP1_MAX_PKT_SIZE    64
#define EP2_MAX_PKT_SIZE    8
#define EP3_MAX_PKT_SIZE    64
#define EP4_MAX_PKT_SIZE    64

/// Assigned endpoint numbers for CDC serial port
#define INT_IN_EP_NUM       2
#define BULK_IN_EP_NUM      3
#define BULK_OUT_EP_NUM     4

/** Initializes the USB CDC serial port
 *
 */
void USBCDC_Init(void);

/** Disconnects the USB CDC device from the host
 *
 */
static inline void USBCDC_Disable(void)
{
    USBD_SET_SE0();
}

/** Performs any necessary periodic tasks for the USB CDC serial port
 *
 */
void USBCDC_Check(void);

/** Sends a byte out the USB serial port
 *
 * @param b The byte
 */
void USBCDC_SendByte(uint8_t b);

/** Sends a block of data over the USB CDC serial port
 *
 * @param data The data to send
 * @param len The number of bytes
 * @return True on success, false on failure
 */
static inline bool USBCDC_SendData(uint8_t const *data, uint16_t len)
{
	while (len--)
	{
		USBCDC_SendByte(*data++);
	}

	return true;
}

/** Reads a byte from the USB serial port, if available
 *
 * @return The byte, or -1 if there is nothing available
 */
int16_t USBCDC_ReadByte(void);

/** Reads a byte from the USB CDC serial port. Blocks until one is available.
 *
 * @return The byte read
 */
static inline uint8_t USBCDC_ReadByteBlocking(void)
{
	int16_t b;
	do
	{
		b = USBCDC_ReadByte();
	} while (b < 0);
	return (uint8_t)b;
}

/** Flushes remaining data out to the USB serial port
 *
 */
void USBCDC_Flush(void);

#endif /* HAL_M258KE_USBCDC_HW_H_ */
