# M258KE-specific include paths
target_include_directories(SIMMProgrammer.elf PRIVATE
	hal/m258ke
)

# M258KE-specific compiler definitions
target_compile_definitions(SIMMProgrammer.elf PRIVATE
)

# M258KE-specific compiler options
target_compile_options(SIMMProgrammer.elf PRIVATE
	-mcpu=cortex-m23 -march=armv8-m.base -mthumb
)

# M258KE-specific linker options
target_link_options(SIMMProgrammer.elf PRIVATE
	-mcpu=cortex-m23 -march=armv8-m.base -mthumb
	-T ${CMAKE_SOURCE_DIR}/hal/m258ke/nuvoton/gcc_arm.ld
	--specs=nano.specs
)

# M258KE-specific command/target to generate .bin file from the ELF file. This program
# is flashed using a bootloader, so there's no need to generate a HEX file.
add_custom_command(OUTPUT SIMMProgrammer.bin
	COMMAND ${CMAKE_OBJCOPY} -O binary SIMMProgrammer.elf SIMMProgrammer.bin
	DEPENDS SIMMProgrammer.elf
)
add_custom_target(SIMMProgrammer_bin ALL DEPENDS SIMMProgrammer.bin)
