/*
 * usbcdc.c
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

#include "../usbcdc.h"

/** Initializes the USB CDC device
 *
 */
void USBCDC_Init(void)
{
	// TODO: How do we pretend to be a USB device?
}

/** Disables the USB CDC device
 *
 */
void USBCDC_Disable(void)
{
	// TODO: How do we stop pretending to be a USB device?
}

/** Main loop handler for the USB CDC device. Call from the main loop.
 *
 */
void USBCDC_Check(void)
{
	// TODO: Any periodic tasks related to USB functionality
}

/** Sends a byte over the USB CDC serial port
 *
 * @param byte The byte to send
 */
void USBCDC_SendByte(uint8_t byte)
{
	(void)byte; // TODO: How do we send a byte as a pretend USB device?
}

/** Sends a block of data over the USB CDC serial port
 *
 * @param data The data to send
 * @param len The number of bytes
 * @return True on success, false on failure
 */
bool USBCDC_SendData(uint8_t const *data, uint16_t len)
{
	(void)data; (void)len; // TODO: How do we send data as a pretend USB device?
	return false;
}

/** Attempts to read a byte from the USB CDC serial port
 *
 * @return The byte read, or -1 if there are no bytes available
 */
int16_t USBCDC_ReadByte(void)
{
	// TODO: How do we read data as a pretend USB device?
	return -1;
}

/** Reads a byte from the USB CDC serial port. Blocks until one is available.
 *
 * @return The byte read
 */
uint8_t USBCDC_ReadByteBlocking(void)
{
	// TODO: How do we read data (blocking) as a pretend USB device?
	return 0;
}

/** Forces any transmitted data to be sent over USB immediately
 *
 */
void USBCDC_Flush(void)
{
	// TODO: How do we flush data sent to a pretend USB device?
}
