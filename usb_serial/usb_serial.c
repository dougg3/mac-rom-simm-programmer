/*
 * usb_serial.c
 *
 *  Created on: Dec 9, 2011
 *      Author: Doug
 */

#include "usb_serial.h"
#include "../LUFA/Drivers/USB/USB.h"
#include "../Descriptors.h"
#include "../external_mem.h"

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

void USBSerial_Check(void)
{
	/*if (USB_DeviceState == DEVICE_STATE_Configured)
	{
		if (CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface))
		{
			CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

			struct ChipID chips[4];
			ExternalMem_IdentifyChips(chips);
			char tmp[20];

			int x;
			for (x = 0; x < 4; x++)
			{
				sprintf(tmp, "IC%d: M%02X, D%02X\r\n", x, chips[x].manufacturerID, chips[x].deviceID);
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, tmp);
			}
		}
	}*/

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
