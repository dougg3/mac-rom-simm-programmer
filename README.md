# Description

This project is a bootloader and firmware for a Macintosh ROM SIMM programmer, along with control software for Mac OS X, Windows, and Linux. The ROM SIMM is compatible with the SE/30, all II-series Macs with a 64-pin SIMM socket (should include the IIx, IIcx, IIci, IIfx, and IIsi), and the Quadra 700.

This particular repository contains the main firmware that runs on the programmer board. There are two variants of the firmware that are built from this source code:

1. The firmware for [my original programmer](https://www.downtowndougbrown.com/2012/08/mac-rom-simm-programmer/), the [Big Mess o' Wires programmer](http://www.bigmessowires.com/mac-rom-inator-ii-programming/), and the [CayMac Vintage revision 1 programmer](https://ko-fi.com/s/6f9e9644e4). These programmers use the Atmel/Microchip AT90USB646/1286 AVR microcontroller. The current compiler version used with this version of the firmware is avr-gcc 4.8.2. Using a different version of gcc may result in worse performance due to some very tight optimizations performed on this project to decrease programming time.
2. The firmware for the [CayMac Vintage ROMmate-2](https://ko-fi.com/s/d6e7e4494d), which uses the Nuvoton M258KE3AE ARM Cortex-M23 microcontroller. The compiler that has been tested with this firmware is [ARM GCC 6-2017-q1-update](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/6-2017-q1-update).

# Downloads

Binary downloads can be found at the following links:

- [Firmware](https://github.com/dougg3/mac-rom-simm-programmer/releases)
- [Control Software](https://github.com/dougg3/mac-rom-simm-programmer.software/releases)
- [Bootloader](https://github.com/dougg3/mac-rom-simm-programmer.bootloader/releases)

# Repositories

The project is spread over a few repositories. Some of them have a wiki.

| Repository                                | Repository location                                              | Wiki location  |
| ------------------------------------------------------ | ---------------------------------------------------------------- | -------------- |
| Programmer Firmware (AVR microcontroller)              | https://github.com/dougg3/mac-rom-simm-programmer                | https://github.com/dougg3/mac-rom-simm-programmer/wiki |
| Bootloader (AVR microcontroller)                       | https://github.com/dougg3/mac-rom-simm-programmer.bootloader
| Programmer Software (Windows/Mac/Linux)                | https://github.com/dougg3/mac-rom-simm-programmer.software       | none |
| Windows Driver (.inf file, not needed on Windows 10)   | https://github.com/dougg3/mac-rom-simm-programmer.windriver      | none |
| CAD for programmer, along with 2 MB and 8 MB SIMM PCBs | https://github.com/dougg3/mac-rom-simm-programmer.cad            | none |                    
| Mac ROM patcher                                        | https://github.com/jpluimers/macrompatcher/ (from https://code.google.com/p/macrompatcher) | none |

# Firmware compilation instructions

## AT90USB646/AT90USB1286

This firmware is used on my original programmer, the BMOW programmer, and CayMac's original programmer.

As mentioned earlier, this is an AVR project that is currently optimized for avr-gcc 4.8.2. It can be built using either CMake or Eclipse with the [AVR Eclipse plugin](https://avr-eclipse.sourceforge.net/wiki/index.php/The_AVR_Eclipse_Plugin). To build with CMake, make sure avr-gcc is in your path, and then run:

```
mkdir build_avr
cd build_avr
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-avr.cmake ..
make
```

## M258KE3AE

This firmware is used on the CayMac ROMmate-2.

Tested with [ARM GCC 6-2017-q1-update](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/6-2017-q1-update). To build with CMake, make sure arm-none-eabi-gcc is in your path, and then run:

```
mkdir build_arm
cd build_arm
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-m258ke.cmake ..
make
```

## Common information

The build processes described above will create a SIMMProgrammer.bin file that can be programmed to the board using the [Windows/Mac/Linux software](https://github.com/dougg3/mac-rom-simm-programmer.software). You can also generate a combined firmware image containing both the AVR and ARM builds that automatically flashes the correct firmware based on the detected board when using software version 2.0 or newer:

```
cat build_avr/SIMMProgrammer.bin \
    <(echo -en "\xDB\x00\xDB\x01\xDB\x02\xDB\x03\xDB\x04\xDB\x05\xDB\x06\xDB\x07") \
    <(echo -en "\xDB\x08\xDB\x09\xDB\x0A\xDB\x0B\xDB\x0C\xDB\x0D\xDB\x0E\xDB\x0F") \
    <(echo -en "\xDB\xDB\xDB\xDB\xAA\xAA\xAA\xAA\xDB\xDB\xDB\xDB\x55\x55\x55\x55") \
    build_arm/SIMMProgrammer.bin \
    > SIMMProgrammerFirmware.bin
```

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
