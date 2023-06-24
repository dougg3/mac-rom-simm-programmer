# This will tell CMake that we are cross compiling
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Make sure it knows what binaries to use
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_LINKER arm-none-eabi-ld)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_RANLIB arm-none-eabi-ranlib)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_STRIP arm-none-eabi-strip)
