/*
 * simm_programmer.c
 *
 *  Created on: Dec 9, 2011
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

#include "simm_programmer.h"
#include "hal/usbcdc.h"
#include "drivers/parallel_flash.h"
#include "tests/simm_electrical_test.h"
#include "programmer_protocol.h"
#include "led.h"
#include "hardware.h"
#include <stdbool.h>

/// Maximum size of an individual chip on a SIMM we read
#define MAX_CHIP_SIZE				(2UL * 1024UL * 1024UL)
/// Number of bytes we read/write at once
#define READ_WRITE_CHUNK_SIZE_BYTES	1024UL
/// Make sure the chunk size is a multiple of 4 bytes, since there are 4 chips
#if ((READ_WRITE_CHUNK_SIZE_BYTES % 4) != 0)
#error Read/write chunk size should be a multiple of 4 bytes
#endif
/// The smallest granularity for sector erase that we support
#define ERASE_SECTOR_SIZE_BYTES		(256UL * 1024UL)

/// Internal state so we know how to interpret the next-received byte
typedef enum ProgrammerCommandState
{
	WaitingForCommand = 0,       //!< No active commands
	ReadingChipsReadLength,      //!< Reading the length for reading data from the SIMM
	ReadingChips,                //!< Reading data from the SIMM
	WritingChips,                //!< Writing data to the SIMM
	ErasePortionReadingPosLength,//!< Reading the length of SIMM data to erase
	ReadingChipsReadStartPos,    //!< Reading the start position for reading data from the SIMM
	WritingChipsReadingStartPos, //!< Reading the start position for writing data to the SIMM
	ReadingChipsMask,            //!< Reading the bitmask of which chips should be programmed
} ProgrammerCommandState;
static ProgrammerCommandState curCommandState = WaitingForCommand;

// State info for reading/writing
static uint16_t curReadIndex;
static uint32_t readLength;
static uint8_t readLengthByteIndex;
static int16_t writePosInChunk = -1;
static uint16_t curWriteIndex = 0;
static bool verifyDuringWrite = false;
static uint32_t erasePosition;
static uint32_t eraseLength;
static uint8_t chipsMask = ALL_CHIPS;

/// Buffers we use to store incoming/outgoing data.
static union
{
	uint32_t words[READ_WRITE_CHUNK_SIZE_BYTES / PARALLEL_FLASH_NUM_CHIPS];
	uint8_t bytes[READ_WRITE_CHUNK_SIZE_BYTES];
} writeChunks, readChunks;

// Private functions
static void SIMMProgrammer_HandleWaitingForCommandByte(uint8_t byte);
static void SIMMProgrammer_HandleReadingChipsByte(uint8_t byte);
static void SIMMProgrammer_HandleReadingChipsReadLengthByte(uint8_t byte);
static void SIMMProgrammer_SendReadDataChunk(void);
static void SIMMProgrammer_HandleWritingChipsByte(uint8_t byte);
static void SIMMProgrammer_ElectricalTest_Fail_Handler(uint8_t index1, uint8_t index2);
static void SIMMProgrammer_HandleErasePortionReadPosLengthByte(uint8_t byte);
static void SIMMProgrammer_HandleReadingChipsReadStartPosByte(uint8_t byte);
static void SIMMProgrammer_HandleWritingChipsReadingStartPosByte(uint8_t byte);
static void SIMMProgrammer_HandleReadingChipsMaskByte(uint8_t byte);

/** Initializes the SIMM programmer and prepares it for USB communication.
 *
 */
void SIMMProgrammer_Init(void)
{
	USBCDC_Init();
}

/** Allows the SIMM programmer to do its thing. Main loop handler.
 *
 * Call this function during every main loop iteration.
 */
void SIMMProgrammer_Check(void)
{
	// Read as many bytes as we can and process them
	int16_t result;
	while ((result = USBCDC_ReadByte()) >= 0)
	{
		uint8_t recvByte = (uint8_t)result;

		// Hand it off to the correct handler function based on the current state
		switch (curCommandState)
		{
		case WaitingForCommand:
			SIMMProgrammer_HandleWaitingForCommandByte(recvByte);
			break;
		case ReadingChipsReadLength:
			SIMMProgrammer_HandleReadingChipsReadLengthByte(recvByte);
			break;
		case ReadingChips:
			SIMMProgrammer_HandleReadingChipsByte(recvByte);
			break;
		case WritingChips:
			SIMMProgrammer_HandleWritingChipsByte(recvByte);
			break;
		case ErasePortionReadingPosLength:
			SIMMProgrammer_HandleErasePortionReadPosLengthByte(recvByte);
			break;
		case ReadingChipsReadStartPos:
			SIMMProgrammer_HandleReadingChipsReadStartPosByte(recvByte);
			break;
		case WritingChipsReadingStartPos:
			SIMMProgrammer_HandleWritingChipsReadingStartPosByte(recvByte);
			break;
		case ReadingChipsMask:
			SIMMProgrammer_HandleReadingChipsMaskByte(recvByte);
			break;
		}
	}

	// And do any periodic USB CDC tasks
	USBCDC_Check();
}

/** Handles a received byte when we are waiting for a command
 *
 * @param byte The received byte
 */
static void SIMMProgrammer_HandleWaitingForCommandByte(uint8_t byte)
{
	switch (byte)
	{
	// Asked to enter waiting mode -- we're already there, so say OK.
	case EnterWaitingMode:
		USBCDC_SendByte(CommandReplyOK);
		curCommandState = WaitingForCommand;
		break;
	// Asked to do the electrical test. Reply OK, and then do the test,
	// sending whatever replies necessary
	case DoElectricalTest:
		USBCDC_SendByte(CommandReplyOK);
		// Flush out the initial "OK" reply immediately in this case so the
		// caller gets immediate feedback that the test has started
		USBCDC_Flush();
		SIMMElectricalTest_Run(SIMMProgrammer_ElectricalTest_Fail_Handler);
		USBCDC_SendByte(ProgrammerElectricalTestDone);
		curCommandState = WaitingForCommand;
		break;
	// Asked to identify the chips in the SIMM. Identify them and send reply.
	case IdentifyChips:
	{
		struct ParallelFlashChipID chips[PARALLEL_FLASH_NUM_CHIPS];
		USBCDC_SendByte(CommandReplyOK);
		ParallelFlash_IdentifyChips(chips);
		for (int i = 0; i < PARALLEL_FLASH_NUM_CHIPS; i++)
		{
			USBCDC_SendByte(chips[i].manufacturer);
			USBCDC_SendByte(chips[i].device);
		}
		USBCDC_SendByte(ProgrammerIdentifyDone);
		break;
	}
	// Asked to read a single byte from each SIMM. Change the state and reply.
	case ReadByte:
		USBCDC_SendByte(CommandReplyInvalid); // not implemented yet
		break;
	// Asked to read all four chips. Set the state, reply with the first chunk.
	// This will read from the BEGINNING of the SIMM every time. Use
	// ReadChipsAt to specify a start position
	case ReadChips:
		curCommandState = ReadingChipsReadLength;
		curReadIndex = 0;
		readLengthByteIndex = 0;
		readLength = 0;
		USBCDC_SendByte(CommandReplyOK);
		break;
	case ReadChipsAt:
		curCommandState = ReadingChipsReadStartPos;
		curReadIndex = 0;
		readLengthByteIndex = 0;
		readLength = 0;
		USBCDC_SendByte(CommandReplyOK);
		break;
	// Erase the chips and reply OK. (TODO: Sometimes erase might fail)
	case EraseChips:
		ParallelFlash_EraseChips(chipsMask);
		USBCDC_SendByte(CommandReplyOK);
		break;
	// Begin writing the chips. Change the state, reply, wait for chunk of data
	case WriteChips:
		curCommandState = WritingChips;
		curWriteIndex = 0;
		writePosInChunk = -1;
		USBCDC_SendByte(CommandReplyOK);
		break;
	case WriteChipsAt:
		curCommandState = WritingChipsReadingStartPos;
		curWriteIndex = 0;
		readLengthByteIndex = 0;
		writePosInChunk = -1;
		USBCDC_SendByte(CommandReplyOK);
		break;
	// Asked for the current bootloader state. We are in the program right now,
	// so reply accordingly.
	case GetBootloaderState:
		USBCDC_SendByte(CommandReplyOK);
		USBCDC_SendByte(BootloaderStateInProgrammer);
		break;
	// Enter the bootloader. Wait a bit, then jump to the bootloader location.
	case EnterBootloader:
		USBCDC_SendByte(CommandReplyOK);
		// Force this to be sent immediately so the programmer software knows.
		USBCDC_Flush();
		// Jump into the bootloader
		Hardware_EnterBootloader();
		break;
	// Enter the programmer. We're already there, so reply OK.
	case EnterProgrammer:
		// Already in the programmer
		USBCDC_SendByte(CommandReplyOK);
		break;
	// Set the SIMM type to the older, smaller chip size (2MB and below)
	case SetSIMMTypePLCC32_2MB:
		ParallelFlash_SetChipType(ParallelFlash_SST39SF040_x4);
		USBCDC_SendByte(CommandReplyOK);
		break;
	case SetSIMMTypeLarger:
		ParallelFlash_SetChipType(ParallelFlash_M29F160FB5AN6E2_x4);
		USBCDC_SendByte(CommandReplyOK);
		break;
	case SetVerifyWhileWriting:
		verifyDuringWrite = true;
		USBCDC_SendByte(CommandReplyOK);
		break;
	case SetNoVerifyWhileWriting:
		verifyDuringWrite = false;
		USBCDC_SendByte(CommandReplyOK);
		break;
	case ErasePortion:
		readLengthByteIndex = 0;
		eraseLength = 0;
		erasePosition = 0;
		curCommandState = ErasePortionReadingPosLength;
		USBCDC_SendByte(CommandReplyOK);
		break;
	case SetChipsMask:
		curCommandState = ReadingChipsMask;
		USBCDC_SendByte(CommandReplyOK);
		break;
	// We don't know what this command is, so reply that it was invalid.
	default:
		USBCDC_SendByte(CommandReplyInvalid);
		break;
	}
}

/** Handles a received byte when we are reading from chips
 *
 * @param byte The received byte
 */
static void SIMMProgrammer_HandleReadingChipsByte(uint8_t byte)
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
			USBCDC_SendByte(ProgrammerReadFinished);
			curCommandState = WaitingForCommand;
		}
		else // There's more data left to read, so read it and send it to them!
		{
			LED_Toggle();
			USBCDC_SendByte(ProgrammerReadMoreData);
			SIMMProgrammer_SendReadDataChunk();
		}
		break;
	case ComputerReadCancel:
		// If they've canceled, let them know we got their request and go back
		// to "waiting for command" state
		USBCDC_SendByte(ProgrammerReadConfirmCancel);
		curCommandState = WaitingForCommand;
		break;
	}
}

/** Handles a received byte when we are reading the length of data requested to be read
 *
 * @param byte The received byte
 */
static void SIMMProgrammer_HandleReadingChipsReadLengthByte(uint8_t byte)
{
	// There will be four bytes, so count up until we know the length. If they
	// have sent all four bytes, send the first read chunk.
	readLength |= (((uint32_t)byte) << (8*readLengthByteIndex));
	if (++readLengthByteIndex >= 4)
	{
		// Ensure it's within limits and a multiple of 1024
		if ((curReadIndex + readLength > PARALLEL_FLASH_NUM_CHIPS * MAX_CHIP_SIZE) ||
			(readLength % READ_WRITE_CHUNK_SIZE_BYTES) ||
			(curReadIndex % READ_WRITE_CHUNK_SIZE_BYTES) ||
			(readLength == 0))// Ensure it's within limits and a multiple of 1024
		{
			USBCDC_SendByte(ProgrammerReadError);
			curCommandState = WaitingForCommand;
		}
		else
		{
			// Convert the length/pos into the number of chunks we need to send
			readLength /= READ_WRITE_CHUNK_SIZE_BYTES;
			curReadIndex /= READ_WRITE_CHUNK_SIZE_BYTES;
			curCommandState = ReadingChips;
			USBCDC_SendByte(ProgrammerReadOK);
			SIMMProgrammer_SendReadDataChunk();
		}
	}
}

/** Reads a chunk of data from the SIMM and sends it over the USB CDC serial port.
 *
 */
static void SIMMProgrammer_SendReadDataChunk(void)
{
	// Read the next chunk of data, send it over USB, and make sure
	// we sent it correctly.
	ParallelFlash_Read(curReadIndex * (READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS),
			readChunks.words, READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS);
	bool retVal = USBCDC_SendData(readChunks.bytes, READ_WRITE_CHUNK_SIZE_BYTES);

	// If for some reason there was an error, mark it as such. Otherwise,
	// increment our pointer so we know the next chunk of data to send.
	if (!retVal)
	{
		//curCommandState = ReadingChipsUnableSendError; // TODO: not implemented
		curCommandState = WaitingForCommand;
	}
	else
	{
		curReadIndex++;
	}
}

/** Handles a received byte when we are in the "writing chips" state
 *
 * @param byte The received byte
 */
static void SIMMProgrammer_HandleWritingChipsByte(uint8_t byte)
{
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
			if (curWriteIndex < MAX_CHIP_SIZE / (READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS))
			{
				USBCDC_SendByte(ProgrammerWriteOK);
			}
			else
			{
				LED_Off();
				USBCDC_SendByte(ProgrammerWriteError);
				curCommandState = WaitingForCommand;
			}
			break;
		// The computer said that it's done writing.
		case ComputerWriteFinish:
			LED_Off();
			USBCDC_SendByte(ProgrammerWriteOK);
			curCommandState = WaitingForCommand;
			break;
		// The computer asked to cancel.
		case ComputerWriteCancel:
			LED_Off();
			USBCDC_SendByte(ProgrammerWriteConfirmCancel);
			curCommandState = WaitingForCommand;
			break;
		}
	}
	else // Interpret the incoming byte as data to write to the SIMM.
	{
		// Save the byte. Then, block until we receive the rest of the data.
		writeChunks.bytes[writePosInChunk++] = byte;
		while (writePosInChunk < READ_WRITE_CHUNK_SIZE_BYTES)
		{
			writeChunks.bytes[writePosInChunk++] = USBCDC_ReadByteBlocking();
		}

		// We filled up the chunk, write it out and confirm it, then wait
		// for the next command from the computer!
		if (chipsMask == ALL_CHIPS)
		{
			ParallelFlash_WriteAllChips(curWriteIndex * (READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS),
										writeChunks.words, READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS);
		}
		else
		{
			ParallelFlash_WriteSomeChips(curWriteIndex * (READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS),
										 writeChunks.words, READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS, chipsMask);
		}

		// Verify if we were asked to.
		uint8_t badVerifyChipsMask = 0;
		if (verifyDuringWrite)
		{
			// Read back a chunk
			ParallelFlash_Read(curWriteIndex * (READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS),
							   readChunks.words, READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS);

			// Compare the readback to what we attempted to flash.
			// Look at each chip
			for (uint8_t chip = 0; chip < PARALLEL_FLASH_NUM_CHIPS; chip++)
			{
				uint16_t bytePos = chip;
				uint8_t thisChipMask = 1 << chip;
				// Loop over all bytes that are on this chip
				for (uint16_t i = 0; i < READ_WRITE_CHUNK_SIZE_BYTES/PARALLEL_FLASH_NUM_CHIPS; i++)
				{
					if (writeChunks.bytes[bytePos] != readChunks.bytes[bytePos])
					{
						badVerifyChipsMask |= thisChipMask;
					}
					bytePos += PARALLEL_FLASH_NUM_CHIPS;
				}
			}

			// Filter out chips we didn't care about
			badVerifyChipsMask &= chipsMask;
		}

		// Bail if verification failed
		if (badVerifyChipsMask != 0)
		{
			// Verification failed. The mask we calculated is actually
			// backwards. We need to reverse it when we transmit the IC
			// status back to the programmer software. This is kind of silly
			// but it's too late to update the protocol.
			uint8_t actualBadMask = 0;
			for (uint8_t i = 0; i < PARALLEL_FLASH_NUM_CHIPS; i++)
			{
				if (badVerifyChipsMask & (1 << i))
				{
					actualBadMask |= 0x80;
				}
				actualBadMask >>= 1;
			}

			// Uh oh -- verification failure.
			LED_Off();
			// Send the fail bit along with a mask of failed chips.
			USBCDC_SendByte(ProgrammerWriteVerificationError | badVerifyChipsMask);
			curCommandState = WaitingForCommand;
		}
		else
		{
			USBCDC_SendByte(ProgrammerWriteOK);
			curWriteIndex++;
			writePosInChunk = -1;
			LED_Toggle();
		}
	}
}

/** Handler called during an electrical test when a short is detected
 *
 * @param index1 The index of the first shorted pin
 * @param index2 The index of the second shorted pin
 *
 * The two pins at index1 and index2 have been detected as shorted together.
 * The numbering is internal to the SIMM electrical test, and the programmer
 * software knows how to interpret it.
 */
static void SIMMProgrammer_ElectricalTest_Fail_Handler(uint8_t index1, uint8_t index2)
{
	USBCDC_SendByte(ProgrammerElectricalTestFail);
	USBCDC_SendByte(index1);
	USBCDC_SendByte(index2);
}

/** Handles a received byte when we are determining what part of the chip to erase
 *
 * @param byte The received byte
 */
static void SIMMProgrammer_HandleErasePortionReadPosLengthByte(uint8_t byte)
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
		ParallelFlashChipType chipType = ParallelFlash_ChipType();
		bool eraseSuccess = false;

		// Ensure they are both within limits of sector size erasure
		if (((erasePosition % ERASE_SECTOR_SIZE_BYTES) == 0) &&
			((eraseLength % ERASE_SECTOR_SIZE_BYTES) == 0))
		{
			uint32_t boundary = eraseLength + erasePosition;

			// Ensure they are within the limits of the chip size too
			if (chipType == ParallelFlash_SST39SF040_x4)
			{
				if (boundary <= (2 * 1024UL * 1024UL))
				{
					// OK! We're erasing certain sectors of a 2 MB SIMM.
					USBCDC_SendByte(ProgrammerErasePortionOK);
					// Send the response immediately, it could take a while.
					USBCDC_Flush();
					if (ParallelFlash_EraseSectors(erasePosition/PARALLEL_FLASH_NUM_CHIPS,
							eraseLength/PARALLEL_FLASH_NUM_CHIPS, chipsMask))
					{
						eraseSuccess = true;
					}
				}
			}
			else if (chipType == ParallelFlash_M29F160FB5AN6E2_x4)
			{
				if (boundary <= (8 * 1024UL * 1024UL))
				{
					// OK! We're erasing certain sectors of an 8 MB SIMM.
					USBCDC_SendByte(ProgrammerErasePortionOK);
					// Send the response immediately, it could take a while.
					USBCDC_Flush();
					if (ParallelFlash_EraseSectors(erasePosition/PARALLEL_FLASH_NUM_CHIPS,
							eraseLength/PARALLEL_FLASH_NUM_CHIPS, chipsMask))
					{
						eraseSuccess = true;
					}
				}
			}
		}

		if (eraseSuccess)
		{
			// Not on a sector boundary for erase position and/or length
			USBCDC_SendByte(ProgrammerErasePortionFinished);
			curCommandState = WaitingForCommand;
		}
		else
		{
			// Not on a sector boundary for erase position and/or length
			USBCDC_SendByte(ProgrammerErasePortionError);
			curCommandState = WaitingForCommand;
		}
	}
}

/** Handles a received byte when we are determining where to start reading from the SIMM
 *
 * @param byte The received byte
 */
static void SIMMProgrammer_HandleReadingChipsReadStartPosByte(uint8_t byte)
{
	// There will be four bytes, so count up until we know the position. If they
	// have sent all four bytes, then start reading the length
	curReadIndex |= (((uint32_t)byte) << (8*readLengthByteIndex));
	if (++readLengthByteIndex >= 4)
	{
		readLengthByteIndex = 0;
		curCommandState = ReadingChipsReadLength;
	}
}

/** Handles a received byte when we are determining where to start writing to the SIMM
 *
 * @param byte The received byte
 */
static void SIMMProgrammer_HandleWritingChipsReadingStartPosByte(uint8_t byte)
{
	// There will be four bytes, so count up until we know the position. If they
	// have sent all four bytes, then confirm the write and begin
	curWriteIndex |= (((uint32_t)byte) << (8*readLengthByteIndex));
	if (++readLengthByteIndex >= 4)
	{
		// Got it...now, is it valid? If so, allow the write to begin
		if ((curWriteIndex % READ_WRITE_CHUNK_SIZE_BYTES) ||
			(curWriteIndex >= PARALLEL_FLASH_NUM_CHIPS * MAX_CHIP_SIZE))
		{
			USBCDC_SendByte(ProgrammerWriteError);
			curCommandState = WaitingForCommand;
		}
		else
		{
			// Convert write size into an index appropriate for rest of code
			curWriteIndex /= READ_WRITE_CHUNK_SIZE_BYTES;
			USBCDC_SendByte(ProgrammerWriteOK);
			curCommandState = WritingChips;
		}
	}
}

/** Handles a received byte when we are determining the mask of which chips to write to
 *
 * @param byte The received byte
 */
static void SIMMProgrammer_HandleReadingChipsMaskByte(uint8_t byte)
{
	// Single byte follows containing mask of chips we're programming
	if (byte <= 0x0F)
	{
		// Mask has to be less than or equal to 0x0F because there are only
		// four valid mask bits.
		chipsMask = byte;
		USBCDC_SendByte(CommandReplyOK);
	}
	else
	{
		USBCDC_SendByte(CommandReplyError);
	}

	// Done either way; now we're waiting for a command to arrive
	curCommandState = WaitingForCommand;
}
