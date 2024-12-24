cmake_minimum_required (VERSION 3.13.4)
project(ufw C CXX)

include(CheckIncludeFile)
include(CheckSymbolExists)

include(CTest)
enable_testing()

option(UFW_USE_BUILTIN_SWAP "Use __builtin_bswapXX() if available." ON)
set(UFW_PRIVATE_ERRNO_OFFSET 16384 CACHE STRING "Offset for errno-extensions")

# This is mainly used for development, to test ABI/API compatibility. Users
# should use static linking with ufw. This will create a dynamic library, that
# is called libufw-full.
option(UFW_ENABLE_DYNAMIC_LIBRARY
  "Enable dynamic libufw build with full API" OFF)

if (ZEPHYR_BASE)
  include(zephyr/cmake-cache.cmake)
endif()

if ((NOT CMAKE_BUILD_TYPE) OR ("${CMAKE_BUILD_TYPE}" STREQUAL ""))
  set(CMAKE_BUILD_TYPE "debug")
endif()

if (NOT MICROFRAMEWORK_ROOT)
  set(MICROFRAMEWORK_ROOT ${PROJECT_SOURCE_DIR})
endif()
if (NOT MICROFRAMEWORK_BINARY_ROOT)
  set(MICROFRAMEWORK_BINARY_ROOT ${PROJECT_BINARY_DIR})
endif()

include(InitialiseToolchain OPTIONAL RESULT_VARIABLE WITH_INITIALISE_TOOLCHAIN)
if (NOT WITH_INITIALISE_TOOLCHAIN)
  list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
  message(STATUS "InitialiseToolchain missing; trying local cmake-modules")
  include(InitialiseToolchain OPTIONAL RESULT_VARIABLE WITH_INITIALISE_TOOLCHAIN)
endif()

include(UFWCompiler)

if (WITH_INITIALISE_TOOLCHAIN)
  initialise_toolchain()
else()
  message(FATAL_ERROR "InitialiseToolchain missing! Giving up!")
endif()

include(SetupTargetCPU)
check_include_file("sys/types.h" WITH_SYS_TYPES_H)

check_include_file("ctype.h" WITH_CTYPE_H)
if (WITH_CTYPE_H)
  check_symbol_exists(isprint "ctype.h" UFW_HAVE_CTYPE_ISPRINT)
endif()

check_include_file("unistd.h" WITH_UNISTD_H)
if (WITH_UNISTD_H)
  check_symbol_exists(read  "unistd.h" UFW_HAVE_POSIX_READ)
  check_symbol_exists(write "unistd.h" UFW_HAVE_POSIX_WRITE)
endif()

ufw_compiler_has_type(uint8_t WITH_UINT8_T)

ufw_force_compat(force_compat)
if (${force_compat} EQUAL 0)
  check_symbol_exists(strlcat "string.h" UFW_COMPAT_HAVE_STRLCAT)
  check_symbol_exists(strlcpy "string.h" UFW_COMPAT_HAVE_STRLCPY)
  check_symbol_exists(strnlen "string.h" UFW_COMPAT_HAVE_STRNLEN)
else()
  set(UFW_COMPAT_HAVE_STRLCAT 0)
  set(UFW_COMPAT_HAVE_STRLCPY 0)
  set(UFW_COMPAT_HAVE_STRNLEN 0)
endif()

configure_file("${PROJECT_SOURCE_DIR}/include/ufw/toolchain.h.in"
               "${PROJECT_BINARY_DIR}/include/ufw/toolchain.h" )

set(__ufw_sources src/allocator.c
                  src/crc-16-arc.c
                  src/endpoints/buffer.c
                  src/endpoints/continuable-sink.c
                  src/endpoints/core.c
                  src/endpoints/instrumentable.c
                  src/endpoints/posix.c
                  src/endpoints/trivial.c
                  src/length-prefix.c
                  src/hexdump.c
                  src/byte-buffer.c
                  src/persistent-storage.c
                  src/registers/core.c
                  src/registers/utilities.c
                  src/register-protocol.c
                  src/rfc1055.c
                  src/ring-buffer-iter.c
                  src/variable-length-integer.c)

if (WITH_UINT8_T)
  list(APPEND __ufw_sources src/octet-ring.c)
endif()

set(__ufw_include ${CMAKE_CURRENT_SOURCE_DIR}/include
                  ${CMAKE_CURRENT_BINARY_DIR}/include)

set(__ufw_tap_sources src/test/memdiff.c
                      src/test/output-utils.c
                      src/test/tap.c)

set(__ufw_sx_sources src/sx.c)

set(__ufw_targets)

function(__ufw_setup lib)
  list(APPEND __ufw_targets "${lib}")
  set(__ufw_targets "${__ufw_targets}" PARENT_SCOPE)
  set_property(TARGET ${lib} PROPERTY C_STANDARD 99)
  set_target_cpu(${lib})
  MakeStrictCompilerC(${lib})
endfunction()

function(__ufw_sanitise lib)
  ufw_toolchain(${lib} FEATURES sanitize-address
                                sanitize-integer
                                sanitize-undefined-behaviour)
endfunction()

if (NOT UFW_COMPAT_HAVE_STRLCAT)
  list(APPEND __ufw_sources src/compat/strlcat.c)
endif()

if (NOT UFW_COMPAT_HAVE_STRLCPY)
  list(APPEND __ufw_sources src/compat/strlcpy.c)
endif()

if (NOT UFW_COMPAT_HAVE_STRNLEN)
  list(APPEND __ufw_sources src/compat/strnlen.c)
endif()

add_library(ufw STATIC ${__ufw_sources})
__ufw_setup(ufw)
__ufw_sanitise(ufw)
target_include_directories(ufw PUBLIC ${__ufw_include})
if (UFW_USE_BUILTIN_SWAP)
  target_compile_definitions(ufw PUBLIC UFW_USE_BUILTIN_SWAP)
endif()
# TODO: Can we test for this requirement? Picolibc needs this.
target_compile_definitions(ufw PUBLIC _DEFAULT_SOURCE)

if (NOT ("${TOOLCHAIN_ID}" MATCHES "^ti-"))
  target_link_libraries(ufw PUBLIC m)
endif()

add_library(ufw-tap STATIC ${__ufw_tap_sources})
add_library(ufw-sx STATIC ${__ufw_sx_sources})

foreach (__lib ufw-tap ufw-sx)
  __ufw_setup(${__lib})
  __ufw_sanitise(${__lib})
  target_link_libraries(${__lib} PUBLIC ufw)
endforeach()

if ("${PROJECT_TARGET_CPU}" STREQUAL "native")
  add_library(ufw-nosan STATIC ${__ufw_sources})
  __ufw_setup(ufw-nosan)
  target_include_directories(ufw-nosan PUBLIC ${__ufw_include})
  if (UFW_USE_BUILTIN_SWAP)
    target_compile_definitions(ufw-nosan PUBLIC UFW_USE_BUILTIN_SWAP)
  endif()

  if (NOT ("${TOOLCHAIN_ID}" MATCHES "^ti-"))
    target_link_libraries(ufw-nosan PUBLIC m)
  endif()

  add_library(ufw-tap-nosan STATIC ${__ufw_tap_sources})
  add_library(ufw-sx-nosan STATIC ${__ufw_sx_sources})
  foreach (__lib ufw-tap ufw-sx)
    __ufw_setup(${__lib}-nosan)
    target_link_libraries(${__lib}-nosan PUBLIC ufw-nosan)
  endforeach()
endif()

if (UFW_ENABLE_DYNAMIC_LIBRARY)
  add_library(ufw-full SHARED
    ${__ufw_sources}
    ${__ufw_sx_sources}
    ${__ufw_tap_sources})
  __ufw_setup(ufw-full)
  __ufw_sanitise(ufw)
  target_include_directories(ufw-full PUBLIC ${__ufw_include})
  target_compile_definitions(ufw-full PUBLIC _DEFAULT_SOURCE)
endif()

if (ZEPHYR_BASE)
  add_subdirectory_ifdef(CONFIG_UFW_ZEPHYR src/z)
endif()

add_subdirectory(examples)
add_subdirectory(test)
add_subdirectory(doc)
