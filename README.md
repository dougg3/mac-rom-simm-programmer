# Description

This project is a bootloader and firmware for a Macintosh ROM SIMM programmer, along with control software for Mac OS X, Windows, and Linux. The ROM SIMM is compatible with the SE/30, all II-series Macs with a 64-pin SIMM socket (should include the IIx, IIcx, IIci, IIfx, and IIsi), and the Quadra 700.

This particular repository contains the main firmware that runs on the programmer board. The current compiler version used with this project is avr-gcc 4.8.2. Using a different version of gcc may result in worse performance due to some very tight optimization performed on this project to decrease programming time.

# Downloads

Binary downloads (originally [from Google Code](https://code.google.com/p/mac-rom-simm-programmer/downloads/list)) can be found at https://github.com/dougg3/mac-rom-simm-programmer/tree/downloads/downloads

# Repositories

The project is spread over a few repositories. Some of them have a wiki.

| Repository                                | Repository location                                              | Wiki location  |
| ------------------------------------------------------ | ---------------------------------------------------------------- | -------------- |
| Programmer Firmware (AVR microcontroller)              | https://github.com/dougg3/mac-rom-simm-programmer                | https://github.com/dougg3/mac-rom-simm-programmer/wiki |
| Bootloader (AVR microcontroller)                       | https://github.com/dougg3/mac-rom-simm-programmer.bootloader
| Programmer Software (Windows/Mac/Linux)                | https://github.com/dougg3/mac-rom-simm-programmer.software       | none |
| Windows Driver (.inf file, not needed on Windows 10)   | https://github.com/dougg3/mac-rom-simm-programmer.windriver      | none |
| Custom QextSerialPort for Programmer Software          | https://github.com/dougg3/doug-qextserialport-linuxnotifications | none |
| QextSerialPort base                                    | https://github.com/qextserialport/qextserialport                 | https://github.com/qextserialport/qextserialport/blob/wiki/Welcome.md |
| CAD for programmer, along with 2 MB and 8 MB SIMM PCBs | https://github.com/dougg3/mac-rom-simm-programmer.cad            | none |                    
| Mac ROM patcher                                        | https://github.com/jpluimers/macrompatcher/ (from https://code.google.com/p/macrompatcher) | none |

# Firmware compilation instructions

As mentioned earlier, this is an AVR project that is currently optimized for avr-gcc 4.8.2. It can be built using either CMake or Eclipse with the [AVR Eclipse plugin](https://avr-eclipse.sourceforge.net/wiki/index.php/The_AVR_Eclipse_Plugin). To build with CMake:

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-avr.cmake ..
make
```

This will result in a generated SIMMProgrammer.bin file which can be programmed to the board using the [Windows/Mac/Linux software](https://github.com/dougg3/mac-rom-simm-programmer.software).

# Videos

## ROM SIMM

| What | Where |
| ---- | ----- |
| IIci booting from ROM disk | https://www.youtube.com/watch?v=SEFcQRmYtBI |
| Mac IIci Modified (Slower) Startup Chime | https://www.youtube.com/watch?v=lyIIRtR3Aw0 |
| Playing with IIci ROM - other death chimes | https://www.youtube.com/watch?v=mlmt1AealLo |
| Mac IIci with a newer Mac's startup chime | https://www.youtube.com/watch?v=bRJtMMYCf0E |
| Mac IIci with Super Mario Bros startup chime! | https://www.youtube.com/watch?v=omL7mx0zxvI |
| Mac IIci Mario Startup Chime Part 2 -- Long! | https://www.youtube.com/watch?v=Yen0omvBo2Y |
| Mac IIci - Another Custom Startup Chime | https://www.youtube.com/watch?v=1R4W3mApAio |

## Regular ROM

Mac IIci ROM hack (custom startup icons): https://www.youtube.com/watch?v=LALaYy7ZLy0

# Related articles

## [Blog](http://www.downtowndougbrown.com/programmable-mac-rom-simms/) posts

- [Mac ROM SIMMs](http://www.downtowndougbrown.com/programmable-mac-rom-simms/) where you can order them too.
- [Soldering using solder paste, a dispenser, and a toaster oven](http://www.downtowndougbrown.com/2014/04/soldering-using-solder-paste-a-dispenser-and-a-toaster-oven/)
- [linux `udev` rules](http://www.downtowndougbrown.com/2014/03/linux-udev-rules/)
- [8 MB Mac ROM SIMM](http://www.downtowndougbrown.com/2013/01/8-mb-mac-rom-simm/)
- [Mac ROM SIMM programmer](http://www.downtowndougbrown.com/2012/08/mac-rom-simm-programmer/)
- [Review of Seeed Studio Fusion PCB service](http://www.downtowndougbrown.com/2011/10/review-of-seeed-studio-fusion-pcb-service/)
- [Mac IIci custom startup chime, part II](http://www.downtowndougbrown.com/2011/08/mac-iici-custom-startup-chime-part-ii/)
- [Mac IIci custom startup sound ROM hack](http://www.downtowndougbrown.com/2011/08/mac-iici-custom-startup-sound-rom-hack/)

## Others

- [Capturing a Mac ROM Image](http://www.emaculation.com/doku.php/capturing_rom)
- [HxD - Freeware Hex Editor and Disk Editor](http://mh-nexus.de/en/hxd/)
- [Mac ROM Checksum Verifier](http://www.d.umn.edu/~bold0070/projects/checksum/)
- [68k Macintosh Liberation Army](https://68kmla.org/forums/)
  - [Another IIci ROM hack](https://68kmla.org/forums/index.php?/topic/15436-another-iici-rom-hack/)
- [68k.hax.com](http://68k.hax.com/)
- [(Enhanced) Apple Sound Chip EASC/ASC programming](http://web.archive.org/web/20131004115313/http://mamedev.org/source/src/emu/sound/asc.c.html)
- [68k Mac ROM Boot Disk](http://synack.net/~bbraun/macromboot.html)
- [bbraun hacks](http://synack.net/~bbraun/)
  - [Mac Related 3D Models](http://synack.net/~bbraun/mac3d/) including ROM SIMM Programmer Case
  - [AppleTalk over IP](http://synack.net/~bbraun/avpn.html)

# Non-related articles

- [Microcontroller lessons](http://www.downtowndougbrown.com/microcontroller-lessons/)
