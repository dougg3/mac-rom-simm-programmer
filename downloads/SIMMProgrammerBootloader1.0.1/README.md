# File:
[SIMMProgrammerBootloader1.0.1.hex](SIMMProgrammerBootloader1.0.1.hex)   10.9 KB

# Abstract
SIMM programmer board bootloader version 1.0.1

# Description:

This is the bootloader for the SIMM programmer board. Unless you have an AVR ISP programmer, you cannot change this program on your board. You shouldn't ever need to change it unless a catastrophic bug is discovered in this bootloader program--all of the SIMM programming operation happens in the user-updateable firmware. This bootloader is provided here for reference, or for anyone who wants to tinker with their board.

Changelist:
1.0.1 -- The LED now blinks as a firmware update is in progress.
1.0.0 -- Initial release

Fuse bit settings for the AT90USB646 on the programmer board:
Low = 0xDF
High = 0xD0
Extended = 0xF8

# SHA1 Checksum:
`f5b27197fce0cbad115315d26a9bcc64849526b1`
