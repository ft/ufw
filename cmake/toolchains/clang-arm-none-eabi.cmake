# Copyright (c) 2018-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if (${CMAKE_TOOLCHAIN_FILE})
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(triple arm-none-eabi)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})

# TODO: This needs to be much more generic. Works with current Debian packages
# for this toolchain, though.
set(CMAKE_SYSROOT /usr/lib/arm-none-eabi)

set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=bfd -static -nostartfiles -nodefaultlibs -nostdlib")

set(TOOLCHAIN_ID "clang-arm")
set(COMPILER_API "gnu")
