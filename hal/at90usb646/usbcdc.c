/*
 * usbcdc.c
 *
 *  Created on: Nov 22, 2020
 *      Author: Doug
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
