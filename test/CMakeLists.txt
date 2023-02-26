cmake_minimum_required (VERSION 3.13.4)

if (DEFINED UFW_ZEPHYR_KERNEL)
  return()
endif()

include(UFWTest)
ufw_test_init()

set(test_names)
if (WITH_UINT8_T)
  list(APPEND test_names t-binary-format)
endif()
list(APPEND test_names
  t-convolution-low-pass
  t-length-prefix
  t-hexdump
  t-persistent-storage
  t-register-table
  t-register-table-persistent-storage
  t-rfc1055
  t-ring-buffer
  t-varint)

set(test_prgs)
foreach (tst ${test_names})
  ufw_test_program(${tst} ${tst}.c)
  ufw_toolchain(${tst} FEATURES sanitize-address
                                sanitize-integer
                                sanitize-undefined-behaviour)
  target_link_libraries(${tst} PRIVATE ufw ufw-tap)
  if (DEFINED PROJECT_TARGET_CPU AND ${PROJECT_TARGET_CPU} STREQUAL native)
    target_compile_definitions(${tst} PRIVATE UFW_NATIVE_BUILD)
  endif()
  MakeStrictCompilerC(${tst})
  list(APPEND test_prgs $<TARGET_FILE:${tst}>)
endforeach()

ufw_add_test_runner(
  NAME ufw-c-unit-tests
  TESTS ${test_prgs})
