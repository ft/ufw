cmake_minimum_required(VERSION 3.8.2)

if (((NOT PROJECT_TARGET_CPU) OR (NOT ${PROJECT_TARGET_CPU} STREQUAL "native"))
    AND CMAKE_CROSSCOMPILING)
  message(STATUS "Cross compiling; skipping test-suite")
  return()
endif()

include(CheckIncludeFile)

set(test_names)

if (NOT TARGET tap)
  message(STATUS "Libtap wasn't found, giving up on test-suite!")
  return()
endif()

list(APPEND test_names
  t-binary-format
  t-persistent-storage
  t-register-table
  t-ring-buffer)

set(test_prgs)
list(APPEND test_prgs)
foreach (tst ${test_names})
  add_executable(${tst} ${tst}.c)
  ufw_toolchain(${tst} FEATURES sanitize-address
                                sanitize-integer
                                sanitize-undefined-behaviour)
  target_link_libraries(${tst} PRIVATE ufw tap)
  MakeStrictCompilerC(${tst})
  MakeFatalCompilerC(${tst})
  set_target_cpu(${tst})
  list(APPEND test_prgs $<TARGET_FILE:${tst}>)
endforeach()

add_test(
  NAME ufw-c-unit-tests
  COMMAND prove --verbose --merge --color --exec "" ${test_prgs})
