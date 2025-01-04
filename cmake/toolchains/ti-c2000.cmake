# Copyright (c) 2018-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR c2000)

set(TOOLCHAIN_ID "ti-c2000")
set(COMPILER_API "ti")

set(cc cl2000)
if (DEFINED TOOLCHAIN_CC_TI_C2000)
  set(cc ${TOOLCHAIN_CC_TI_C2000})
endif()
set(cxx cl2000)
if (DEFINED TOOLCHAIN_CXX_TI_C2000)
  set(cxx ${TOOLCHAIN_CXX_TI_C2000})
endif()

if (DEFINED ENV{TOOLCHAIN_ROOT_TI_C2000})
  find_program(COMPILER_BINARY ${cc}
    PATH_SUFFIXES "bin"
    HINTS ENV TOOLCHAIN_ROOT_TI_C2000
    REQUIRED)
else()
  # This resembles the old toolchain file's default behaviour.
  find_program(COMPILER_BINARY ${cc}
    PATH_SUFFIXES "bin"
    HINTS "/opt/ti/ti-cgt-c2000_18.1.2.LTS"
    REQUIRED)
endif()
get_filename_component(TOOLCHAIN_BIN ${COMPILER_BINARY} DIRECTORY)
get_filename_component(TOOLCHAIN_ROOT ${TOOLCHAIN_BIN} DIRECTORY)

set(CMAKE_C_COMPILER          ${TOOLCHAIN_BIN}/${cc})
set(CMAKE_CXX_COMPILER        ${TOOLCHAIN_BIN}/${cxx})
set(AS                        ${TOOLCHAIN_BIN}/${cc})
set(AR                        ${TOOLCHAIN_BIN}/ar2000)
set(OBJCOPY                   ${TOOLCHAIN_BIN}/ofd2000)
set(OBJDUMP                   ${TOOLCHAIN_BIN}/hex2000)
set(SIZE                      ${TOOLCHAIN_BIN}/size2000)
set(DISASSEMBLER              ${TOOLCHAIN_BIN}/dis2000)
set(LD                        ${TOOLCHAIN_BIN}/${cc})
# Setting ranlib to touch, since TI's toolchain doesn't have this, and CMake
# tends to pick up the one from the host's native toolchain, which in turn
# breaks the libraries produced by the build system.
set(CMAKE_C_COMPILER_RANLIB   "touch")
set(CMAKE_CXX_COMPILER_RANLIB "touch")
set(CMAKE_RANLIB              "touch")

include_directories(${TOOLCHAIN_ROOT}/include)
link_directories(${TOOLCHAIN_ROOT}/lib)

# This forces the C++ compiler's identification without using
# CMakeForceCompiler, which is deprecated. At some point we'll
# need to understand how compiler identification works, so we
# can fix that.
set(CMAKE_CXX_COMPILER_ID_RUN TRUE)
set(CMAKE_CXX_COMPILER_ID "TI")

# Looks like CMake upstream does not support the C_STANDARD property with TI's
# toolchains. Not cool. Default to the most recent language standards that the
# toolchains we're using support out of the box.
set(CMAKE_C_FLAGS_INIT "--c99 --preproc_with_compile")
set(CMAKE_CXX_FLAGS_INIT "--c++03 --preproc_with_compile")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
