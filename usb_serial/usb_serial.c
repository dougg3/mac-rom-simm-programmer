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
			//CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Reading...");
			//CDC_Device_Flush(&VirtualSerial_CDC_Interface);

			/*uint32_t mem = ExternalMem_ReadData();

			char dataString[11];

			sprintf(dataString, "%08lX\r\n", mem);

			CDC_Device_SendString(&VirtualSerial_CDC_Interface, dataString);

			sprintf(dataString, "%02X%02X%02X%02X\r\n",
					(uint8_t)(mem>>24),
					(uint8_t)(mem>>16),
					(uint8_t)(mem>>8),
					(uint8_t)(mem>>0));

			CDC_Device_SendString(&VirtualSerial_CDC_Interface, dataString);

			sprintf(dataString, "%02X %02X %02X\r\n", DDRA, DDRC, DDRD);

			CDC_Device_SendString(&VirtualSerial_CDC_Interface, dataString);*/

#define BUFSIZE 128UL
			static uint32_t readBuf[BUFSIZE];
			int x;
			for (x = 0; x < 512UL * 1024UL / BUFSIZE; x++)
			{
				ExternalMem_Read(x*BUFSIZE, readBuf, BUFSIZE);

				if (CDC_Device_SendData(&VirtualSerial_CDC_Interface, (const char *)readBuf, BUFSIZE*4) != ENDPOINT_RWSTREAM_NoError)
				{
					PORTD |= (1 << 7);
				}

				//int y;
				//for (y = 0; y < BUFSIZE; y++)
				//{
					//if ((y % 4) == 0) CDC_Device_SendString(&VirtualSerial_CDC_Interface, ".\r\n");

					//char tmpBuf[20];
					//sprintf(tmpBuf, "%02X %02X %02X %02X ",
					//		(uint8_t)(readBuf[y] >> 24),
					//		(uint8_t)(readBuf[y] >> 16),
					//		(uint8_t)(readBuf[y] >> 8),
					//		(uint8_t)(readBuf[y] >> 0));
					//CDC_Device_SendString(&VirtualSerial_CDC_Interface, tmpBuf);
				//}
				//if ((x % 64) == 0) CDC_Device_SendString(&VirtualSerial_CDC_Interface, "\r\n");
				//CDC_Device_SendString(&VirtualSerial_CDC_Interface, ".");
				//CDC_Device_Flush(&VirtualSerial_CDC_Interface);
			}

			//CDC_Device_SendString(&VirtualSerial_CDC_Interface, "\r\nDone\r\n");
		}

		/*int16_t rb = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
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
		{*/
			//if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface, 'A') != ENDPOINT_READYWAIT_NoError)
			/*if (CDC_Device_SendData(&VirtualSerial_CDC_Interface,
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					"ASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDFASDF"
					, 2048) != ENDPOINT_RWSTREAM_NoError)
			{
				PORTD |= (1 << 7);
			}
			else
			{
				PORTD &= ~(1 << 7);
			}*/
		//}
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
