# PC-specific include paths
target_include_directories(SIMMProgrammer.elf PRIVATE
	hal/pc
)

# PC-specific compile options
target_compile_definitions(SIMMProgrammer.elf PRIVATE
	IS_PC_BUILD=1
)
set_property(TARGET SIMMProgrammer.elf PROPERTY CMAKE_CXX_STANDARD 11)

# Link against Qt
target_link_libraries(SIMMProgrammer.elf PRIVATE Qt5::Widgets Qt5::Concurrent)
