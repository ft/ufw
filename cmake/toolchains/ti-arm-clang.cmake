# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_ID "ti-arm-clang")
set(COMPILER_API "gnu")

set(cc tiarmclang)
if (DEFINED TOOLCHAIN_CC_TI_ARM_CLANG)
  set(cc ${TOOLCHAIN_CC_TI_ARM_CLANG})
endif()
set(cxx tiarmclang)
if (DEFINED TOOLCHAIN_CXX_TI_ARM_CLANG)
  set(cxx ${TOOLCHAIN_CXX_TI_ARM_CLANG})
endif()

if (DEFINED ENV{TOOLCHAIN_ROOT_TI_ARM_CLANG})
  find_program(COMPILER_BINARY ${cc}
    PATH_SUFFIXES "bin"
    HINTS ENV TOOLCHAIN_ROOT_TI_ARM_CLANG
    REQUIRED)
else()
  # This resembles the old to-arm toolchain file's default behaviour.
  find_program(COMPILER_BINARY ${cc}
    PATH_SUFFIXES "bin"
    HINTS "/opt/ti/ti-cgt-armllvm_3.2.2.LTS"
    REQUIRED)
endif()

get_filename_component(TOOLCHAIN_ROOT ${COMPILER_BINARY} DIRECTORY)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_ROOT}/${cc})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/${cc})
set(OBJCOPY            ${TOOLCHAIN_ROOT}/tiarmobjcopy)
set(OBJDUMP            ${TOOLCHAIN_ROOT}/tiarmobjdump)
set(SIZE               ${TOOLCHAIN_ROOT}/tiarmsize)

set(CMAKE_C_FLAGS_INIT
    "-fdata-sections -ffunction-sections -frecord-gcc-switches -pipe")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
