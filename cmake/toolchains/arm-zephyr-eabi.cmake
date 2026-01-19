# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_ID "gcc-arm")
set(COMPILER_API "gnu")

set(cc arm-zephyr-eabi-gcc)
if (DEFINED TOOLCHAIN_CC_ARM_ZEPHYR_EABI)
  set(cc ${TOOLCHAIN_CC_ARM_ZEPHYR_EABI})
endif()
set(cxx arm-zephyr-eabi-g++)
if (DEFINED TOOLCHAIN_CXX_ARM_ZEPHYR_EABI)
  set(cxx ${TOOLCHAIN_CXX_ARM_ZEPHYR_EABI})
endif()

find_program(COMPILER_BINARY ${cc}
  PATH_SUFFIXES "bin"
  HINTS ENV TOOLCHAIN_ROOT_ARM_ZEPHYR_EABI
  REQUIRED)
get_filename_component(TOOLCHAIN_ROOT ${COMPILER_BINARY} DIRECTORY)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_ROOT}/arm-zephyr-eabi-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/arm-zephyr-eabi-g++)
set(OBJCOPY            ${TOOLCHAIN_ROOT}/arm-zephyr-eabi-objcopy)
set(OBJDUMP            ${TOOLCHAIN_ROOT}/arm-zephyr-eabi-objdump)
set(SIZE               ${TOOLCHAIN_ROOT}/arm-zephyr-eabi-size)

set(CMAKE_C_FLAGS_INIT
    "-fdata-sections -ffunction-sections -frecord-gcc-switches -pipe")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
