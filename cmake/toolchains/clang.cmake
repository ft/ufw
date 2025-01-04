# Copyright (c) 2019-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR native)

set(TOOLCHAIN_ID "clang-native")
set(COMPILER_API "gnu")

set(cc clang)
if (DEFINED TOOLCHAIN_CC_CLANG)
  set(cc ${TOOLCHAIN_CC_CLANG})
endif()
set(cxx clang++)
if (DEFINED TOOLCHAIN_CXX_CLANG)
  set(cxx ${TOOLCHAIN_CXX_CLANG})
endif()

find_program(COMPILER_BINARY ${cc}
  PATH_SUFFIXES "bin"
  HINTS ENV TOOLCHAIN_ROOT_CLANG
  REQUIRED)
get_filename_component(TOOLCHAIN_ROOT ${COMPILER_BINARY} DIRECTORY)

set(CMAKE_C_COMPILER ${TOOLCHAIN_ROOT}/${cc})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/${cxx})

set(TOOLCHAIN_FEATURES
  sanitize-address
  sanitize-coverage-thorough
  sanitize-integer
  sanitize-undefined-behaviour
  fuzzer-libfuzzer)

set(TOOLCHAIN_FEATURE_SANITIZE_ADDRESS
  COMPILER_OPTIONS -fsanitize=address -fno-omit-frame-pointer
  LINKER_OPTIONS   -fsanitize=address -fno-omit-frame-pointer)

set(TOOLCHAIN_FEATURE_SANITIZE_COVERAGE_FAST
  COMPILER_OPTIONS -fsanitize-coverage=func
  LINKER_OPTIONS   -fsanitize-coverage=func)

set(TOOLCHAIN_FEATURE_SANITIZE_COVERAGE_BASIC
  COMPILER_OPTIONS -fsanitize-coverage=bb
  LINKER_OPTIONS   -fsanitize-coverage=bb)

set(TOOLCHAIN_FEATURE_SANITIZE_COVERAGE_THOROUGH
  COMPILER_OPTIONS -fsanitize-coverage=edge
  LINKER_OPTIONS   -fsanitize-coverage=edge)

set(TOOLCHAIN_FEATURE_SANITIZE_INTEGER
  COMPILER_OPTIONS -fsanitize=integer
  LINKER_OPTIONS   -fsanitize=integer)

set(TOOLCHAIN_FEATURE_SANITIZE_UNDEFINED_BEHAVIOUR
  COMPILER_OPTIONS -fsanitize=undefined
  LINKER_OPTIONS   -fsanitize=undefined)

set(TOOLCHAIN_FEATURE_FUZZER_LIBFUZZER
  LINKER_OPTIONS -lFuzzer)
