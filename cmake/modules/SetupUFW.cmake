# Copyright (c) 2019-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_SetupUFW)
  return()
endif()
set(__UFW_SetupUFW 1)

macro(ufw_subtree_build)
  cmake_parse_arguments(PA "" "AUTO_ZEPHYR;PRELOAD" "" ${ARGN})
  if (PA_PRELOAD)
    include(${PA_PRELOAD})
  endif()
  if (NOT DEFINED PA_AUTO_ZEPHYR)
    set(PA_AUTO_ZEPHYR True)
  endif()
  if (DEFINED UFW_LOAD_BUILD_SYSTEM)
    include(${UFW_LOAD_BUILD_SYSTEM})
    return()
  endif()
  if (UFW_TOPLEVEL_ZEPHYR_BUILDSYSTEM_${UFW_ZEPHYR_APPLICATION})
    include("${UFW_TOPLEVEL_ZEPHYR_BUILDSYSTEM_${UFW_ZEPHYR_APPLICATION}}")
    return()
  endif()
  if (PA_AUTO_ZEPHYR AND UFW_ZEPHYR_APPLICATION)
    add_subdirectory(${UFW_ZEPHYR_APPLICATION})
  endif()
endmacro()

function(__ufw_toplevel_args)
  cmake_parse_arguments(PA "" "ROOT" "MODULES" ${ARGN})
  if (NOT PA_ROOT)
    message(FATAL_ERROR "ufw_toplevel: Please specify UFW ROOT directory!")
  endif()
  set(EMBEDDED_CMAKE 1 PARENT_SCOPE)
  if (NOT IS_DIRECTORY "${PA_ROOT}")
    message(FATAL_ERROR
      "Specified MICROFRAMEWORK_ROOT does not exist: ${PA_ROOT}")
  endif()
  set(MICROFRAMEWORK_ROOT "${PA_ROOT}" PARENT_SCOPE)
  if (PA_MODULES)
    set(loadpath ${CMAKE_MODULE_PATH})
    list(APPEND loadpath ${PA_MODULES})
    set(CMAKE_MODULE_PATH ${loadpath} PARENT_SCOPE)
  endif()
endfunction()

macro(ufw_toplevel)
  __ufw_toplevel_args(${ARGV})
  include(HardwareAbstraction)
  include(CTest)
  if (DEFINED UFW_ZEPHYR_KERNEL)
    message(STATUS "ufw: Enabling Zephyr Kernel ${UFW_ZEPHYR_KERNEL}")
    set(APPLICATION_SOURCE_DIR ${UFW_ZEPHYR_APPLICATION} CACHE PATH "Application Source Directory")
    # This is, so CMake's find_package will find the right place
    # for ZephyrConfig.cmake:
    set(Zephyr_ROOT "${UFW_ZEPHYR_KERNEL}")
    # This is, so Zephyr's CMake magic doesn't pick up other zephyr
    # kernels from other prefixes such as ~/src.
    set(ZEPHYR_BASE "${UFW_ZEPHYR_KERNEL}")
    # There is some code in Zephyr (in samples, for instance), that uses
    # ZEPHYR_BASE from the execution environment to address some source files,
    # and if it is not set, that'll break, obviously. Personally, I think they
    # should be using ${ZEPHYR_BASE} everywhere to figure out file names. But
    # there is zephyr-env.sh in the repository root that would set this envi-
    # ronment variable, so maybe that's intentional. At any rate, it is easy to
    # set the environment variable here as well. But keep in mind, that in
    # CMake setting $ENV{...} is only active in the configure phase. It does
    # not affect later compilation, testing, or installation invocations. We
    # are also only doing this if the value is not set yet, so the impact
    # should be minimal.
    if (NOT DEFINED ENV{ZEPHYR_BASE})
      message(STATUS "ufw: Setting \$ENV{ZEPHYR_BASE} to match \${ZEPHYR_BASE}")
      set(ENV{ZEPHYR_BASE} ${ZEPHYR_BASE})
    endif()
    find_package(Zephyr REQUIRED)
  endif()
endmacro()

function(setup_ufw)
  if (UFW_SETUP_DONE)
    return()
  endif()
  cmake_parse_arguments(PA "FORCE_INCLUDE" "" "" ${ARGN})
  list(LENGTH PA_UNPARSED_ARGUMENTS argc)
  if (${argc} GREATER 1)
      message(FATAL_ERROR "setup_ufw used with too many arguments!")
  endif()
  if (MICROFRAMEWORK_ROOT)
    set(_dir "${MICROFRAMEWORK_ROOT}")
  else()
    if (NOT (${argc} EQUAL 1))
      message(FATAL_ERROR "setup_ufw used without ufw root directory!")
    endif()
    list(GET PA_UNPARSED_ARGUMENTS 0 _dir)
    set(_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}")
    if (NOT IS_DIRECTORY "${_dir}")
      message(FATAL_ERROR
        "Specified MICROFRAMEWORK_ROOT does not exist: ${_dir}")
    endif()
    set(MICROFRAMEWORK_ROOT "${_dir}" PARENT_SCOPE)
  endif()
  set(UFW_SETUP_DONE True PARENT_SCOPE)
  if ((NOT DEFINED ZEPHYR_BASE) OR PA_FORCE_INCLUDE)
    add_subdirectory(${_dir})
  endif()
endfunction()
