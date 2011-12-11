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

static int gotChar = 0;
void USBSerial_Check(void)
{
	if (USB_DeviceState == DEVICE_STATE_Configured)
	{
		if (CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface))
		{
			CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

			/*#define BUFSIZE 128UL
			static uint32_t readBuf[BUFSIZE];
			int x;
			for (x = 0; x < 512UL * 1024UL / BUFSIZE; x++)
			{
				ExternalMem_Read(x*BUFSIZE, readBuf, BUFSIZE);

				if (CDC_Device_SendData(&VirtualSerial_CDC_Interface, (const char *)readBuf, BUFSIZE*4) != ENDPOINT_RWSTREAM_NoError)
				{
					PORTD |= (1 << 7);
				}
			}*/

			// Start out in programming state...
			ExternalMem_DeassertCS();
			ExternalMem_DeassertOE();
			ExternalMem_DeassertWE();
			_delay_us(1);

			// Write 0xAA to 0x555 (first byte of unlock sequence)
			// (The SST39SF040 asks for 0x5555 here -- but it doesn't matter, it's
			// just an alternating pattern of 1s and 0s and the chips will ignore
			// the bits above the value it asks for -- so I can just write a huge
			// alternating pattern and it will work with most/all chips)
			ExternalMem_SetAddressAndData(0x55555555UL, 0xAAAAAAAAUL);
			ExternalMem_AssertCS();
			ExternalMem_AssertWE();
			_delay_us(1);
			ExternalMem_DeassertWE();
			_delay_us(1);

			// Write 0x55 to 0x2AA (second byte of unlock sequence)
			ExternalMem_SetAddressAndData(0xAAAAAAAAUL, 0x55555555UL);
			ExternalMem_AssertWE();
			_delay_us(1);
			ExternalMem_DeassertWE();
			_delay_us(1);

			// Write 0x90 to 0x555 (autoselect command)
			ExternalMem_SetAddressAndData(0x55555555UL, 0x90909090UL);
			ExternalMem_AssertWE();
			_delay_us(1);
			ExternalMem_DeassertWE();
			//ExternalMem_DeassertCS();
			_delay_us(1);

			// Now we can start reading...
			ExternalMem_SetAddress(0x0);
			ExternalMem_SetDataAsInput();
			ExternalMem_AssertOE();
			//ExternalMem_AssertCS();

			_delay_us(1);

			uint32_t result = ExternalMem_ReadData();

			char test[20];
			sprintf(test, "%08lX ", result);
			CDC_Device_SendString(&VirtualSerial_CDC_Interface, test);

			ExternalMem_SetAddress(0x1);
			result = ExternalMem_ReadData();
			sprintf(test, "%08lX\r\n", result);
			CDC_Device_SendString(&VirtualSerial_CDC_Interface, test);

			// Exit back to normal mode...
			ExternalMem_DeassertOE();
			ExternalMem_SetAddressAndData(0x0, 0xF0F0F0F0UL);
			//ExternalMem_AssertCS();
			ExternalMem_AssertWE();
			_delay_us(1);
			ExternalMem_DeassertWE();
			//ExternalMem_DeassertCS();
			_delay_us(1);

			// Now do normal read cycle to confirm we exited
			ExternalMem_SetAddress(0x0);
			ExternalMem_SetDataAsInput();
			ExternalMem_AssertOE();
			//ExternalMem_AssertCS();

			_delay_us(1);

			result = ExternalMem_ReadData();

			sprintf(test, "%08lX ", result);
			CDC_Device_SendString(&VirtualSerial_CDC_Interface, test);

			ExternalMem_SetAddress(0x1);
			result = ExternalMem_ReadData();
			sprintf(test, "%08lX\r\n", result);
			CDC_Device_SendString(&VirtualSerial_CDC_Interface, test);
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
