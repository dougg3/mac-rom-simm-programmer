/*
 * programmer_protocol.h
 *
 *  Created on: Dec 26, 2011
 *      Author: Doug
 */

#ifndef PROGRAMMER_PROTOCOL_H_
#define PROGRAMMER_PROTOCOL_H_

// When the programmer is in "waiting for command" mode,
// you send it one of the bytes below to do something (or begin to do something)
typedef enum ProgrammerCommand
{
	EnterWaitingMode = 0,
	DoElectricalTest,
	IdentifyChips,
	ReadByte,
	ReadChips,
	EraseChips,
	WriteChips,
	GetBootloaderState,
	EnterBootloader,
	EnterProgrammer,
	BootloaderEraseAndWriteProgram
} ProgrammerCommand;

// After a command is sent, the programmer will always respond with
// one of the replies below. (Depending on the command, it may reply with other
// stuff after that, too)
typedef enum ProgrammerReply
{
	CommandReplyOK = 0,
	CommandReplyError,
	CommandReplyInvalid
} ProgrammerReply;

// -------------------------  ELECTRICAL TEST PROTOCOL  -------------------------
// After CommandReplyOK, any failures are reported by the programmer.
// It will send ProgrammerElectricalTestFail followed by two more bytes -- each
// representing an index of the failure location. Any more electrical test failures
// will result in another 3-byte sequence as described above.
// When it's totally finished, it will send ProgrammerElectricalTestDone.
// So a pass would be indicated by no ProgrammerElectricalTestFail sequences being
// sent.
typedef enum ProgrammerElectricalTestReply
{
	ProgrammerElectricalTestFail = 0,
	ProgrammerElectricalTestDone
} ProgrammerElectricalTestReply;

// -------------------------  IDENTIFY CHIPS PROTOCOL  -------------------------
// After CommandReplyOK, it will send the manufacturer ID and device ID of each
// of the 4 chips:
// M1, D1, M2, D2, M3, D3, M4, D4
// followed by ProgrammerIdentifyDone.
typedef enum ProgrammerIdentifyReply
{
	ProgrammerIdentifyDone = 0
} ProgrammerIdentifyReply;

// -------------------------  READ PROTOCOL  -------------------------
// After CommandReplyOK, the programmer will send a chunk of data.
// The computer will send a reply (see the enum below this one)
// The programmer will then reply to *that* reply (see this enum)
typedef enum ProgrammerReadReply
{
	ProgrammerReadOK = 0,
	ProgrammerReadError,
	ProgrammerReadMoreData,
	ProgrammerReadFinished,
	ProgrammerReadConfirmCancel
} ProgrammerReadReply;

// When the computer is confirming reception of a block of data from the device,
// it should reply with either OK that it received the data OK, or cancel
// to tell the device that the user canceled the read.
typedef enum ComputerReadReply
{
	ComputerReadOK = 0,
	ComputerReadCancel
} ComputerReadReply;

// -------------------------  ERASE PROTOCOL  -------------------------
// There is none -- a reply of CommandReplyOK will indicate that the erase
// completed successfully.

// -------------------------  WRITE PROTOCOL  -------------------------
// After CommandReplyOK, the computer should send one of the commands below.
// The programmer will reply with one of the replies seen in the enum below
// this one, and then the computer can send a 1024-byte chunk of data.
// The programmer will reply with ProgrammerWriteOK, and then the cycle can
// continue (the computer sends another request in this enum)
typedef enum ComputerWriteRequest
{
	ComputerWriteMore = 0,
	ComputerWriteFinish,
	ComputerWriteCancel
} ComputerWriteRequest;

typedef enum ProgrammerWriteReply
{
	ProgrammerWriteOK = 0,
	ProgrammerWriteError,
	ProgrammerWriteConfirmCancel
} ProgrammerWriteReply;

// -------------------------  BOOTLOADER STATE PROTOCOL  -------------------------
// If the command is GetBootloaderState, it will reply with CommandReplyOK followed
// by one of the two replies below to tell the control program which mode
// the device is currently in.
typedef enum BootloaderStateReply
{
	BootloaderStateInBootloader = 0,
	BootloaderStateInProgrammer
} BootloaderStateReply;

// -------------------------  BOOTLOADER ERASE/WRITE PROTOCOL  -------------------------
// If the command is BootloaderEraseAndWriteProgram, it will reply with CommandReplyOK
// followed by either BootloaderEraseOK or BootloaderEraseError. At this point
// the program can ask to write more data or finish or cancel, and then the appropriate
// reply will be sent back to it. Works very similar to the write protocol.
typedef enum ProgrammerBootloaderEraseWriteReply
{
	BootloaderWriteOK,
	BootloaderWriteError,
	BootloaderWriteConfirmCancel
} ProgrammerBootloaderEraseWriteReply;

typedef enum ComputerBootloaderEraseWriteRequest
{
	ComputerBootloaderWriteMore = 0,
	ComputerBootloaderFinish,
	ComputerBootloaderCancel
} ComputerBootloaderEraseWriteRequest;

#endif /* PROGRAMMER_PROTOCOL_H_ */
