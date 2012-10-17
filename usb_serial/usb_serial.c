/*
 * usb_serial.c
 *
 *  Created on: Dec 9, 2011
 *      Author: Doug
 *
 * Copyright (C) 2011-2012 Doug Brown
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

#include "usb_serial.h"
#include "../LUFA/Drivers/USB/USB.h"
#include "../cdc_device_definition.h"
#include "../external_mem.h"
#include "../tests/simm_electrical_test.h"
#include "../programmer_protocol.h"
#include "../led.h"
#include <stdbool.h>

#define MAX_CHIP_SIZE				(2UL * 1024UL * 1024UL)
#define READ_CHUNK_SIZE_BYTES		1024UL
#define WRITE_CHUNK_SIZE_BYTES		1024UL
#define ERASE_SECTOR_SIZE_BYTES		(256UL * 1024UL)
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

// Internal state so we know how to interpret the next-received byte
typedef enum ProgrammerCommandState
{
	WaitingForCommand = 0,
	ReadingByteWaitingForAddress,
	ReadingChipsReadLength,
	ReadingChips,
	ReadingChipsUnableSendError,
	WritingChips,
	ErasePortionReadingPosLength
} ProgrammerCommandState;
static ProgrammerCommandState curCommandState = WaitingForCommand;

// State info for reading/writing
static uint8_t byteAddressReceiveCount = 0;
static uint16_t curReadIndex;
static uint32_t readLength;
static uint8_t readLengthByteIndex;
static int16_t writePosInChunk = -1;
static uint16_t curWriteIndex = 0;
static bool verifyDuringWrite = false;
static uint32_t erasePosition;
static uint32_t eraseLength;

// Private functions
void USBSerial_HandleWaitingForCommandByte(uint8_t byte);
void USBSerial_HandleReadingChipsByte(uint8_t byte);
void USBSerial_HandleReadingChipsReadLengthByte(uint8_t byte);
void USBSerial_SendReadDataChunk(void);
void USBSerial_HandleWritingChipsByte(uint8_t byte);
void USBSerial_ElectricalTest_Fail_Handler(uint8_t index1, uint8_t index2);
void USBSerial_HandleErasePortionReadPosLengthByte(uint8_t byte);

// Read/write to USB serial macros -- easier than retyping
// CDC_Device_XXX(&VirtualSerial_CDC_Interface...) every time
#define SendByte(b) CDC_Device_SendByte(&VirtualSerial_CDC_Interface, b)
#define ReadByte() CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface)
#define SendData(d, l) CDC_Device_SendData(&VirtualSerial_CDC_Interface, d, l)

// Should be called periodically in the main loop
void USBSerial_Check(void)
{
	// If we're configured, read a byte (if one is available) and process it
	if (USB_DeviceState == DEVICE_STATE_Configured)
	{
		int16_t recvByte = ReadByte();

		// Did we get a byte? If so, hand it off to the correct handler
		// function based on the current state
		if (recvByte >= 0)
		{
			switch (curCommandState)
			{
			case WaitingForCommand:
				USBSerial_HandleWaitingForCommandByte((uint8_t)recvByte);
				break;
			case ReadingChipsReadLength:
				USBSerial_HandleReadingChipsReadLengthByte((uint8_t)recvByte);
				break;
			case ReadingChips:
				USBSerial_HandleReadingChipsByte((uint8_t)recvByte);
				break;
			case WritingChips:
				USBSerial_HandleWritingChipsByte((uint8_t)recvByte);
				break;
			case ErasePortionReadingPosLength:
				USBSerial_HandleErasePortionReadPosLengthByte((uint8_t)recvByte);
				break;
			}
		}
	}

	// And do the periodic CDC and USB tasks...
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

// If we're in the "waiting for command" state, handle the command...
void USBSerial_HandleWaitingForCommandByte(uint8_t byte)
{
	switch (byte)
	{
	// Asked to enter waiting mode -- we're already there, so say OK.
	case EnterWaitingMode:
		SendByte(CommandReplyOK);
		curCommandState = WaitingForCommand;
		break;
	// Asked to do the electrical test. Reply OK, and then do the test,
	// sending whatever replies necessary
	case DoElectricalTest:
		SendByte(CommandReplyOK);
		// Force LUFA to send initial "OK" reply immediately in this case
		// so the caller gets immediate feedback that the test has started
		CDC_Device_Flush(&VirtualSerial_CDC_Interface);
		SIMMElectricalTest_Run(USBSerial_ElectricalTest_Fail_Handler);
		SendByte(ProgrammerElectricalTestDone);
		curCommandState = WaitingForCommand;
		break;
	// Asked to identify the chips in the SIMM. Identify them and send reply.
	case IdentifyChips:
	{
		struct ChipID chips[NUM_CHIPS];
		SendByte(CommandReplyOK);
		ExternalMem_IdentifyChips(chips);
		int x;
		for (x = 0; x < NUM_CHIPS; x++)
		{
			SendByte(chips[x].manufacturerID);
			SendByte(chips[x].deviceID);
		}
		SendByte(ProgrammerIdentifyDone);
		break;
	}
	// Asked to read a single byte from each SIMM. Change the state and reply.
	case ReadByte:
		curCommandState = ReadingByteWaitingForAddress;
		byteAddressReceiveCount = 0;
		SendByte(CommandReplyOK);
		break;
	// Asked to read all four chips. Set the state, reply with the first chunk
	case ReadChips:
		curCommandState = ReadingChipsReadLength;
		curReadIndex = 0;
		readLengthByteIndex = 0;
		readLength = 0;
		SendByte(CommandReplyOK);
		break;
	// Erase the chips and reply OK. (TODO: Sometimes erase might fail)
	case EraseChips:
		ExternalMem_EraseChips(ALL_CHIPS);
		SendByte(CommandReplyOK);
		break;
	// Begin writing the chips. Change the state, reply, wait for chunk of data
	case WriteChips:
		curCommandState = WritingChips;
		curWriteIndex = 0;
		writePosInChunk = -1;
		SendByte(CommandReplyOK);
		break;
	// Asked for the current bootloader state. We are in the program right now,
	// so reply accordingly.
	case GetBootloaderState:
		SendByte(CommandReplyOK);
		SendByte(BootloaderStateInProgrammer);
		break;
	// Enter the bootloader. Wait a bit, then jump to the bootloader location.
	case EnterBootloader:
		SendByte(CommandReplyOK);
		CDC_Device_Flush(&VirtualSerial_CDC_Interface);

		// Insert a small delay to ensure that it arrives before rebooting.
		_delay_ms(1000);

		// Done with the USB interface -- the bootloader will re-initialize it.
		USB_Disable();

		// Disable interrupts so nothing weird happens...
		cli();

		// Wait a little bit to let everything settle and let the program
		// close the port after the USB disconnect
		_delay_ms(2000);

		// And, of course, go into the bootloader.
		__asm__ __volatile__ ( "jmp 0xE000" );
		break;
	// Enter the programmer. We're already there, so reply OK.
	case EnterProgrammer:
		// Already in the programmer
		SendByte(CommandReplyOK);
		break;
	// Set the SIMM type to the older, smaller chip size (2MB and below)
	case SetSIMMTypePLCC32_2MB:
		ExternalMem_SetChipType(ChipType8BitData_4MBitSize);
		SendByte(CommandReplyOK);
		break;
	case SetSIMMTypeLarger:
		ExternalMem_SetChipType(ChipType8Bit16BitData_16MBitSize);
		SendByte(CommandReplyOK);
		break;
	case SetVerifyWhileWriting:
		verifyDuringWrite = true;
		SendByte(CommandReplyOK);
		break;
	case SetNoVerifyWhileWriting:
		verifyDuringWrite = false;
		SendByte(CommandReplyOK);
		break;
	case ErasePortion:
		readLengthByteIndex = 0;
		eraseLength = 0;
		erasePosition = 0;
		curCommandState = ErasePortionReadingPosLength;
		SendByte(CommandReplyOK);
		break;
	// We don't know what this command is, so reply that it was invalid.
	default:
		SendByte(CommandReplyInvalid);
		break;
	}
}

// If we're in the "reading chips" state, handle the incoming byte...
void USBSerial_HandleReadingChipsByte(uint8_t byte)
{
	// The byte should be a reply from the computer. It should be either:
	// 1) ComputerReadOK -- meaning it got the chunk we just sent
	// or
	// 2) ComputerReadCancel -- meaning the user canceled the read
	switch (byte)
	{
	case ComputerReadOK:
		// If they have confirmed the final data chunk, let them know
		// that they have finished, and enter command state.
		if (curReadIndex >= readLength)
		{
			LED_Off();
			SendByte(ProgrammerReadFinished);
			curCommandState = WaitingForCommand;
		}
		else // There's more data left to read, so read it and send it to them!
		{
			LED_Toggle();
			SendByte(ProgrammerReadMoreData);
			USBSerial_SendReadDataChunk();
		}
		break;
	case ComputerReadCancel:
		// If they've canceled, let them know we got their request and go back
		// to "waiting for command" state
		SendByte(ProgrammerReadConfirmCancel);
		curCommandState = WaitingForCommand;
		break;
	}
}

// If we're figuring out the length to read, grab it now...
void USBSerial_HandleReadingChipsReadLengthByte(uint8_t byte)
{
	// There will be four bytes, so count up until we know the length. If they
	// have sent all four bytes, send the first read chunk.
	readLength |= (((uint32_t)byte) << (8*readLengthByteIndex));
	if (++readLengthByteIndex >= 4)
	{
		// Ensure it's within limits and a multiple of 1024
		if ((readLength > NUM_CHIPS * MAX_CHIP_SIZE) ||
			(readLength % READ_CHUNK_SIZE_BYTES) ||
			(readLength == 0))
		{
			SendByte(ProgrammerReadError);
			curCommandState = WaitingForCommand;
		}
		else
		{
			// Convert the length into the number of chunks we need to send
			readLength /= READ_CHUNK_SIZE_BYTES;
			curCommandState = ReadingChips;
			SendByte(ProgrammerReadOK);
			USBSerial_SendReadDataChunk();
		}
	}
}

// Read the next chunk of data from the SIMM and send it off over the serial.
void USBSerial_SendReadDataChunk(void)
{
	// Here's a buffer we will use to read the next chunk of data.
	// It's static because the stack is NOT big enough for it. If I start
	// running low on RAM, I could pull this out of the function and share it
	// with other functions, but I'm not bothering with that for now.
	static union
	{
		uint32_t readChunks[READ_CHUNK_SIZE_BYTES / NUM_CHIPS];
		uint8_t readChunkBytes[READ_CHUNK_SIZE_BYTES];
	} chunks;

	// Read the next chunk of data, send it over USB, and make sure
	// we sent it correctly.
	ExternalMem_Read(curReadIndex * (READ_CHUNK_SIZE_BYTES/NUM_CHIPS),
			chunks.readChunks, READ_CHUNK_SIZE_BYTES/NUM_CHIPS);
	uint8_t retVal = SendData((const char *)chunks.readChunkBytes,
			READ_CHUNK_SIZE_BYTES);

	// If for some reason there was an error, mark it as such. Otherwise,
	// increment our pointer so we know the next chunk of data to send.
	if (retVal != ENDPOINT_RWSTREAM_NoError)
	{
		curCommandState = ReadingChipsUnableSendError;
	}
	else
	{
		curReadIndex++;
	}
}

// Handles a received byte from the computer while we're in the "writing chips"
// mode.
void USBSerial_HandleWritingChipsByte(uint8_t byte)
{
	// A buffer we use to store the incoming data. This, too, could be shared
	// with other functions if I end up running out of RAM. Again, I'm not
	// bothering with that yet, but this could easily be shared with the
	// read function.
	static union
	{
		uint32_t writeChunks[WRITE_CHUNK_SIZE_BYTES / 4];
		uint8_t writeChunkBytes[WRITE_CHUNK_SIZE_BYTES];
	} chunks;

	// This means we have just started the entire process or just finished
	// a chunk, so see what the computer has decided for us to do.
	if (writePosInChunk == -1)
	{
		switch (byte)
		{
		// The computer asked to write more data to the SIMM.
		case ComputerWriteMore:
			writePosInChunk = 0;
			// Make sure we don't write past the capacity of the chips.
			if (curWriteIndex < MAX_CHIP_SIZE / (WRITE_CHUNK_SIZE_BYTES/4))
			{
				SendByte(ProgrammerWriteOK);
			}
			else
			{
				SendByte(ProgrammerWriteError);
				// TODO: Enter waiting for command mode?
			}
			break;
		// The computer said that it's done writing.
		case ComputerWriteFinish:
			LED_Off();
			SendByte(ProgrammerWriteOK);
			curCommandState = WaitingForCommand;
			break;
		// The computer asked to cancel.
		case ComputerWriteCancel:
			LED_Off();
			SendByte(ProgrammerWriteConfirmCancel);
			curCommandState = WaitingForCommand;
			break;
		}
	}
	else // Interpret the incoming byte as data to write to the SIMM.
	{
		// Save the byte, and check if we've filled up an entire chunk
		chunks.writeChunkBytes[writePosInChunk++] = byte;
		if (writePosInChunk >= WRITE_CHUNK_SIZE_BYTES)
		{
			// We filled up the chunk, write it out and confirm it, then wait
			// for the next command from the computer!
			uint8_t writeResult = ExternalMem_Write(curWriteIndex * (WRITE_CHUNK_SIZE_BYTES/4),
					chunks.writeChunks, WRITE_CHUNK_SIZE_BYTES/4, ALL_CHIPS, verifyDuringWrite);

			// But if we asked to verify, make sure it came out OK.
			if (verifyDuringWrite && (writeResult != 0))
			{
				// Uh oh -- verification failure.
				LED_Off();
				// Send the fail bit along with a mask of failed chips.
				SendByte(ProgrammerWriteVerificationError | writeResult);
				curCommandState = WaitingForCommand;
			}
			else
			{
				SendByte(ProgrammerWriteOK);
				curWriteIndex++;
				writePosInChunk = -1;
				LED_Toggle();
			}
		}
	}
}

// Whenever an electrical test failure occurs, this handler will be called
// by it. It sends out a failure notice followed by indexes of the two
// shorted pins.
void USBSerial_ElectricalTest_Fail_Handler(uint8_t index1, uint8_t index2)
{
	SendByte(ProgrammerElectricalTestFail);
	SendByte(index1);
	SendByte(index2);
}

// If we're figuring out the position/length to erase, parse it here.
void USBSerial_HandleErasePortionReadPosLengthByte(uint8_t byte)
{
	// Read in the position and length to erase
	if (readLengthByteIndex < 4)
	{
		erasePosition |= (((uint32_t)byte) << (8*readLengthByteIndex));
	}
	else
	{
		eraseLength |= (((uint32_t)byte) << (8*(readLengthByteIndex - 4)));
	}

	if (++readLengthByteIndex >= 8)
	{
		ChipType chipType = ExternalMem_GetChipType();
		bool eraseSuccess = false;

		// Ensure they are both within limits of sector size erasure
		if (((erasePosition % ERASE_SECTOR_SIZE_BYTES) == 0) &&
			((eraseLength % ERASE_SECTOR_SIZE_BYTES) == 0))
		{
			uint32_t boundary = eraseLength + erasePosition;

			// Ensure they are within the limits of the chip size too
			if (chipType == ChipType8BitData_4MBitSize)
			{
				if (boundary <= (2 * 1024UL * 1024UL))
				{
					// OK! We're erasing certain sectors of a 2 MB SIMM.
					SendByte(ProgrammerErasePortionOK);
					CDC_Device_Flush(&VirtualSerial_CDC_Interface);
					if (ExternalMem_EraseSectors(erasePosition/NUM_CHIPS,
							eraseLength/NUM_CHIPS, ALL_CHIPS))
					{
						eraseSuccess = true;
					}
				}
			}
			else if (chipType == ChipType8Bit16BitData_16MBitSize)
			{
				if (boundary <= (8 * 1024UL * 1024UL))
				{
					// OK! We're erasing certain sectors of an 8 MB SIMM.
					SendByte(ProgrammerErasePortionOK);
					CDC_Device_Flush(&VirtualSerial_CDC_Interface);
					if (ExternalMem_EraseSectors(erasePosition/NUM_CHIPS,
							eraseLength/NUM_CHIPS, ALL_CHIPS))
					{
						eraseSuccess = true;
					}
				}
			}
		}

		if (eraseSuccess)
		{
			// Not on a sector boundary for erase position and/or length
			SendByte(ProgrammerErasePortionFinished);
			curCommandState = WaitingForCommand;
		}
		else
		{
			// Not on a sector boundary for erase position and/or length
			SendByte(ProgrammerErasePortionError);
			curCommandState = WaitingForCommand;
		}
	}
}

// LUFA event handler for when the USB configuration changes.
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

// LUFA event handler for when a USB control request is received
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
