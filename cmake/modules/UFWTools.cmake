# Copyright (c) 2020-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_Tools)
  return()
endif()
set(__UFW_Tools 1)

define_property(
  TARGET PROPERTY UFW_BOARD_NAME
  BRIEF_DOCS "Name of Embedded Board defined by target."
  FULL_DOCS "Name of Embedded Board defined by target.")

define_property(
  TARGET PROPERTY UFW_BOARD_CPU_FAMILY
  BRIEF_DOCS "Family of the CPU used by defined board."
  FULL_DOCS "Family of the CPU used by defined board.")

define_property(
  TARGET PROPERTY UFW_BOARD_CPU_HANDLE
  BRIEF_DOCS "Full name of CPU used by defined board."
  FULL_DOCS "Full name of CPU used by defined board.")

define_property(
  TARGET PROPERTY UFW_BOARD_CPU_VENDOR
  BRIEF_DOCS "Name of vendor of CPU used by defined board."
  FULL_DOCS "Name of vendor of CPU used by defined board.")

define_property(
  TARGET PROPERTY UFW_BOARD_ARCHITECTURE
  BRIEF_DOCS "Architecture of CPU used by defined board."
  FULL_DOCS "Architecture of CPU used by defined board.")

define_property(
  TARGET PROPERTY UFW_BOARD_DEFAULT_LINKERSCRIPT
  BRIEF_DOCS "Linkerscript used for defined board."
  FULL_DOCS "Linkerscript used for defined board.")

# The default value of this is for backward compatibility. This may change to
# "ufw-install" at another major release.
set(UFW_INSTALL_COMPONENT ufw-git-install CACHE STRING
  "CMake Installation Component name for ufw installation items.")

function(ufw_install)
  install(${ARGV} COMPONENT ${UFW_INSTALL_COMPONENT})
endfunction()

function(ufw_set_property target prop value)
  set_target_properties(${target} PROPERTIES UFW_${prop} ${value})
endfunction()

function(ufw_get_property target prop variable)
  cmake_parse_arguments(PA "" "ERROR" "" ${ARGN})
  if (NOT TARGET ${target})
    message(WARNING "ufw_get_property: No such target: ${target}")
  endif()
  get_target_property(value ${target} UFW_${prop})
  if (${value} STREQUAL value-NOTFOUND)
    if (PA_ERROR)
      set(${PA_ERROR} 1 PARENT_SCOPE)
    else()
      message(WARNING "Could not find property ${prop} in target ${target}")
    endif()
  endif()
  set(${variable} ${value} PARENT_SCOPE)
endfunction()

function(ufw_add_library name include sources)
  include(InitialiseToolchain)
  include(SetupTargetCPU)
  initialise_toolchain()
  add_library(${name} STATIC ${sources})
  target_include_directories(${name} PUBLIC ${include})
  set_target_cpu(${name})
endfunction()

function(ufw_prefix var prefix)
   set(rv)
   foreach(f ${ARGN})
      list(APPEND rv ${prefix}${f})
   endforeach()
   set(${var} ${rv} PARENT_SCOPE)
endfunction()

function(ufw_split_pair pair delim key value)
  string(FIND "${pair}" "${delim}" splitidx)
  string(SUBSTRING "${pair}" 0 ${splitidx} tmp)
  set(${key} ${tmp} PARENT_SCOPE)
  math(EXPR next ${splitidx}+1)
  string(SUBSTRING "${pair}" ${next} -1 tmp)
  set(${value} ${tmp} PARENT_SCOPE)
endfunction()

function(ufw_filter_nonexistent files var msgprefix)
  set(rv)
  foreach(f ${files})
    set(full "${f}")
    if (NOT IS_ABSOLUTE "${full}")
      set(full "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
    endif()
    if (EXISTS ${full})
      list(APPEND rv ${f})
    else()
      message(STATUS "${msgprefix}: Missing file: ${f}")
    endif()
  endforeach()
  set(${var} ${rv} PARENT_SCOPE)
endfunction()
