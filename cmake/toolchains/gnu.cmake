# Copyright (c) 2019-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR native)

set(TOOLCHAIN_ID "gcc-native")
set(COMPILER_API "gnu")

set(cc gcc)
if (DEFINED TOOLCHAIN_CC_GNU)
  set(cc ${TOOLCHAIN_CC_GNU})
endif()
set(cxx g++)
if (DEFINED TOOLCHAIN_CXX_GNU)
  set(cxx ${TOOLCHAIN_CXX_GNU})
endif()

find_program(COMPILER_BINARY ${cc}
  PATH_SUFFIXES "bin"
  HINTS ENV TOOLCHAIN_ROOT_GNU
  REQUIRED)
get_filename_component(TOOLCHAIN_ROOT ${COMPILER_BINARY} DIRECTORY)

set(CMAKE_C_COMPILER ${TOOLCHAIN_ROOT}/${cc})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/${cxx})

set(TOOLCHAIN_FEATURES
  sanitize-address
  sanitize-integer
  sanitize-undefined-behaviour)

set(TOOLCHAIN_FEATURE_SANITIZE_ADDRESS
  COMPILER_OPTIONS -fsanitize=address -fno-omit-frame-pointer
  LINKER_OPTIONS   -fsanitize=address -fno-omit-frame-pointer
  LINKER_LIBRARIES -lasan)

set(TOOLCHAIN_FEATURE_SANITIZE_INTEGER
  COMPILER_OPTIONS -fsanitize=integer-divide-by-zero
  LINKER_OPTIONS   -fsanitize=integer-divide-by-zero)

set(TOOLCHAIN_FEATURE_SANITIZE_UNDEFINED_BEHAVIOUR
  COMPILER_OPTIONS -fsanitize=undefined
  LINKER_OPTIONS   -fsanitize=undefined)
