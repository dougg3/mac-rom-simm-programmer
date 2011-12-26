/*
 * usb_serial.c
 *
 *  Created on: Dec 9, 2011
 *      Author: Doug
 */

#include "usb_serial.h"
#include "../LUFA/Drivers/USB/USB.h"
#include "../cdc_device_definition.h"
#include "../external_mem.h"
#include "../tests/simm_electrical_test.h"
#include "../programmer_protocol.h"

#define CHIP_SIZE					(512UL * 1024UL)
#define READ_CHUNK_SIZE_BYTES		1024UL
#define WRITE_CHUNK_SIZE_BYTES		1024UL
#if ((READ_CHUNK_SIZE_BYTES % 4) != 0)
#error Read chunk size should be a multiple of 4 bytes
#endif
#if ((WRITE_CHUNK_SIZE_BYTES % 4) != 0)
#error Write chunk size should be a multiple of 4 bytes
#endif

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
	WritingChips
} ProgrammerCommandState;

static ProgrammerCommandState curCommandState = WaitingForCommand;
static uint8_t byteAddressReceiveCount = 0;
static uint16_t curReadIndex;
static int16_t writePosInChunk = -1;
static uint16_t curWriteIndex = 0;

void USBSerial_HandleWaitingForCommandByte(uint8_t byte);
void USBSerial_HandleReadingChipsByte(uint8_t byte);
void USBSerial_SendReadDataChunk(void);
void USBSerial_HandleWritingChipsByte(uint8_t byte);
void USBSerial_ElectricalTest_Fail_Handler(uint8_t index1, uint8_t index2);

#define SendByte(b) CDC_Device_SendByte(&VirtualSerial_CDC_Interface, b)
#define ReadByte() CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface)
#define SendData(d, l) CDC_Device_SendData(&VirtualSerial_CDC_Interface, d, l)

void USBSerial_Check(void)
{
	/*if (USB_DeviceState == DEVICE_STATE_Configured)
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
			else if (rb == 't')
			{
				int result = SIMMElectricalTest_Run();

				char tmp[20];
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, "SIMM electrical test complete: ");
				sprintf(tmp, "%d errors\r\n", result);
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, tmp);
			}
			else if (rb == 'a')
			{
				uint32_t x;
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Reading...\r\n");
				CDC_Device_Flush(&VirtualSerial_CDC_Interface);

				for (x = 0; x < 512UL*1024UL; x++)
				{
					ExternalMem_ReadCycle(x);
				}

				CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Finished\r\n");
			}
		}
	}*/

	if (USB_DeviceState == DEVICE_STATE_Configured)
	{
		// Check for commands, etc...
		int16_t recvByte = ReadByte();

		if (recvByte >= 0)
		{
			switch (curCommandState)
			{
			case WaitingForCommand:
				USBSerial_HandleWaitingForCommandByte((uint8_t)recvByte);
				break;
			case ReadingChips:
				USBSerial_HandleReadingChipsByte((uint8_t)recvByte);
				break;
			case WritingChips:
				USBSerial_HandleWritingChipsByte((uint8_t)recvByte);
				break;
			}
		}
	}

	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

void USBSerial_HandleWaitingForCommandByte(uint8_t byte)
{
	switch (byte)
	{
	case EnterWaitingMode:
		SendByte(CommandReplyOK);
		curCommandState = WaitingForCommand;
		break;
	case DoElectricalTest:
		SendByte(CommandReplyOK);
		SIMMElectricalTest_Run(USBSerial_ElectricalTest_Fail_Handler);
		SendByte(ProgrammerElectricalTestDone);
		curCommandState = WaitingForCommand;
		break;
	case IdentifyChips:
	{
		struct ChipID chips[4];
		SendByte(CommandReplyOK);
		ExternalMem_IdentifyChips(chips);
		int x;
		for (x = 0; x < 4; x++)
		{
			SendByte(chips[x].manufacturerID);
			SendByte(chips[x].deviceID);
		}
		SendByte(ProgrammerIdentifyDone);
		break;
	}
	case ReadByte:
		curCommandState = ReadingByteWaitingForAddress;
		byteAddressReceiveCount = 0;
		SendByte(CommandReplyOK);
		break;
	case ReadChips:
		curCommandState = ReadingChips;
		curReadIndex = 0;
		SendByte(CommandReplyOK);
		USBSerial_SendReadDataChunk();
		break;
	case EraseChips:
		ExternalMem_EraseChips(ALL_CHIPS);
		SendByte(CommandReplyOK);
		break;
	case WriteChips:
		curCommandState = WritingChips;
		curWriteIndex = 0;
		writePosInChunk = -1;
		SendByte(CommandReplyOK);
		break;
	case GetBootloaderState:
		SendByte(CommandReplyOK);
		SendByte(BootloaderStateInProgrammer);
		break;
	case EnterBootloader:
		SendByte(CommandReplyOK);
		USB_Disable();

		// Disable interrupts...
		cli();

		// Wait a little bit to let everything settle and let the program close the port after the USB disconnect
		_delay_ms(2000);

		// Now run the bootloader
		__asm__ __volatile__ ( "jmp 0xE000" );
		break;
	case EnterProgrammer:
		// Already in the programmer
		SendByte(CommandReplyOK);
		break;
	default:
		SendByte(CommandReplyInvalid);
		break;
	}
}

void USBSerial_HandleReadingChipsByte(uint8_t byte)
{
	switch (byte)
	{
	case ComputerReadOK:
		if (curReadIndex >= (CHIP_SIZE / (READ_CHUNK_SIZE_BYTES/4)))
		{
			SendByte(ProgrammerReadFinished);
			curCommandState = WaitingForCommand;
		}
		else
		{
			SendByte(ProgrammerReadMoreData);
			USBSerial_SendReadDataChunk();
		}
		break;
	case ComputerReadCancel:
		SendByte(ProgrammerReadConfirmCancel);
		curCommandState = WaitingForCommand;
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
	uint8_t retVal = SendData((const char *)chunks.readChunkBytes, READ_CHUNK_SIZE_BYTES);
	if (retVal != ENDPOINT_RWSTREAM_NoError)
	{
		curCommandState = ReadingChipsUnableSendError;
	}
	else
	{
		curReadIndex++;
	}
}

void USBSerial_HandleWritingChipsByte(uint8_t byte)
{
	static union
	{
		uint32_t writeChunks[WRITE_CHUNK_SIZE_BYTES / 4];
		uint8_t writeChunkBytes[WRITE_CHUNK_SIZE_BYTES];
	} chunks;

	if (writePosInChunk == -1)
	{
		switch (byte)
		{
		case ComputerWriteMore:
			writePosInChunk = 0;
			if (curWriteIndex < CHIP_SIZE / (WRITE_CHUNK_SIZE_BYTES/4))
			{
				SendByte(ProgrammerWriteOK);
			}
			else
			{
				SendByte(ProgrammerWriteError);
			}
			break;
		case ComputerWriteFinish:
			// Just to confirm that we finished writing...
			SendByte(ProgrammerWriteOK);
			curCommandState = WaitingForCommand;
			break;
		case ComputerWriteCancel:
			SendByte(ProgrammerWriteConfirmCancel);
			curCommandState = WaitingForCommand;
			break;
		}
	}
	else
	{
		chunks.writeChunkBytes[writePosInChunk++] = byte;
		if (writePosInChunk >= WRITE_CHUNK_SIZE_BYTES)
		{
			// Write this data
			ExternalMem_Write(curWriteIndex * (WRITE_CHUNK_SIZE_BYTES/4), chunks.writeChunks, WRITE_CHUNK_SIZE_BYTES/4, ALL_CHIPS);
			SendByte(ProgrammerWriteOK);
			curWriteIndex++;
			writePosInChunk = -1;
		}
	}
}

void USBSerial_ElectricalTest_Fail_Handler(uint8_t index1, uint8_t index2)
{
	// Sends out a failure notice -- followed by indexes of the two shorted pins
	SendByte(ProgrammerElectricalTestFail);
	SendByte(index1);
	SendByte(index2);
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
