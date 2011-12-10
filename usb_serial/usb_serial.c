/*
 * usb_serial.c
 *
 *  Created on: Dec 9, 2011
 *      Author: Doug
 */

#include "usb_serial.h"
#include "../LUFA/Drivers/USB/USB.h"
#include "../Descriptors.h"

USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
{
	.Config =
		{
			.ControlInterfaceNumber         = 0,

			.DataINEndpointNumber           = CDC_TX_EPNUM,
			.DataINEndpointSize             = CDC_TXRX_EPSIZE,
			.DataINEndpointDoubleBank       = false,

			.DataOUTEndpointNumber          = CDC_RX_EPNUM,
			.DataOUTEndpointSize            = CDC_TXRX_EPSIZE,
			.DataOUTEndpointDoubleBank      = false,

			.NotificationEndpointNumber     = CDC_NOTIFICATION_EPNUM,
			.NotificationEndpointSize       = CDC_NOTIFICATION_EPSIZE,
			.NotificationEndpointDoubleBank = false,
		},
};

void USBSerial_Init(void)
{
	USB_Init();
}

static int gotChar = 0;
void USBSerial_Check(void)
{
	if (USB_DeviceState == DEVICE_STATE_Configured)
	{
		int16_t rb = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		if (rb >= 0)
		{
			if (rb == '.')
			{
				gotChar = 0;
			}
			else
			{
				gotChar = 1;
			}
		}

		if (gotChar)
		{
			//if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface, 'A') != ENDPOINT_READYWAIT_NoError)
			if (CDC_Device_SendData(&VirtualSerial_CDC_Interface,
					"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 64) != ENDPOINT_RWSTREAM_NoError)
			{
				PORTD |= (1 << 7);
			}
		}
	}

	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
