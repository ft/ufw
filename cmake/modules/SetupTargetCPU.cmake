if(__UFW_SetupTargetCPU)
  return()
endif()
set(__UFW_SetupTargetCPU 1)

function(add_target_link_flags _target _link_flags)
    set(new_link_flags ${_link_flags})
    get_target_property(existing_link_flags ${_target} LINK_FLAGS)
    if(existing_link_flags)
        set(new_link_flags "${new_link_flags} ${existing_link_flags}")
    else()
        message("No existing link flags found: ${existing_link_flags}")
    endif()
    set_target_properties(${_target} PROPERTIES LINK_FLAGS ${new_link_flags})
endfunction()

function(set_target_cpu_gcc_arm target _cpu)
  if (${_cpu} STREQUAL "cortex-m0")
    set(_flags -mthumb -mcpu=cortex-m0)
  elseif (${_cpu} STREQUAL "cortex-m0+")
    set(_flags -mthumb -mcpu=cortex-m0plus)
  elseif (${_cpu} STREQUAL "cortex-m1")
    set(_flags -mthumb -mcpu=cortex-m1)
  elseif (${_cpu} STREQUAL "cortex-m3")
    set(_flags -mthumb -mcpu=cortex-m3)
  elseif (${_cpu} STREQUAL "cortex-m4")
    set(_flags -mthumb -mcpu=cortex-m4)
  elseif (${_cpu} STREQUAL "cortex-m4-softfp")
    set(_flags -mthumb -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16)
  elseif (${_cpu} STREQUAL "cortex-m4-hardfp")
    set(_flags -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16)
  elseif (${_cpu} STREQUAL "cortex-m7")
    set(_flags -mthumb -mcpu=cortex-m7)
  elseif (${_cpu} STREQUAL "cortex-m7-softfp")
    set(_flags -mthumb -mcpu=cortex-m7 -mfloat-abi=softfp -mfpu=fpv5-sp-d16)
  elseif (${_cpu} STREQUAL "cortex-m7-hardfp")
    set(_flags -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16)
  else()
    message(WARNING "-- ${TOOLCHAIN_ID}: Unsupported PROJECT_TARGET_CPU: ${_cpu}")
  endif()

  target_compile_options(${target} PUBLIC ${_flags})
  string(REPLACE ";" " " _flags_str "${_flags}")
  set_target_properties(${target} PROPERTIES LINK_FLAGS "${_flags_str}")
endfunction()

function(set_target_cpu_ti_arm target _cpu)
  if (${_cpu} STREQUAL "cortex-m3")
    set(_flags --silicon_version=7M3 --little_endian --code_state=16)
  elseif (${_cpu} STREQUAL "cortex-m4f")
    set(_flags --silicon_version=7M4 --little_endian --code_state=16)
  else()
    message(WARNING "-- ${TOOLCHAIN_ID}: Unsupported PROJECT_TARGET_CPU: ${_cpu}")
  endif()

  target_compile_options(${target} PUBLIC ${_flags})
endfunction()

function(set_target_cpu_ti_c28x target _cpu)
  if (${_cpu} STREQUAL "c28x-float")
    set(_flags -v28
      --large_memory_model
      --unified_memory
      --float_support=fpu32
      --tmu_support=tmu0
      --define=CPU1
      --define=_TMS320C28XX_TMU0__)
  else()
    message(WARNING "-- ${TOOLCHAIN_ID}: Unsupported PROJECT_TARGET_CPU: ${_cpu}")
  endif()

  target_compile_options(${target} PUBLIC ${_flags})
endfunction()

function(set_target_cpu target)
  if (NOT PROJECT_TARGET_CPU)
    # This needs to be set in the build-tree's board file.
    message(FATAL_ERROR "-- PROJECT_TARGET_CPU is not set! Giving up.")
    return()
  endif()
  if (NOT TOOLCHAIN_ID)
    # This one is set in our cmake-embedded toolchain files.
    message(FATAL_ERROR "-- TOOLCHAIN_ID is not set! Giving up.")
    return()
  endif()
  if (NOT COMPILER_API)
    # This one comes from the toolchain files too.
    message(FATAL_ERROR "-- COMPILER_API is not set! Giving up.")
    return()
  endif()

  if ("${TOOLCHAIN_ID}" STREQUAL "gcc-arm")
    set_target_cpu_gcc_arm("${target}" "${PROJECT_TARGET_CPU}")
  elseif ("${TOOLCHAIN_ID}" STREQUAL "ti-arm")
    set_target_cpu_ti_arm("${target}" "${PROJECT_TARGET_CPU}")
  elseif ("${TOOLCHAIN_ID}" STREQUAL "ti-c2000")
    set_target_cpu_ti_c28x("${target}" "${PROJECT_TARGET_CPU}")
  elseif ("${TOOLCHAIN_ID}" STREQUAL "gcc-native")
    # Nothing
  elseif ("${TOOLCHAIN_ID}" STREQUAL "clang-native")
    # Nothing
  else()
    message(WARNING "-- set_target_cpu: Unknown TOOLCHAIN_ID ${TOOLCHAIN_ID}")
  endif()
endfunction()
