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
#include "../tests/simm_electrical_test.h"

#define READ_CHUNK_SIZE_BYTES		1024UL
#if ((READ_CHUNK_SIZE_BYTES % 4) != 0)
#error Read chunk size should be a multiple of 4 bytes
#endif

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

void USBSerial_Init(void)
{
	USB_Init();
}

typedef enum ProgrammerCommandState
{
	WaitingForCommand = 0,
	ReadingByteWaitingForAddress,
	ReadingChips,
	ReadingChipsUnableSendError,
} ProgrammerCommandState;

typedef enum ProgrammerCommand
{
	EnterWaitingMode = 0,
	DoElectricalTest,
	IdentifyChips,
	ReadByte,
	ReadChips,
	EraseChips,
	WriteChips,
} ProgrammerCommand;

typedef enum ProgrammerReply
{
	ReplyOK,
	ReplyError,
} ProgrammerReply;

static ProgrammerCommandState curCommandState = WaitingForCommand;
static uint8_t byteAddressReceiveCount = 0;
static uint16_t curReadIndex;

void USBSerial_HandleWaitingForCommandByte(uint8_t byte);
void USBSerial_SendReadDataChunk(void);

void USBSerial_Check(void)
{
	if (USB_DeviceState == DEVICE_STATE_Configured)
	{
		if (CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface))
		{
			uint8_t rb = (uint8_t)CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

			if (rb == 'i')
			{
				struct ChipID chips[4];
				ExternalMem_IdentifyChips(chips);
				char tmp[20];
				uint32_t data = ExternalMem_ReadCycle(0);

				int x;
				for (x = 0; x < 4; x++)
				{
					sprintf(tmp, "IC%d: M%02X, D%02X\r\n", x+1, chips[x].manufacturerID, chips[x].deviceID);
					CDC_Device_SendString(&VirtualSerial_CDC_Interface, tmp);
				}

				sprintf(tmp, "%08lX\r\n", data);
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, tmp);
			}
			else if (rb == 'e')
			{
				ExternalMem_EraseChips(ALL_CHIPS);
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Erased\r\n");
			}
			else if (rb == 'r')
			{
				uint32_t result = ExternalMem_ReadCycle(0);
				char tmp[20];
				sprintf(tmp, "%08lX\r\n", result);
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, tmp);
			}
			else if (rb == 'w')
			{
				uint32_t address = 0;
				uint32_t x;
				uint32_t y;
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Writing...\r\n");
				CDC_Device_Flush(&VirtualSerial_CDC_Interface);
				for (y = 0; y < 512UL*1024UL / (READ_CHUNK_SIZE_BYTES/4); y++)
				{
					for (x = 0; x < READ_CHUNK_SIZE_BYTES/4; x++)
					{
						ExternalMem_WriteByteToChips(address++, 0x12345678, ALL_CHIPS);
					}
				}
				//ExternalMem_WriteByteToChips(0, 0x12345678UL, ALL_CHIPS);
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Wrote\r\n");
			}
		}
	}

	/*if (USB_DeviceState == DEVICE_STATE_Configured)
	{
		// Check for commands, etc...
		int16_t recvByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

		if (recvByte >= 0)
		{
			switch (curCommandState)
			{
			case WaitingForCommand:
				USBSerial_HandleWaitingForCommandByte((uint8_t)recvByte);
				break;
			}
		}
	}*/

	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

void USBSerial_HandleWaitingForCommandByte(uint8_t byte)
{
	switch (byte)
	{
	case EnterWaitingMode:
		curCommandState = WaitingForCommand;
		break;
	case DoElectricalTest:
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ReplyOK);
		SIMMElectricalTest_Run();
		curCommandState = WaitingForCommand;
		break;
	case IdentifyChips:
	{
		struct ChipID chips[4];
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ReplyOK);
		ExternalMem_IdentifyChips(chips);
		// TODO: Send chip ID info back to receiver
		break;
	}
	case ReadByte:
		curCommandState = ReadingByteWaitingForAddress;
		byteAddressReceiveCount = 0;
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ReplyOK);
		break;
	case ReadChips:
		curCommandState = ReadingChips;
		curReadIndex = 0;
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ReplyOK);
		USBSerial_SendReadDataChunk();
		break;
	case EraseChips:
	case WriteChips:
		break;
	}
}

void USBSerial_SendReadDataChunk(void)
{
	// TODO: How do I send an error back to the device?
	// Maybe the device, when it tries to request the next data chunk,
	// will get an ERROR response instead of an "OK" response?

	static union
	{
		uint32_t readChunks[READ_CHUNK_SIZE_BYTES / 4];
		uint8_t readChunkBytes[READ_CHUNK_SIZE_BYTES];
	} chunks;

	ExternalMem_Read(curReadIndex * (READ_CHUNK_SIZE_BYTES/4), chunks.readChunks, READ_CHUNK_SIZE_BYTES/4);
	uint8_t retVal = CDC_Device_SendData(&VirtualSerial_CDC_Interface, (const char *)chunks.readChunkBytes, READ_CHUNK_SIZE_BYTES);
	if (retVal != ENDPOINT_RWSTREAM_NoError)
	{
		curCommandState = ReadingChipsUnableSendError;
	}
	else
	{
		curReadIndex++;
	}
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
