if(__UFW_HardwareAbstraction)
  return()
endif()
set(__UFW_HardwareAbstraction 1)

include(SetupTargetCPU)
include(UFWTools)

function(__ufw_board_ensure target)
  if (TARGET ${target})
    ufw_get_property(${target} BOARD_NAME name)
    message(FATAL_ERROR "${target} already set to ${name}!")
  endif()
endfunction()

function(ufw_pick_board name)
  message(STATUS "Setting target board: ${name}")
  __ufw_board_ensure(board-interface)
  __ufw_board_ensure(board-objects)
  add_library(board-interface ALIAS board-${name}-interface)
  add_library(board-objects   ALIAS board-${name}-objects)
  ufw_get_property(board-objects BOARD_ARCHITECTURE cpu)
  message(STATUS "Architecture from board definition: ${cpu}")
  set(PROJECT_TARGET_CPU ${cpu})
  set(PROJECT_TARGET_CPU ${cpu} PARENT_SCOPE)
  set_target_cpu(board-${name}-objects)
endfunction()

function(define_board name)
  set(__multi_args
    SOURCES LIBRARIES
    INCLUDE SYSTEM_INCLUDE
    DEFINITIONS
    ARCHITECTURE CPUFAMILY CPUHANDLE CPUVENDOR)
  cmake_parse_arguments(PA "" "" "${__multi_args}" ${ARGN})

  set(interface "board-${name}-interface")
  set(objects   "board-${name}-objects")

  add_library(${interface} INTERFACE)
  if (PA_INCLUDE)
    target_include_directories(${interface} INTERFACE ${PA_INCLUDE})
  endif()
  if (PA_SYSTEM_INCLUDE)
    target_include_directories(${interface} SYSTEM INTERFACE ${PA_SYSTEM_INCLUDE})
  endif()
  if (PA_DEFINITIONS)
    target_compile_definitions(${interface} INTERFACE ${PA_DEFINITIONS})
  endif()

  add_library(${objects} OBJECT EXCLUDE_FROM_ALL ${PA_SOURCES})
  target_include_directories(${objects} PUBLIC
    $<TARGET_PROPERTY:${interface},INTERFACE_INCLUDE_DIRECTORIES>)
  target_include_directories(${objects} SYSTEM PUBLIC
    $<TARGET_PROPERTY:${interface},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>)
  target_compile_definitions(${objects} PUBLIC
    $<TARGET_PROPERTY:${interface},INTERFACE_COMPILE_DEFINITIONS>)
  target_link_libraries(${objects} PUBLIC ${PA_LIBRARIES})

  ufw_set_property(${objects} BOARD_NAME ${name})
  if (PA_ARCHITECTURE)
    ufw_set_property(${objects} BOARD_ARCHITECTURE ${PA_ARCHITECTURE})
  endif()
  if (PA_CPUFAMILY)
    ufw_set_property(${objects} BOARD_CPU_FAMILY ${PA_CPUFAMILY})
  endif()
  if (PA_CPUHANDLE)
    ufw_set_property(${objects} BOARD_CPU_HANDLE ${PA_CPUHANDLE})
  endif()
  if (PA_CPUVENDOR)
    ufw_set_property(${objects} BOARD_CPU_VENDOR ${PA_CPUVENDOR})
  endif()
endfunction(define_board)
