cmake_minimum_required (VERSION 3.8.2)
project(ufw C CXX)

if ((NOT CMAKE_BUILD_TYPE) OR ("${CMAKE_BUILD_TYPE}" STREQUAL ""))
  set(CMAKE_BUILD_TYPE "debug")
endif()

include(InitialiseToolchain OPTIONAL RESULT_VARIABLE WITH_INITIALISE_TOOLCHAIN)
if (NOT WITH_INITIALISE_TOOLCHAIN)
  list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
  message(STATUS "InitialiseToolchain missing; trying local cmake-modules")
  include(InitialiseToolchain OPTIONAL RESULT_VARIABLE WITH_INITIALISE_TOOLCHAIN)
endif()

if (WITH_INITIALISE_TOOLCHAIN)
  if (NOT COMPILER_API)
    message(STATUS "COMPILER_API unset; assuming GNU")
    set(COMPILER_API gnu)
  endif()
  if (NOT TOOLCHAIN_ID)
    message(STATUS "TOOLCHAIN_ID unset; assuming gcc-native")
    set(TOOLCHAIN_ID gcc-native)
  endif()
  initialise_toolchain()
else()
  message(FATAL_ERROR "InitialiseToolchain missing! Giving up!")
endif()

include(SetupTargetCPU)
if (NOT PROJECT_TARGET_CPU)
  message(STATUS "PROJECT_TARGET_CPU unset; assuming default")
  set(PROJECT_TARGET_CPU native)
endif()

include(CheckIncludeFile)
check_include_file("sys/types.h" WITH_SYS_TYPES_H)

include(CheckTypeSize)
check_type_size("uint8_t" UFW_UINT8_T_SIZE)
if (UFW_UINT8_T_SIZE)
    set(UINT8_T_SIZE ${UFW_UINT8_T_SIZE})
    set(WITH_UINT8_T 1)
endif()

include(CheckSymbolExists)
check_symbol_exists(strlcat "string.h" UFW_COMPAT_HAVE_STRLCAT)
check_symbol_exists(strlcpy "string.h" UFW_COMPAT_HAVE_STRLCPY)
check_symbol_exists(strnlen "string.h" UFW_COMPAT_HAVE_STRNLEN)

configure_file("${PROJECT_SOURCE_DIR}/include/common/toolchain.h.in"
               "${PROJECT_BINARY_DIR}/include/common/toolchain.h" )

set(__ufw_sources src/c/logger.c
                  src/c/persistent-storage.c
                  src/c/ring-buffer-iter.c
                  src/c/register-table.c
                  src/c/register-utilities.c)

if (NOT UFW_COMPAT_HAVE_STRLCAT)
  list(APPEND __ufw_sources src/c/strlcat.c)
endif()

if (NOT UFW_COMPAT_HAVE_STRLCPY)
  list(APPEND __ufw_sources src/c/strlcpy.c)
endif()

if (NOT UFW_COMPAT_HAVE_STRNLEN)
  list(APPEND __ufw_sources src/c/strnlen.c)
endif()

add_library(ufw STATIC ${__ufw_sources})

set_property(TARGET ufw PROPERTY C_STANDARD 99)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug" AND "${COMPILER_API}" STREQUAL "gnu")
  if ("${TOOLCHAIN_ID}" STREQUAL "gcc-native")
    target_compile_options(ufw PUBLIC
      -fsanitize=address,undefined -fno-omit-frame-pointer)
  elseif ("${TOOLCHAIN_ID}" STREQUAL "clang-native")
    target_compile_options(ufw PUBLIC
      -fsanitize=address,undefined,integer -fno-omit-frame-pointer)
  endif()
endif()

add_library(ufw++ INTERFACE)
target_include_directories(ufw++
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_BINARY_DIR}/include)
add_library(ufw-common INTERFACE)
target_include_directories(ufw-common
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_BINARY_DIR}/include)

target_link_libraries(ufw PUBLIC ufw-common)
if (NOT ("${TOOLCHAIN_ID}" MATCHES "^ti-"))
  target_link_libraries(ufw PUBLIC m)
endif()

MakeStrictCompilerC(ufw)
set_target_cpu(ufw)

add_subdirectory(test)
add_subdirectory(doc)
