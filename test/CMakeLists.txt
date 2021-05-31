cmake_minimum_required(VERSION 3.8.2)

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
  t-persistent-storage
  t-register-table
  t-ring-buffer)

set(test_prgs)
foreach (tst ${test_names})
  ufw_test_program(${tst} ${tst}.c)
  ufw_toolchain(${tst} FEATURES sanitize-address
                                sanitize-integer
                                sanitize-undefined-behaviour)
  target_link_libraries(${tst} PRIVATE ufw ufw-tap)
  MakeStrictCompilerC(${tst})
  list(APPEND test_prgs $<TARGET_FILE:${tst}>)
endforeach()

ufw_add_test_runner(
  NAME ufw-c-unit-tests
  TESTS ${test_prgs})
