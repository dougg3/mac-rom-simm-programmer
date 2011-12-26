/*
 * cdc_device_definition.c
 *
 *  Created on: Dec 26, 2011
 *      Author: Doug
 */

#include "LUFA/Drivers/USB/USB.h"
#include "Descriptors.h"

USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
{
	.Config =
		{
			.ControlInterfaceNumber         = 0,

			.DataINEndpointNumber           = CDC_TX_EPNUM,
			.DataINEndpointSize             = CDC_TXRX_EPSIZE,
			.DataINEndpointDoubleBank       = true,

			.DataOUTEndpointNumber          = CDC_RX_EPNUM,
			.DataOUTEndpointSize            = CDC_TXRX_EPSIZE,
			.DataOUTEndpointDoubleBank      = true,

			.NotificationEndpointNumber     = CDC_NOTIFICATION_EPNUM,
			.NotificationEndpointSize       = CDC_NOTIFICATION_EPSIZE,
			.NotificationEndpointDoubleBank = false,
		},
};
