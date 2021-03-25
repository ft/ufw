if(__UFW_SetupTargetCPU)
  return()
endif()
set(__UFW_SetupTargetCPU 1)

include(TestBigEndian)

function(add_target_link_flags _target _link_flags)
  set_property(TARGET ${_target} APPEND_STRING PROPERTY LINK_FLAGS " ${_link_flags}")
endfunction()

function(add_target_endianness target endianness)
  target_compile_definitions(${target} PUBLIC "SYSTEM_ENDIANNESS_${endianness}")
endfunction()

function(get_library_dir lib arch_flags result)
  find_program(gcc arm-none-eabi-gcc
    HINTS ${TOOLCHAIN_PATH} ENV GNU_ARM_NONE_EABI_TOOLCHAIN_PATH REQUIRED)

  execute_process(COMMAND ${gcc} ${arch_flags} -print-file-name=${lib}
    OUTPUT_VARIABLE lib_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  get_filename_component(lib_dir "${lib_path}" DIRECTORY)

  set("${result}" "${lib_dir}" PARENT_SCOPE)
endfunction()

function(set_target_cpu_gcc_arm target _cpu)
  add_target_endianness(${target} LITTLE)
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

  if ((CMAKE_C_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    get_library_dir(libc.a "${_flags}" libc_path)
    get_library_dir(libm.a "${_flags}" libm_path)
    get_library_dir(libgcc.a "${_flags}" libgcc_path)
    add_target_link_flags(${target} "-Wl,-L${libc_path} -Wl,-L${libm_path} -Wl,-L${libgcc_path}")
  endif()
endfunction()

function(set_target_cpu_clang_arm target cpu)
  set_target_cpu_gcc_arm(${target} ${cpu})
endfunction()

function(set_target_cpu_ti_arm target _cpu)
  add_target_endianness(${target} LITTLE)
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
  add_target_endianness(${target} LITTLE)
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

function(set_target_cpu_native target _cpu)
  if (NOT (DEFINED HOST_BIG_ENDIAN))
    test_big_endian(HOST_BIG_ENDIAN)
    set(HOST_BIG_ENDIAN ${HOST_BIG_ENDIAN} PARENT_SCOPE)
  endif()
  if (HOST_BIG_ENDIAN EQUAL 0)
    add_target_endianness(${target} LITTLE)
  else()
    add_target_endianness(${target} BIG)
  endif()
endfunction()

function(set_target_cpu target)
  if (ZEPHYR_TOOLCHAIN_VARIANT)
    target_compile_options(${target} PUBLIC ${TOOLCHAIN_C_FLAGS})
    target_link_libraries(${target} PUBLIC ${TOOLCHAIN_LD_FLAGS})
    return()
  endif()
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
  elseif ("${TOOLCHAIN_ID}" STREQUAL "clang-arm")
    set_target_cpu_clang_arm("${target}" "${PROJECT_TARGET_CPU}")
  elseif ("${TOOLCHAIN_ID}" STREQUAL "ti-arm")
    set_target_cpu_ti_arm("${target}" "${PROJECT_TARGET_CPU}")
  elseif ("${TOOLCHAIN_ID}" STREQUAL "ti-c2000")
    set_target_cpu_ti_c28x("${target}" "${PROJECT_TARGET_CPU}")
  elseif ("${TOOLCHAIN_ID}" STREQUAL "gcc-native")
    set_target_cpu_native("${target}" "${PROJECT_TARGET_CPU}")
  elseif ("${TOOLCHAIN_ID}" STREQUAL "clang-native")
    set_target_cpu_native("${target}" "${PROJECT_TARGET_CPU}")
  else()
    message(WARNING "-- set_target_cpu: Unknown TOOLCHAIN_ID ${TOOLCHAIN_ID}")
  endif()
endfunction()
