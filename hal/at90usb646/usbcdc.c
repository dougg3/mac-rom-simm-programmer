/*
 * usbcdc.c
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

#include "../usbcdc.h"
#include "LUFA/Drivers/USB/USB.h"
#include "cdc_device_definition.h"

/** Initializes the USB CDC device
 *
 */
void USBCDC_Init(void)
{
	// Initialize LUFA
	USB_Init();
}

/** Disables the USB CDC device
 *
 */
void USBCDC_Disable(void)
{
	// Disable LUFA, this will cause us to no longer identify as a USB device
	USB_Disable();
}

/** Main loop handler for the USB CDC device. Call from the main loop.
 *
 */
void USBCDC_Check(void)
{
	// Do the periodic CDC and USB tasks in LUFA
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

/** LUFA event handler for when the USB configuration changes.
 *
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** LUFA event handler for when a USB control request is received
 *
 */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
