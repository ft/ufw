set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# TODO: This needs to be much more generic. Works with current Debian packages
# for this toolchain, though.
set(CMAKE_SYSROOT /usr/lib/arm-none-eabi)

set(tools ${CMAKE_SYSROOT})
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

# Need to setup the linker so it doesn't fail with the simplest of test programs.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -nostartfiles -nodefaultlibs -nostdlib")
