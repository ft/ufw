if(__UFW_Test)
  return()
endif()
set(__UFW_Test 1)

include(InitialiseToolchain)
include(Newlib)

function(ufw_test_qemu arch with_qemu)
  find_program(found_qemu qemu-system-${arch})
  if (${found_qemu} STREQUAL "found_qemu-NOTFOUND")
    message(STATUS "QEMU system for ${arch} NOT found!")
    set(${with_qemu} False PARENT_SCOPE)
  else()
    message(STATUS "QEMU system for ${arch} found: ${found_qemu}")
    set(${with_qemu} True PARENT_SCOPE)
  endif()
endfunction()

function(ufw_get_test_runner var)
  if ("${PROJECT_TARGET_CPU}" STREQUAL cortex-m3)
    set(${var} "${MICROFRAMEWORK_ROOT}/target/cortex-m3/bin/run" PARENT_SCOPE)
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
  elseif (${PROJECT_TARGET_CPU} STREQUAL "cortex-m3")
    if (${COMPILER_API} STREQUAL gnu)
      ufw_test_qemu(arm with_qemu)
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

  set(__UFW_TEST_CAN_RUN_TESTS ${can_run_tests} PARENT_SCOPE)
  set(__UFW_TEST_CAN_LINK_TESTS ${can_link_tests} PARENT_SCOPE)
  set(__UFW_TEST_BUILD ${test_build} PARENT_SCOPE)
endfunction()

function(ufw_add_test_runner)
  if (NOT ${__UFW_TEST_CAN_RUN_TESTS})
    return()
  endif()
  cmake_parse_arguments(PA "" "NAME" "TESTS" ${ARGN})
  ufw_get_test_runner(runner)
  add_test(
    NAME ${PA_NAME}
    COMMAND prove --verbose --merge --color --exec "${runner}" ${PA_TESTS})
endfunction()
