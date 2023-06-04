# This will tell CMake that we are cross compiling
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR avr)

# Make sure it knows what binaries to use
set(CMAKE_AR avr-ar)
set(CMAKE_ASM_COMPILER avr-as)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_LINKER avr-ld)
set(CMAKE_OBJCOPY avr-objcopy)
set(CMAKE_RANLIB avr-ranlib)
set(CMAKE_SIZE avr-size)
set(CMAKE_STRIP avr-strip)
