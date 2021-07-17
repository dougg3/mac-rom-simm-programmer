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

# Create a list of all source files specific to the AVR
set(AVRSOURCES
	hal/at90usb646/LUFA/Drivers/USB/Class/Device/CDCClassDevice.c
	hal/at90usb646/LUFA/Drivers/USB/Core/AVR8/Device_AVR8.c
	hal/at90usb646/LUFA/Drivers/USB/Core/AVR8/Endpoint_AVR8.c
	hal/at90usb646/LUFA/Drivers/USB/Core/AVR8/EndpointStream_AVR8.c
	hal/at90usb646/LUFA/Drivers/USB/Core/AVR8/Host_AVR8.c
	hal/at90usb646/LUFA/Drivers/USB/Core/AVR8/Pipe_AVR8.c
	hal/at90usb646/LUFA/Drivers/USB/Core/AVR8/PipeStream_AVR8.c
	hal/at90usb646/LUFA/Drivers/USB/Core/AVR8/USBController_AVR8.c
	hal/at90usb646/LUFA/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.c
	hal/at90usb646/LUFA/Drivers/USB/Core/ConfigDescriptor.c
	hal/at90usb646/LUFA/Drivers/USB/Core/DeviceStandardReq.c
	hal/at90usb646/LUFA/Drivers/USB/Core/Events.c
	hal/at90usb646/LUFA/Drivers/USB/Core/USBTask.c
	hal/at90usb646/board.c
	hal/at90usb646/cdc_device_definition.c
	hal/at90usb646/Descriptors.c
	hal/at90usb646/gpio.c
	hal/at90usb646/parallel_bus.c
	hal/at90usb646/spi.c
	hal/at90usb646/usbcdc.c
)

add_executable(SIMMProgrammer.elf ${SOURCES} ${AVRSOURCES})

# Common compiler options
target_compile_options(SIMMProgrammer.elf PRIVATE
	-Wall -Os -std=gnu99 -ffunction-sections -fdata-sections
)

# Common linker options
target_link_options(SIMMProgrammer.elf PRIVATE
	-Wl,-Map,SIMMProgrammer.map -Wl,--gc-sections
)

# AVR-specific include paths
target_include_directories(SIMMProgrammer.elf PRIVATE
	hal/at90usb646
)

# AVR-specific compiler definitions
target_compile_definitions(SIMMProgrammer.elf PRIVATE
	F_CPU=16000000UL
	F_USB=16000000UL
	USE_LUFA_CONFIG_HEADER
)

# AVR-specific compiler options
target_compile_options(SIMMProgrammer.elf PRIVATE
	-fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -mmcu=at90usb646
)

# AVR-specific linker options
target_link_options(SIMMProgrammer.elf PRIVATE
	-mmcu=at90usb646
)

# AVR-specific command/target to generate .bin file from the ELF file. This program
# is flashed using a bootloader, so there's no need to generate a HEX file.
add_custom_command(OUTPUT SIMMProgrammer.bin
	COMMAND ${CMAKE_OBJCOPY} -R .eeprom -O binary SIMMProgrammer.elf SIMMProgrammer.bin
	DEPENDS SIMMProgrammer.elf
)
add_custom_target(SIMMProgrammer_bin ALL DEPENDS SIMMProgrammer.bin)
