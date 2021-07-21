if(__UFW_BuildInZephyrDir)
  return()
endif()
set(__UFW_BuildInZephyrDir 1)

function(ufw_check_zephyr_kernel location retval)
  set(${retval} False PARENT_SCOPE)
  if (NOT IS_DIRECTORY ${location})
    return()
  endif()
  if (NOT EXISTS "${location}/Kconfig.zephyr")
    return()
  endif()
  set(${retval} True PARENT_SCOPE)
endfunction()

function(ufw_check_zephyr_module root name retval)
  set(${retval} False PARENT_SCOPE)
  if (NOT IS_DIRECTORY ${root})
    message(WARNING "MODULE_ROOT does not exist: ${root}")
    return()
  endif()
  if (NOT IS_DIRECTORY "${root}/${name}")
    message(WARNING "MODULE does not exist: ${name}")
    return()
  endif()
  if (NOT IS_DIRECTORY "${root}/${name}/zephyr")
    message(WARNING "MODULE (${name}) does not exist: ${root}")
    return()
  endif()
  set(${retval} True PARENT_SCOPE)
endfunction()

function(ufw_add_stringlist lst value)
  if ("${${lst}}" STREQUAL "")
    set("${lst}" "${${lst}}${value}" PARENT_SCOPE)
  else()
    set("${lst}" "${${lst}}$<SEMICOLON>${value}" PARENT_SCOPE)
  endif()
endfunction()

function(ufw_zephyr_modules params root modules)
  set(_fails 0)
  list(LENGTH modules _count)
  if (${_count} EQUAL 0)
    set(_return)
  else()
    set(_return "")
    foreach (mod ${modules})
      ufw_add_stringlist(_return "${root}/${mod}")
    endforeach()
  endif()
  set(${params} "ZEPHYR_MODULES=${_return}" PARENT_SCOPE)
  if (${UFW_ZEPHYR_DEBUG})
    message(STATUS "Zephyr modules argument: ${_return}")
  endif()
endfunction()

function(ufw_parse_toolchain_zephyr str name root)
  string(FIND "${str}" ":" splitidx)
  string(SUBSTRING "${str}" 0 ${splitidx} tmp)
  set(${name} ${tmp} PARENT_SCOPE)
  if (${UFW_ZEPHYR_DEBUG})
    message(STATUS "Toolchain name: ${tmp}")
  endif()
  if (NOT "${tmp}" STREQUAL "${str}")
    math(EXPR next ${splitidx}+1)
    string(SUBSTRING "${str}" ${next} -1 tmp)
    set(${root} ${tmp} PARENT_SCOPE)
    if (${UFW_ZEPHYR_DEBUG})
      message(STATUS "Toolchain root: ${tmp}")
    endif()
  else()
    set(${root} "" PARENT_SCOPE)
  endif()
endfunction()

function(ufw_zephyr_toolchain name root params)
  set(_return ZEPHYR_TOOLCHAIN_VARIANT=${name})
  if (${name} STREQUAL gnuarmemb)
    if (NOT ("${root}" STREQUAL ""))
      list(APPEND _return GNUARMEMB_TOOLCHAIN_PATH=${root})
    endif()
  endif()
  set(${params} "${_return}" PARENT_SCOPE)
endfunction()

function(ufw_zephyr_make_dir dir)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory "${dir}"
    RESULT_VARIABLE error_code)
  if (error_code)
    message(FATAL_ERROR "Failed to create directory: ${dir}")
  endif()
endfunction()

function(ufw_zephyr_change_dir dir)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E chdir "${dir}"
    RESULT_VARIABLE error_code)
  if (error_code)
    message(FATAL_ERROR "Failed to change into directory: ${dir}")
  endif()
endfunction()

function(ufw_zephyr_configure dir)
  set(cmd
    ${CMAKE_COMMAND} -E chdir ${builddir} ${CMAKE_COMMAND}
    -G${CMAKE_GENERATOR}
    ${__cmake_args__}
    -DCMAKE_INSTALL_PREFIX=${__install_prefix__}
    -DCMAKE_BUILD_TYPE=${PA_BUILDCFG}
    -DUFW_ZEPHYR_APPLICATION=${PA_APPLICATION}
    -DTARGET_BOARD=${PA_BOARD}
    -DUFW_RECURSIVE_RUN=1
    -DCMAKE_EXPORT_COMPILE_COMMANDS=on
    ${PROJECT_SOURCE_DIR})
  execute_process(COMMAND ${cmd} RESULT_VARIABLE error_code)
  if (error_code)
    message(FATAL_ERROR "Failed to configure ${builddir}")
  endif()
endfunction()

function(ufw_zephyr_build dir)
  set(cmd ${CMAKE_COMMAND} --build ${dir})
  execute_process(COMMAND ${cmd} RESULT_VARIABLE error_code)
  if (error_code)
    message(FATAL_ERROR "Failed to build ${dir}")
  endif()
endfunction()

function(ufw_zephyr_test dir)
  set(cmd ${CMAKE_COMMAND} -E chdir ${dir} ctest .)
  execute_process(COMMAND ${cmd} RESULT_VARIABLE error_code)
  if (error_code)
    message(FATAL_ERROR "Failed to run tests for ${dir}")
  endif()
endfunction()

function(ufw_zephyr_build_config name file has_mapping)
  set(kconfigdir "${MICROFRAMEWORK_ROOT}/cmake/kconfig")
  string(TOLOWER "${name}" _name)
  set(_file "${kconfigdir}/${_name}.conf")
  if (${UFW_ZEPHYR_DEBUG})
    message(STATUS "Build config mapping candidate: ${_file}")
  endif()
  if (EXISTS "${_file}")
    set(${file} "${_file}" PARENT_SCOPE)
    set(${has_mapping} True PARENT_SCOPE)
    return()
  endif()
  set(${has_mapping} False PARENT_SCOPE)
endfunction()

function(ufw_zephyr_add_kconfig lst)
  set(_lst "${${lst}}")
  foreach (file ${ARGN})
    ufw_add_stringlist(_lst "${file}")
  endforeach()
  set(${lst} ${_lst} PARENT_SCOPE)
endfunction()

function(ufw_build_zephyr)
  set(__single_args APPLICATION BOARD BUILDCFG KERNEL MODULE_ROOT ROOT TOOLCHAIN)
  set(__multi_args KCONFIG MODULES OPTIONS)
  cmake_parse_arguments(PA "" "${__single_args}" "${__multi_args}" ${ARGN})
  if (${UFW_ZEPHYR_DEBUG})
    message(STATUS "Application: ${PA_APPLICATION}")
    message(STATUS "Board: ${PA_BOARD}")
    message(STATUS "Kernel: ${PA_KERNEL}")
    message(STATUS "Build type: ${PA_BUILDCFG}")
  endif()

  ufw_parse_toolchain_zephyr("${PA_TOOLCHAIN}" tc_name tc_root)
  ufw_zephyr_toolchain("${tc_name}" "${tc_root}" tc_args)
  ufw_prefix(tc_args "-D" ${tc_args})
  ufw_zephyr_modules(module_params "${PA_MODULE_ROOT}" "${PA_MODULES}")
  ufw_prefix(module_params "-D" ${module_params})

  set(builddir
    "${CMAKE_BINARY_DIR}/zephyr-${PA_APPLICATION}/${PA_BOARD}/${tc_name}/${PA_BUILDCFG}")
  if (${UFW_ZEPHYR_DEBUG})
    message(STATUS "Build directory: ${builddir}")
  endif()

  set(__install_prefix__
    "${UFW_ARTIFACTS_DIRECTORY}/${PA_BOARD}/${tc_name}/${PA_BUILDCFG}")
  ufw_prefix(__cmake_args__ "-D" ${PA_OPTIONS})

  ufw_zephyr_build_config("${PA_BUILDCFG}" buildkconfig has_mapping)
  if (has_mapping)
    if (${UFW_ZEPHYR_DEBUG})
      message(STATUS "Zephyr build config mapping for ${PA_BUILDCFG}: ${buildkconfig}")
    endif()
    ufw_zephyr_add_kconfig(kconfig_files "${buildkconfig}")
  else()
    message(STATUS "Zephyr no build config mapping for: ${PA_BUILDCFG}")
  endif()

  list(LENGTH PA_KCONFIG _count)
  if (_count EQUAL 1)
    if (NOT ("${PA_KCONFIG}" STREQUAL ""))
      ufw_zephyr_add_kconfig(kconfig_files ${PA_KCONFIG})
    endif()
  elseif (_count GREATER 0)
    ufw_zephyr_add_kconfig(kconfig_files ${PA_KCONFIG})
  endif()

  if (DEFINED kconfig_files)
    set(kconfig_files "-DOVERLAY_CONFIG=${kconfig_files}")
  endif()

  ExternalProject_Add("zephyr-${PA_APPLICATION}_${PA_BOARD}_${tc_name}_${PA_BUILDCFG}"
    PREFIX "zephyr-${PA_APPLICATION}/${PA_BOARD}/${tc_name}/${PA_BUILDCFG}"
    SOURCE_DIR "${PROJECT_SOURCE_DIR}"
    CMAKE_ARGS
    ${__cmake_args__}
    -DCMAKE_INSTALL_PREFIX=${__install_prefix__}
    -DUFW_RECURSIVE_RUN=1
    -DUFW_ZEPHYR_APPLICATION=${PA_APPLICATION}
    -DUFW_ZEPHYR_KERNEL=${PA_KERNEL}
    -DAPPLICATION_SOURCE_DIR=${PA_ROOT}
    -DBOARD=${PA_BOARD}
    ${tc_args}
    ${module_params}
    ${kconfig_files}
    -DCMAKE_EXPORT_COMPILE_COMMANDS=on
    -DUSE_CCACHE=0
    INSTALL_DIR ${__install_prefix__}
    BUILD_ALWAYS 1
    TEST_AFTER_INSTALL 1
    TEST_COMMAND ctest --output-on-failure)
endfunction()

function(ufw_setup_zephyr)
  set(__single_args APPLICATION KERNEL MODULE_ROOT ROOT)
  set(__multi_args BOARDS BUILDCFGS KCONFIG MODULES OPTIONS TOOLCHAINS)
  cmake_parse_arguments(PA "" "${__single_args}" "${__multi_args}" ${ARGN})

  list(GET PA_BOARDS 0 board)
  set(BOARD "${board}" PARENT_SCOPE)

  list(GET PA_TOOLCHAINS 0 tc)
  ufw_parse_toolchain_zephyr("${tc}" tc_name tc_root)
  ufw_zephyr_toolchain("${tc_name}" "${tc_root}" tc_args)
  foreach (pair ${tc_args})
    ufw_split_pair("${pair}" "=" key value)
    set("${key}" "${value}" PARENT_SCOPE)
  endforeach()

  ufw_zephyr_modules(module_params "${PA_MODULE_ROOT}" "${PA_MODULES}")
  foreach (pair ${module_params})
    ufw_split_pair("${pair}" "=" key value)
    string(REPLACE "$<SEMICOLON>" ";" value "${value}")
    set("${key}" "${value}" PARENT_SCOPE)
  endforeach()

  list(GET PA_BUILDCFGS 0 cfg)
  ufw_zephyr_build_config("${cfg}" buildkconfig has_mapping)
  if (has_mapping)
    if (${UFW_ZEPHYR_DEBUG})
      message(STATUS "Zephyr build config mapping for ${cfg}: ${buildkconfig}")
    endif()
    ufw_zephyr_add_kconfig(kconfig_files "${buildkconfig}")
  else()
    message(STATUS "Zephyr no build config mapping for: ${cfg}")
  endif()

  list(LENGTH PA_KCONFIG _count)
  if (_count EQUAL 1)
    if (NOT ("${PA_KCONFIG}" STREQUAL ""))
      ufw_zephyr_add_kconfig(kconfig_files ${PA_KCONFIG})
    endif()
  elseif (_count GREATER 0)
    ufw_zephyr_add_kconfig(kconfig_files ${PA_KCONFIG})
  endif()

  if (DEFINED kconfig_files)
    string(REPLACE "$<SEMICOLON>" ";" kconfig_files "${kconfig_files}")
    set(OVERLAY_CONFIG "${kconfig_files}" PARENT_SCOPE)
  endif()

  set(UFW_ZEPHYR_APPLICATION "${PA_APPLICATION}" PARENT_SCOPE)
  set(UFW_ZEPHYR_KERNEL "${PA_KERNEL}" PARENT_SCOPE)
  set(APPLICATION_SOURCE_DIR "${PA_ROOT}" PARENT_SCOPE)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/artifacts" PARENT_SCOPE)
  set(CMAKE_EXPORT_COMPILE_COMMANDS True PARENT_SCOPE)
  set(USE_CCACHE 0 PARENT_SCOPE)
endfunction()
