# Copyright (c) 2018-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_ID "gcc-arm")
set(COMPILER_API "gnu")

set(cc arm-none-eabi-gcc)
if (DEFINED TOOLCHAIN_CC_GNU_ARM_NONE_EABI)
  set(cc ${TOOLCHAIN_CC_GNU_ARM_NONE_EABI})
endif()
set(cxx arm-none-eabi-g++)
if (DEFINED TOOLCHAIN_CXX_GNU_ARM_NONE_EABI)
  set(cxx ${TOOLCHAIN_CXX_GNU_ARM_NONE_EABI})
endif()

find_program(COMPILER_BINARY ${cc}
  PATH_SUFFIXES "bin"
  HINTS ENV TOOLCHAIN_ROOT_GNU_ARM_NONE_EABI
  REQUIRED)
get_filename_component(TOOLCHAIN_ROOT ${COMPILER_BINARY} DIRECTORY)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_ROOT}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/arm-none-eabi-g++)
set(OBJCOPY            ${TOOLCHAIN_ROOT}/arm-none-eabi-objcopy)
set(OBJDUMP            ${TOOLCHAIN_ROOT}/arm-none-eabi-objdump)
set(SIZE               ${TOOLCHAIN_ROOT}/arm-none-eabi-size)

set(CMAKE_C_FLAGS_INIT
    "-fdata-sections -ffunction-sections -frecord-gcc-switches -pipe")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
