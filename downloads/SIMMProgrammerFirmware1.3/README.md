# File:
[SIMMProgrammerFirmware1.3.bin](SIMMProgrammerFirmware1.3.bin)   11.2 KB

# Abstract
SIMM Programmer board firmware version 1.3

# Description:
This is a new firmware that has been completely reorganized under the hood. The only visible change to users is that it is much faster than the previous version 1.2. Programming an 8 MB SIMM now only takes about 2:28. Previously it took 8:42.

If your programmer board's firmware somehow becomes corrupted, you can fix it by uploading this file using the SIMMProgrammer software by going to the Advanced menu and selecting "Update firmware...".

Version history:

- 1.3: Massive speedup and under-the-hood reorganization
- 1.2: Adds support for only programming/erasing a portion of the SIMM. Also adds support for programming and reading individual chips on the SIMM.
- 1.1.1: Adds support for verifying while writing
- 1.1: Adds support for 8 MB ROM SIMM
- 1.0.1: Adds LED blinking during read and write operations
- 1.0: Initial release

# SHA1 Checksum:
`292c9961397eefa7d713a51982a43fe4`
