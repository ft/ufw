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
  BRIEF_DOCS "Name of Embedded Board defined by target."
  FULL_DOCS "Name of Embedded Board defined by target.")

define_property(
  TARGET PROPERTY UFW_BOARD_CPU_HANDLE
  BRIEF_DOCS "Name of Embedded Board defined by target."
  FULL_DOCS "Name of Embedded Board defined by target.")

define_property(
  TARGET PROPERTY UFW_BOARD_CPU_VENDOR
  BRIEF_DOCS "Name of Embedded Board defined by target."
  FULL_DOCS "Name of Embedded Board defined by target.")

define_property(
  TARGET PROPERTY UFW_BOARD_ARCHITECTURE
  BRIEF_DOCS "Name of Embedded Board defined by target."
  FULL_DOCS "Name of Embedded Board defined by target.")

define_property(
  TARGET PROPERTY UFW_BOARD_DEFAULT_LINKERSCRIPT
  BRIEF_DOCS "Default linker script to use with a board."
  FULL_DOCS "Default linker script to use with a board.")

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
      list(APPEND rv ${prefix}/${f})
   endforeach()
   set(${var} ${rv} PARENT_SCOPE)
endfunction()

function(ufw_filter_nonexistent files var msgprefix)
  set(rv)
  foreach(f ${files})
    if (EXISTS ${f})
      list(APPEND rv ${f})
    else()
      message(STATUS "${msgprefix}: Missing file: ${f}")
    endif()
  endforeach()
  set(${var} ${rv} PARENT_SCOPE)
endfunction()
