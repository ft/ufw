set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

find_program(BINUTILS_PATH arm-none-eabi-gcc)

get_filename_component(ARM_TOOLCHAIN_DIR ${BINUTILS_PATH} DIRECTORY)

set(CMAKE_C_COMPILER ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-g++)

set(ARM_OBJCOPY ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objcopy)
set(ARM_OBJDUMP ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objdump)
set(ARM_SIZE ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-size)

set(CMAKE_C_FLAGS_INIT
    "-fdata-sections -ffunction-sections -frecord-gcc-switches -pipe")

# Need to setup the linker so it doesn't fail with the simplest of test programs.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostartfiles")

set(OBJCOPY ${ARM_OBJCOPY})
set(OBJDUMP ${ARM_OBJDUMP})
set(SIZE ${ARM_SIZE})

set(TOOLCHAIN_ID "gcc-arm")
set(COMPILER_API "gnu")
