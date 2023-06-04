cmake_minimum_required(VERSION 3.13)
project(SIMMProgrammer)

# Create a list of all source files common to all architectures
set(SOURCES
	drivers/mcp23s17.c
	drivers/mcp23s17.h
	drivers/parallel_flash.c
	drivers/parallel_flash.h
	hal/board.h
	hal/gpio.h
	hal/parallel_bus.h
	hal/spi.h
	hal/usbcdc.h
	tests/simm_electrical_test.c
	tests/simm_electrical_test.h
	chip_id.h
	main.c
	led.h
	programmer_protocol.h
	simm_programmer.c
	simm_programmer.h
	util.h
)

# Get hardware-specific source files
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "avr")
	include(hal/at90usb646/at90usb646_sources.cmake)
else()
	message(FATAL_ERROR "unrecognized architecture for build")
endif()

# The actual executable, in ELF format
add_executable(SIMMProgrammer.elf ${SOURCES} ${HWSOURCES})

# Common compiler options
target_compile_options(SIMMProgrammer.elf PRIVATE
	-Wall -Os -ffunction-sections -fdata-sections
)
set_property(TARGET SIMMProgrammer.elf PROPERTY C_STANDARD 99)

# Common linker options
target_link_options(SIMMProgrammer.elf PRIVATE
	-Wl,-Map,SIMMProgrammer.map -Wl,--gc-sections
)

# Get hardware-specific options
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "avr")
	include(hal/at90usb646/at90usb646_options.cmake)
endif()
