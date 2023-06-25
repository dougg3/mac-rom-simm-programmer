/*
 * usbcdc_hw.h
 *
 *  Created on: Nov 22, 2020
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

#ifndef HAL_AT90USB646_USBCDC_HW_H_
#define HAL_AT90USB646_USBCDC_HW_H_

#include "LUFA/Drivers/USB/USB.h"
#include "cdc_device_definition.h"
#include "../../util.h"

/** Sends a byte over the USB CDC serial port
 *
 * @param byte The byte to send
 */
static ALWAYS_INLINE void USBCDC_SendByte(uint8_t byte)
{
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, byte);
}

/** Sends a block of data over the USB CDC serial port
 *
 * @param data The data to send
 * @param len The number of bytes
 * @return True on success, false on failure
 */
static ALWAYS_INLINE bool USBCDC_SendData(uint8_t const *data, uint16_t len)
{
	return CDC_Device_SendData(&VirtualSerial_CDC_Interface, (char const *)data, len) == ENDPOINT_RWSTREAM_NoError;
}

/** Attempts to read a byte from the USB CDC serial port
 *
 * @return The byte read, or -1 if there are no bytes available
 */
static ALWAYS_INLINE int16_t USBCDC_ReadByte(void)
{
	return CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
}

/** Reads a byte from the USB CDC serial port. Blocks until one is available.
 *
 * @return The byte read
 */
static ALWAYS_INLINE uint8_t USBCDC_ReadByteBlocking(void)
{
	int16_t b;
	do
	{
		b = USBCDC_ReadByte();
	} while (b < 0);
	return (uint8_t)b;
}

/** Forces any transmitted data to be sent over USB immediately
 *
 */
static ALWAYS_INLINE inline void USBCDC_Flush(void)
{
	CDC_Device_Flush(&VirtualSerial_CDC_Interface);
}

#endif /* HAL_AT90USB646_USBCDC_HW_H_ */
