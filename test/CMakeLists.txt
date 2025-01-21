# Copyright (c) 2018-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

cmake_minimum_required (VERSION 3.13.4)

if (DEFINED UFW_ZEPHYR_KERNEL)
  return()
endif()

if (${TOOLCHAIN_ID} STREQUAL ti-arm-clang)
  # The startup code used for some builds is not compatibile with this
  # toolchain. So skip tests with these builds for now.
  return()
endif()

include(UFWTest)
ufw_test_init()

set(test_names)
if (WITH_UINT8_T)
  list(APPEND test_names t-binary-format)
endif()
list(APPEND test_names
  t-byte-buffer
  t-convolution-low-pass
  t-endpoints
  t-ep-retry-ctrl
  t-length-prefix
  t-hexdump
  t-persistent-storage
  t-register-protocol
  t-register-table
  t-register-table-persistent-storage
  t-rfc1055
  t-ring-buffer
  t-sx-parser
  t-varint
  t-versioned-persistence)

set(test_prgs)
foreach (tst ${test_names})
  ufw_test_program(${tst} ${tst}.c)
  ufw_toolchain(${tst} FEATURES sanitize-address
                                sanitize-integer
                                sanitize-undefined-behaviour)
  target_link_libraries(${tst} PRIVATE ufw ufw-tap)
  if (${tst} STREQUAL t-sx-parser)
    target_link_libraries(${tst} PRIVATE ufw-sx)
  endif()
  if (DEFINED PROJECT_TARGET_CPU AND ${PROJECT_TARGET_CPU} STREQUAL native)
    target_compile_definitions(${tst} PRIVATE UFW_NATIVE_BUILD)
  endif()
  MakeStrictCompilerC(${tst})
  list(APPEND test_prgs $<TARGET_FILE:${tst}>)
endforeach()

ufw_add_test_runner(
  NAME ufw-c-unit-tests
  TESTS ${test_prgs})
