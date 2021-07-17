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
