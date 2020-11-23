/*
 * usbcdc_hw.h
 *
 *  Created on: Nov 22, 2020
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

#ifndef HAL_AT90USB646_USBCDC_HW_H_
#define HAL_AT90USB646_USBCDC_HW_H_

#include "LUFA/Drivers/USB/USB.h"
#include "cdc_device_definition.h"

/** Sends a byte over the USB CDC serial port
 *
 * @param byte The byte to send
 */
static __attribute__((always_inline)) inline void USBCDC_SendByte(uint8_t byte)
{
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, byte);
}

/** Sends a block of data over the USB CDC serial port
 *
 * @param data The data to send
 * @param len The number of bytes
 * @return True on success, false on failure
 */
static __attribute__((always_inline)) inline bool USBCDC_SendData(uint8_t const *data, uint16_t len)
{
	return CDC_Device_SendData(&VirtualSerial_CDC_Interface, (char const *)data, len) == ENDPOINT_RWSTREAM_NoError;
}

/** Attempts to read a byte from the USB CDC serial port
 *
 * @return The byte read, or -1 if there are no bytes available
 */
static __attribute__((always_inline)) inline int16_t USBCDC_ReadByte(void)
{
	return CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
}

/** Forces any transmitted data to be sent over USB immediately
 *
 */
static __attribute__((always_inline)) inline void USBCDC_Flush(void)
{
	CDC_Device_Flush(&VirtualSerial_CDC_Interface);
}

#endif /* HAL_AT90USB646_USBCDC_HW_H_ */
