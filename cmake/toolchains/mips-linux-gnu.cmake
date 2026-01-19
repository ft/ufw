# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mips)

set(TOOLCHAIN_ID "gcc-mips")
set(COMPILER_API "gnu")

set(cc mips-linux-gnu-gcc)
if (DEFINED TOOLCHAIN_CC_GNU_MIPS_LINUX_GNU)
  set(cc ${TOOLCHAIN_CC_GNU_MIPS_LINUX_GNU})
endif()
set(cxx mips-linux-gnu-g++)
if (DEFINED TOOLCHAIN_CXX_GNU_MIPS_LINUX_GNU)
  set(cxx ${TOOLCHAIN_CXX_GNU_MIPS_LINUX_GNU})
endif()

find_program(COMPILER_BINARY ${cc}
  PATH_SUFFIXES "bin"
  HINTS ENV TOOLCHAIN_ROOT_GNU_MIPS_LINUX_GNU
  REQUIRED)
get_filename_component(TOOLCHAIN_ROOT ${COMPILER_BINARY} DIRECTORY)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_ROOT}/mips-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/mips-linux-gnu-g++)
set(OBJCOPY            ${TOOLCHAIN_ROOT}/mips-linux-gnu-objcopy)
set(OBJDUMP            ${TOOLCHAIN_ROOT}/mips-linux-gnu-objdump)
set(SIZE               ${TOOLCHAIN_ROOT}/mips-linux-gnu-size)

set(CMAKE_C_FLAGS_INIT
    "-static -fdata-sections -ffunction-sections -frecord-gcc-switches -pipe")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
