cmake_minimum_required(VERSION 3.9)

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
  t-register-table
  t-ring-buffer)

set(test_prgs)
list(APPEND test_prgs)
foreach (tst ${test_names})
  add_executable(${tst} ${tst}.c)
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug" AND "${COMPILER_API}" STREQUAL "gnu")
    target_link_libraries(${tst} PRIVATE ufw tap asan ubsan)
    if ("${TOOLCHAIN_ID}" STREQUAL "gcc-native")
      target_compile_options(${tst} PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    elseif ("${TOOLCHAIN_ID}" STREQUAL "clang-native")
      target_compile_options(${tst} PRIVATE
        -fsanitize=address,undefined,integer -fno-omit-frame-pointer)
    endif()
  else()
    target_link_libraries(${tst} PRIVATE ufw tap)
  endif()
  MakeStrictCompilerC(${tst})
  MakeFatalCompilerC(${tst})
  set_target_cpu(${tst})
  list(APPEND test_prgs $<TARGET_FILE:${tst}>)
endforeach()

add_test(
  NAME ufw-c-unit-tests
  COMMAND prove --verbose --merge --color --exec "" ${test_prgs})
