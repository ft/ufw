# Copyright (c) 2020-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_ToolchainFeatures)
  return()
endif()
set(__UFW_ToolchainFeatures 1)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

macro(__ufw_parse_toolchain_features var)
  set(__multi COMPILER_OPTIONS LINKER_OPTIONS LINKER_LIBRARIES)
  cmake_parse_arguments(PA "" "" "${__multi}" ${${_var}})
endmacro()

function(__ufw_check_toolchain_compiler_call lang var)
  __ufw_parse_toolchain_features(${var})

  set(CMAKE_REQUIRED_LINK_OPTIONS ${PA_LINK_OPTIONS})
  set(CMAKE_REQUIRED_LIBRARIES ${PA_LINK_LIBRARIES})
  set(CMAKE_REQUIRED_FLAGS ${PA_COMPILER_OPTIONS})

  if (${lang} STREQUAL C)
    check_c_source_compiles("int main(void) { return 0; }" UFW_${var})
  elseif (${lang} STREQUAL CXX)
    check_cxx_source_compiles("int main(void) { return 0; }" UFW_${var})
  else()
    message(FATAL_ERROR "Unknown toolchain language: ${lang}")
  endif()
endfunction()

function(__ufw_feature_to_variable feature outvar)
  set(_var ${feature})
  string(REPLACE - _ _var ${_var})
  string(TOUPPER ${_var} _var)
  set(${outvar} TOOLCHAIN_FEATURE_${_var} PARENT_SCOPE)
endfunction()

function(ufw_check_toolchain_feature lang feature outvar)
  # TOOLCHAIN_FEATURES lists the extensions a toolchain supports. To figure out
  # how to enable a feature foo-bar, the TOOLCHAIN_FEATURE_FOO_BAR variable is
  # consulted.
  __ufw_feature_to_variable(${feature} _var)
  __ufw_check_toolchain_compiler_call(${lang} ${_var})
  if (NOT UFW_${_var})
    set(${outvar} 0 PARENT_SCOPE)
  else()
    set(${outvar} 1 PARENT_SCOPE)
  endif()
endfunction()

function(ufw_check_toolchain_features_for lang)
  set(_features ${TOOLCHAIN_FEATURES})
  foreach (feature ${TOOLCHAIN_FEATURES})
    ufw_check_toolchain_feature(${lang} ${feature} success)
    if (${success} EQUAL 0)
      list(REMOVE_ITEM _features ${feature})
    endif()
  endforeach()
  set(TOOLCHAIN_FEATURES ${_features} PARENT_SCOPE)
endfunction()

function(ufw_check_toolchain_features with_c_lang with_cxx_lang)
  if (${with_c_lang} GREATER_EQUAL 0)
    ufw_check_toolchain_features_for(C)
  elseif (${with_cxx_lang} GREATER_EQUAL 0)
    ufw_check_toolchain_features_for(CXX)
  else()
    message(STATUS
      "Neither C not C++ are active, not testing toolchain features")
  endif()
  set(TOOLCHAIN_FEATURES ${_features} PARENT_SCOPE)
endfunction()

function(__ufw_target_enable_feature target feature)
  __ufw_feature_to_variable(${feature} _var)
  if (NOT UFW_${_var})
    if (TOOLCHAIN_FEATURES_VERBOSE)
      message(STATUS "Feature ${feature} requested, but UFW_${_var} is unset.")
    endif()
    return()
  endif()
  __ufw_parse_toolchain_features(${_var})
  if (PA_COMPILER_OPTIONS)
    target_compile_options(${target} PUBLIC ${PA_COMPILER_OPTIONS})
  endif()
  if (PA_LINKER_OPTIONS)
    # Stuck on old stuff at work...
    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13.0")
      target_link_options(${target} PUBLIC ${PA_LINKER_OPTIONS})
    else()
      target_link_libraries(${target} PUBLIC ${PA_LINKER_OPTIONS})
    endif()
  endif()
  if (PA_LINKER_LIBRARIES)
    target_link_libraries(${target} PUBLIC ${PA_LINKER_LIBRARIES})
  endif()
endfunction()

function(ufw_toolchain target)
  cmake_parse_arguments(PA "" "" "FEATURES" ${ARGN})
  foreach (feature ${PA_FEATURES})
    __ufw_target_enable_feature(${target} ${feature})
  endforeach()
endfunction()

function(ufw_force_compat outvar)
  # With zephr's minimal libc, cmake's test for existing symbols will probably
  # fail, because the test will likely use newlib (at least on ARM) which im-
  # plements these, which leads to false positives.
  #
  # With ti-arm-clang, the detection also doesn't work properly at the moment.
  # So force inclusion for that toolchain as well.
  if (CONFIG_MINIMAL_LIBC AND "${CONFIG_MINIMAL_LIBC}" STREQUAL y)
    set(${outvar} 1 PARENT_SCOPE)
    message(STATUS "ufw: Zephyr minimal libc: Forcing compat code inclusion")
  elseif ("${TOOLCHAIN_ID}" STREQUAL ti-arm-clang)
    set(${outvar} 1 PARENT_SCOPE)
    message(STATUS "ufw: ${TOOLCHAIN_ID}: Forcing compat code inclusion")
  else()
    set(${outvar} 0 PARENT_SCOPE)
  endif()
endfunction()
