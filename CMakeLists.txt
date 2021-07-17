cmake_minimum_required(VERSION 3.13)
project(SIMMProgrammer)

# Create a list of all source files common to all architectures
set(SOURCES
	drivers/mcp23s17.c
	drivers/parallel_flash.c
	tests/simm_electrical_test.c
	main.c
	simm_programmer.c
)

# Get hardware-specific source files
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "avr")
	include(hal/at90usb646/at90usb646_sources.cmake)
else()
	include(hal/pc/pc_sources.cmake)
endif()

# The actual executable, in ELF format
add_executable(SIMMProgrammer.elf ${SOURCES} ${HWSOURCES})

# Common compiler options
target_compile_options(SIMMProgrammer.elf PRIVATE
	-Wall -Os -std=gnu99 -ffunction-sections -fdata-sections
)

# Common linker options
target_link_options(SIMMProgrammer.elf PRIVATE
	-Wl,-Map,SIMMProgrammer.map -Wl,--gc-sections
)

# Get hardware-specific options
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "avr")
	include(hal/at90usb646/at90usb646_options.cmake) 
else()
	include(hal/pc/pc_options.cmake)
endif()
