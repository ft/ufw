# Copyright (c) 2020-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_STM32HAL)
  return()
endif()
set(__UFW_STM32HAL 1)

include(SetupTargetCPU)

function(add_stm32_hal root)
  set(__UFW_ROOT_STM32_HAL ${root} PARENT_SCOPE)
endfunction()

function(__ufw_stm32_hal_gotroot)
  if (NOT __UFW_ROOT_STM32_HAL)
    message(FATAL_ERROR "Use add_stm32_hal() to configure HAL location")
  endif()
endfunction()

macro(__ufw_use_stm32_hal kind name suffix)
  cmake_parse_arguments(PA "" "" "EXCLUDE" ${ARGN})
  __ufw_stm32_hal_gotroot()
  ufw_get_property(board-objects BOARD_CPU_FAMILY _cpu)
  include(STM32HAL_${_cpu})
  if (NOT __UFW_STM32HAL_${_cpu}_${kind}_SOURCES)
    message(WARNING "No sources for STM32 ${name} for ${_cpu} found")
  endif()
  set(library stm32-hal-${suffix}-${_cpu})
  set(orig ${__UFW_STM32HAL_${_cpu}_${kind}_SOURCES})
  if (PA_EXCLUDE)
    list(LENGTH PA_EXCLUDE nexclude)
    message(STATUS
      "Removing ${nexclude} source file(s) from STM32HAL-${kind} as requested.")
    list(REMOVE_ITEM orig ${PA_EXCLUDE})
  endif()
  ufw_prefix(sources ${__UFW_ROOT_STM32_HAL}/Src/ ${orig})
  ufw_filter_nonexistent("${sources}" sources "${library}")
  add_library(${library} STATIC ${sources})
  target_include_directories(${library} PUBLIC ${__UFW_ROOT_STM32_HAL}/Inc)
  target_link_libraries(${library} PUBLIC board-config st-cmsis-${_cpu})
  if (${kind} STREQUAL HIGH)
    target_compile_definitions(${library} PUBLIC    USE_HAL_DRIVER)
    target_compile_definitions(${library} INTERFACE USE_HAL_DRIVER)
  elseif (${kind} STREQUAL LOW)
    target_compile_definitions(${library} PUBLIC    USE_FULL_LL_DRIVER)
    target_compile_definitions(${library} INTERFACE USE_FULL_LL_DRIVER)
  endif()
  set_target_cpu(${library})
endmacro()

function(use_stm32_hal)
  __ufw_use_stm32_hal(HIGH HAL high ${ARGV})
endfunction()

function(use_stm32_hal_lowlevel)
  __ufw_use_stm32_hal(LOW LowLevel low ${ARGV})
endfunction()

function(use_stm32_hal_legacy)
  __ufw_use_stm32_hal(LEGACY Legacy legacy ${ARGV})
endfunction()
