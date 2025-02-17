# Copyright (c) 2020-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_Test)
  return()
endif()
set(__UFW_Test 1)

include(InitialiseToolchain)
include(Newlib)

set(UFW_TEST_USE_GUILE_TAP ON CACHE BOOL
  "Use guile-tap's tap-harness program, if available.")

set(UFW_TEST_HARNESS_PROGRAM "" CACHE STRING
  "TAP test harness program name.")

# Feature set of UFW_TEST_HARNESS_PROGRAM. We assume, that your harness is
# either a drop in replacement for Perl's prove programe or guile-tap's
# tap-harness program. The default is perl-prove, which gets overridden when
# the _PROGRAM parameter is empty and the automatic detection runs.
set(UFW_TEST_HARNESS_TYPE "perl-prove" CACHE STRING
  "TAP test harness type; either perl-prove or guile-tap-harness.")

# This works when guile-tap's tap-harness program is available and in use.
set(UFW_TEST_OUTPUT_PREFIX "${CMAKE_INSTALL_PREFIX}/tests" CACHE PATH
  "Directory to place test TAP output.")

# This requires a TAP harness that supports this, like guile-tap's tap-harness.
set(UFW_TEST_HARNESS_LOGDATA OFF CACHE BOOL
  "Make TAP harness log test data to UFW_TEST_OUTPUT_PREFIX.")

function(ufw_test_a_harness name pkg program)
  set(found found-NOTFOUND)
  find_program(found ${name})
  if (NOT (${found} STREQUAL "found-NOTFOUND"))
    execute_process(
      COMMAND ${found} --version
      OUTPUT_VARIABLE UFW_HARNESS_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      RESULT_VARIABLE UFW_HARNESS_VERSION_RESULT)
    if ("${UFW_HARNESS_VERSION_RESULT}" STREQUAL "0")
      message(STATUS "TAP harness: ${found} (${UFW_HARNESS_VERSION})")
      set(${program} ${found} PARENT_SCOPE)
      return()
    endif()
  endif()
  message(STATUS "${name} (from ${pkg}) NOT found.")
  set(${program} "" PARENT_SCOPE)
endfunction()

function(ufw_test_harness _program _type)
  if (${UFW_TEST_USE_GUILE_TAP})
    ufw_test_a_harness(tap-harness guile-tap program)
    if (NOT ("${program}" STREQUAL ""))
      set(${_program} ${program} PARENT_SCOPE)
      set(${_type} guile-tap-harness PARENT_SCOPE)
      return()
    endif()
  endif()
  ufw_test_a_harness(prove perl program)
  if (NOT ("${program}" STREQUAL ""))
    set(${_type} perl-prove PARENT_SCOPE)
  endif()
  set(${_program} ${program} PARENT_SCOPE)
endfunction()

function(ufw_test_qemu arch with_qemu)
  find_program(found_qemu qemu-${arch})
  if (${found_qemu} STREQUAL "found_qemu-NOTFOUND")
    message(STATUS "QEMU for ${arch} NOT found!")
    set(${with_qemu} False PARENT_SCOPE)
  else()
    message(STATUS "QEMU for ${arch} found: ${found_qemu}")
    set(${with_qemu} True PARENT_SCOPE)
  endif()
endfunction()

function(ufw_get_test_runner var)
  if ("${PROJECT_TARGET_CPU}" STREQUAL cortex-m3)
    set(${var} "${MICROFRAMEWORK_ROOT}/target/cortex-m3/bin/run" PARENT_SCOPE)
  elseif ("${PROJECT_TARGET_CPU}" STREQUAL mips)
    set(${var} "${MICROFRAMEWORK_ROOT}/target/mips/bin/run" PARENT_SCOPE)
  else()
    set(${var} "" PARENT_SCOPE)
  endif()
endfunction()

function(ufw_target_link_options target)
  if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13.0")
    target_link_options(${target} PRIVATE ${ARGN})
  else()
    target_link_libraries(${target} PRIVATE ${ARGN})
  endif()
endfunction()

function(ufw_test_program target)
  if (${__UFW_TEST_CAN_LINK_TESTS})
    add_executable(${target} ${ARGN})

    if (${__UFW_TEST_BUILD} STREQUAL cortex-m3)
      ufw_target_link_options(${target} -nostartfiles)
      target_sources(${target} PRIVATE
        ${MICROFRAMEWORK_ROOT}/target/cortex-m3/init/startup.c)
      target_linker_script(${target}
        ${MICROFRAMEWORK_ROOT}/target/cortex-m3/ld/arm-cortex-qemu.ld)
      newlib_target_spec(${target} rdimon)
    elseif (${__UFW_TEST_BUILD} STREQUAL cortex-generic)
      newlib_target_spec(${target} nosys)
    endif()

  else()
    add_library(${target} STATIC ${ARGN})
  endif()

  set_target_cpu(${target})
endfunction()

function(ufw_test_init)
  set(can_run_tests True)
  set(can_link_tests True)
  set(test_build native)

  if (NOT DEFINED PROJECT_TARGET_CPU)
    message(STATUS "ufw-test: Generic build, using native test runner")
  elseif (${PROJECT_TARGET_CPU} STREQUAL "native")
    message(STATUS "ufw-test: Native build set, using according test runner")
  elseif (${PROJECT_TARGET_CPU} STREQUAL "mips")
    message(STATUS "ufw-test: Compiling for mips; running via qemu")
    ufw_test_qemu(mips with_qemu)
    set(test_build mips)
  elseif (${PROJECT_TARGET_CPU} STREQUAL "cortex-m3")
    if (${COMPILER_API} STREQUAL gnu)
      ufw_test_qemu(system-arm with_qemu)
      if (${with_qemu})
        message(STATUS "ufw-test: Compiling for cortex-m3; running via qemu")
      else()
        set(can_run_tests False)
        message(STATUS "ufw-test: Compiling for cortex-m3; cannot run without qemu")
      endif()
    else()
      message(STATUS "ufw-test: Compiling for cortex-m3")
      message(STATUS "ufw-test: Cannot link tests with compiler API: ${COMPILER_API}")
      set(can_link_tests False)
      set(can_run_tests False)
    endif()
    set(test_build cortex-m3)
  elseif (${PROJECT_TARGET_CPU} MATCHES "^cortex-")
    message(STATUS "ufw-test: Compiling for ${PROJECT_TARGET_CPU}; running not supported")
    if (NOT (${COMPILER_API} STREQUAL gnu))
      set(can_link_tests False)
    endif()
    set(test_build cortex-generic)
    set(can_run_tests False)
  else()
    message(STATUS
      "ufw-test: Compiling for ${PROJECT_TARGET_CPU}; Running tests not supported!")
    set(can_link_tests False)
    set(can_run_tests False)
  endif()

  if ("${UFW_TEST_HARNESS_PROGRAM}" STREQUAL "")
    ufw_test_harness(harness type)
    set(UFW_TEST_HARNESS_PROGRAM ${harness} PARENT_SCOPE)
    set(UFW_TEST_HARNESS_TYPE ${type} PARENT_SCOPE)
  else()
    message(STATUS
      "TAP harness: ${UFW_TEST_HARNESS_PROGRAM} (${UFW_TEST_HARNESS_TYPE})")
  endif()

  set(__UFW_TEST_CAN_RUN_TESTS ${can_run_tests} PARENT_SCOPE)
  set(__UFW_TEST_CAN_LINK_TESTS ${can_link_tests} PARENT_SCOPE)
  set(__UFW_TEST_BUILD ${test_build} PARENT_SCOPE)
endfunction()

function(ufw_add_test_runner)
  if (NOT ${__UFW_TEST_CAN_RUN_TESTS})
    return()
  endif()
  if (${UFW_TEST_HARNESS_PROGRAM} STREQUAL "")
    message(STATUS "No TAP harness detected, cannot execute test-suite.")
    return()
  endif()

  set(cmd)
  if ("${UFW_TEST_HARNESS_TYPE}" STREQUAL "guile-tap-harness")
    list(APPEND cmd ${UFW_TEST_HARNESS_PROGRAM} --verbose)
    if (UFW_TEST_HARNESS_LOGDATA)
      list(APPEND cmd --log-dir ${UFW_TEST_OUTPUT_PREFIX})
    endif()
  elseif ("${UFW_TEST_HARNESS_TYPE}" STREQUAL "perl-prove")
    list(APPEND cmd ${UFW_TEST_HARNESS_PROGRAM} --verbose --merge)
  else()
    message(FATAL_ERROR
      "Invalid UFW_TEST_HARNESS_TYPE: ${UFW_TEST_HARNESS_TYPE}")
  endif()

  cmake_parse_arguments(PA "" "NAME" "TESTS" ${ARGN})
  ufw_get_test_runner(runner)

  if ("${runner}" STREQUAL "")
    list(APPEND cmd ${PA_TESTS})
  else()
    list(APPEND cmd --exec "${runner}" ${PA_TESTS})
  endif()

  add_test(NAME ${PA_NAME} COMMAND ${cmd})
endfunction()
