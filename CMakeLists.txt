cmake_minimum_required (VERSION 3.13.4)
project(ufw C CXX)

include(CTest)
enable_testing()

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

if (WITH_INITIALISE_TOOLCHAIN)
  initialise_toolchain()
else()
  message(FATAL_ERROR "InitialiseToolchain missing! Giving up!")
endif()

include(SetupTargetCPU)
include(CheckIncludeFile)
check_include_file("sys/types.h" WITH_SYS_TYPES_H)

include(CheckTypeSize)
check_type_size("uint8_t" UFW_UINT8_T_SIZE)
if (UFW_UINT8_T_SIZE)
    set(UINT8_T_SIZE ${UFW_UINT8_T_SIZE})
    set(WITH_UINT8_T 1)
endif()

ufw_force_compat(force_compat)
if (${force_compat} EQUAL 0)
  include(CheckSymbolExists)
  check_symbol_exists(strlcat "string.h" UFW_COMPAT_HAVE_STRLCAT)
  check_symbol_exists(strlcpy "string.h" UFW_COMPAT_HAVE_STRLCPY)
  check_symbol_exists(strnlen "string.h" UFW_COMPAT_HAVE_STRNLEN)
else()
  message(STATUS "ufw: Zephyr minimal libc: Forcing compat code inclusion")
  set(UFW_COMPAT_HAVE_STRLCAT 0)
  set(UFW_COMPAT_HAVE_STRLCPY 0)
  set(UFW_COMPAT_HAVE_STRNLEN 0)
endif()

configure_file("${PROJECT_SOURCE_DIR}/include/ufw/toolchain.h.in"
               "${PROJECT_BINARY_DIR}/include/ufw/toolchain.h" )

set(__ufw_sources src/deprecated/logger.c
                  src/persistent-storage.c
                  src/ring-buffer-iter.c
                  src/register-table.c
                  src/register-utilities.c)

set(__ufw_include ${CMAKE_CURRENT_SOURCE_DIR}/include
                  ${CMAKE_CURRENT_BINARY_DIR}/include)

set(__ufw_tap_sources src/test/memdiff.c
                      src/test/output-utils.c
                      src/test/tap.c)

function(__ufw_setup lib)
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

if (NOT ("${TOOLCHAIN_ID}" MATCHES "^ti-"))
  target_link_libraries(ufw PUBLIC m)
endif()

add_library(ufw-tap STATIC ${__ufw_tap_sources})
__ufw_setup(ufw-tap)
__ufw_sanitise(ufw-tap)
target_link_libraries(ufw-tap PUBLIC ufw)

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "native")
  add_library(ufw-nosan STATIC ${__ufw_sources})
  __ufw_setup(ufw-nosan)
  target_include_directories(ufw-nosan PUBLIC ${__ufw_include})

  if (NOT ("${TOOLCHAIN_ID}" MATCHES "^ti-"))
    target_link_libraries(ufw-nosan PUBLIC m)
  endif()

  add_library(ufw-tap-nosan STATIC ${__ufw_tap_sources})
  __ufw_setup(ufw-tap-nosan)
  target_link_libraries(ufw-tap-nosan PUBLIC ufw-nosan)
endif()

add_subdirectory(test)
add_subdirectory(doc)
