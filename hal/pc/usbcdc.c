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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

/// The emulated USB CDC serial port associated with the faked USB device
static const char *gadgetPort = "/dev/ttyGS0";
/// File descriptor for the port
static int gadgetfd;

/** Initializes the USB CDC device
 *
 */
void USBCDC_Init(void)
{
	// Attempt to open the device end of the USB CDC gadget.
	gadgetfd = open(gadgetPort, O_RDWR | O_CLOEXEC | O_NOCTTY);
	if (gadgetfd < 0)
	{
		// If an error occurs, print a message and bail
		fprintf(stderr, "Unable to open USB gadget port (%s).\n", gadgetPort);
		exit(1);
	}

	// Set up for raw I/O without weird extra characters
	struct termios options;
	if (tcgetattr(gadgetfd, &options) < 0)
	{
		fprintf(stderr, "Unable to get options on USB gadget port\n");
		exit(1);
	}
	cfmakeraw(&options);
	if (tcsetattr(gadgetfd, TCSANOW, &options) < 0)
	{
		fprintf(stderr, "Unable to set options on USB gadget port\n");
		exit(1);
	}

	// TODO: Actually allow the faked device to enumerate now, should probably
	// check earlier in the process to see if the file is even there
}

/** Disables the USB CDC device
 *
 */
void USBCDC_Disable(void)
{
	// TODO: "unplug" the simulated device
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
	// TODO: write can be interrupted, look at SA_RESTART?
	if (write(gadgetfd, &byte, sizeof(byte)) != sizeof(byte))
	{
		fprintf(stderr, "Warning: Byte TX failed\n");
	}
}

/** Sends a block of data over the USB CDC serial port
 *
 * @param data The data to send
 * @param len The number of bytes
 * @return True on success, false on failure
 */
bool USBCDC_SendData(uint8_t const *data, uint16_t len)
{
	// TODO: write can be interrupted, look at SA_RESTART?
	return (write(gadgetfd, data, len) == len);
}

/** Attempts to read a byte from the USB CDC serial port
 *
 * @return The byte read, or -1 if there are no bytes available
 */
int16_t USBCDC_ReadByte(void)
{
	int bytes;
	if (ioctl(gadgetfd, FIONREAD, &bytes) == 0 && bytes > 0)
	{
		uint8_t b;
		// TODO: read can be interrupted, look at SA_RESTART?
		if (read(gadgetfd, &b, sizeof(b)) == sizeof(b))
		{
			return b;
		}
	}
	return -1;
}

/** Reads a byte from the USB CDC serial port. Blocks until one is available.
 *
 * @return The byte read
 */
uint8_t USBCDC_ReadByteBlocking(void)
{
	uint8_t b = 0;
	// TODO: read can be interrupted, look at SA_RESTART?
	if (read(gadgetfd, &b, sizeof(b)) != sizeof(b))
	{
		fprintf(stderr, "Warning: Byte RX failed\n");
	}
	return b;
}

/** Forces any transmitted data to be sent over USB immediately
 *
 */
void USBCDC_Flush(void)
{
	tcflush(gadgetfd, TCOFLUSH);
}
