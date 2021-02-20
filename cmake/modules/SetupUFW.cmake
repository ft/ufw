if(__UFW_SetupUFW)
  return()
endif()
set(__UFW_SetupUFW 1)

function(ufw_add_board name)
  cmake_parse_arguments(PA "" "BUILDSYSTEM" "TOOLCHAINS;BUILDTYPES;OPTIONS" ${ARGN})
  set(boards ${UFW_TOPLEVEL_BOARDS})
  list(FIND boards ${name} already_defined)
  if (${already_defined} GREATER_EQUAL 0)
    message(FATAL_ERROR "Board ${name} already added.")
  endif()
  list(APPEND boards ${name})
  set(UFW_TOPLEVEL_BOARDS ${boards} PARENT_SCOPE)
  if (NOT PA_TOOLCHAINS)
    message(FATAL_ERROR "Please define TOOLCHAINS for board ${name}")
  endif()
  if (NOT PA_BUILDTYPES)
    set(PA_BUILDTYPES Release Debug)
  endif()
  set(UFW_TOPLEVEL_BOARD_BUILDTYPES_${name} ${PA_BUILDTYPES} PARENT_SCOPE)
  set(UFW_TOPLEVEL_BOARD_TOOLCHAINS_${name} ${PA_TOOLCHAINS} PARENT_SCOPE)
  set(UFW_TOPLEVEL_BOARD_OPTIONS_${name} ${PA_OPTIONS} PARENT_SCOPE)
  if (PA_BUILDSYSTEM)
    set(UFW_TOPLEVEL_BOARD_BUILDSYSTEM_${name} ${PA_BUILDSYSTEM} PARENT_SCOPE)
  endif()
endfunction()

function(ufw_add_zephyr name)
  cmake_parse_arguments(PA
    ""
    "KERNEL;APPLICATION;BUILDSYSTEM;MODULE_ROOT"
    "BOARDS;BUILDTYPES;KCONFIG;MODULES;OPTIONS;TOOLCHAINS"
    ${ARGN})
  if (NOT PA_KERNEL)
    set(PA_KERNEL ${CMAKE_SOURCE_DIR}/zephyr/kernel)
    message(STATUS "zephyr: KERNEL set to default ${PA_KERNEL}")
  endif()
  ufw_check_zephyr_kernel(${PA_KERNEL} zephyr_exists)
  if (NOT ${zephyr_exists})
    message(FATAL_ERROR "Zephyr Kernel not found in ${PA_KERNEL}")
  endif()
  if (NOT PA_TOOLCHAINS)
    set(PA_TOOLCHAINS gnumarmemb:/usr)
    message(STATUS "zephyr: TOOLCHAINS set to default ${PA_TOOLCHAINS}")
  endif()
  if (PA_MODULES AND (NOT PA_MODULE_ROOT))
    set(PA_MODULE_ROOT ${CMAKE_SOURCE_DIR}/zephyr/modules)
    message(STATUS "zephyr: MODULE_ROOT set to default ${PA_MODULE_ROOT}")
  endif()

  set(_count 0)
  foreach (module ${PA_MODULES})
    ufw_check_zephyr_module("${PA_MODULE_ROOT}" "${module}" module_is_valid)
    if (NOT module_is_valid)
      math(EXPR _count ${_count}+1)
      message(WARNING
        "Zephyr: Cannot find valid zephyr module (${module}) in ${PA_MODULE_ROOT}")
    endif()
  endforeach()
  if (${_count} GREATER 0)
      message(FATAL_ERROR "Zephyr: Failed to locate ${_count} module(s)!")
  endif()

  set(apps ${UFW_ZEPHYR_APPLICATIONS})
  list(FIND apps ${name} already_defined)
  if (${already_defined} GREATER_EQUAL 0)
    message(FATAL_ERROR "Application ${name} already added.")
  endif()
  list(APPEND apps ${name})
  set(UFW_ZEPHYR_APPLICATIONS ${apps} PARENT_SCOPE)

  set(UFW_ZEPHYR_APPLICATION_${name} ${PA_APPLICATION} PARENT_SCOPE)
  set(UFW_ZEPHYR_TOOLCHAINS_${name} ${PA_TOOLCHAINS} PARENT_SCOPE)
  set(UFW_ZEPHYR_BUILDTYPES_${name} ${PA_BUILDTYPES} PARENT_SCOPE)
  set(UFW_ZEPHYR_BOARDS_${name} ${PA_BOARDS} PARENT_SCOPE)

  set(UFW_ZEPHYR_KERNEL_${name} ${PA_KERNEL} PARENT_SCOPE)
  set(UFW_ZEPHYR_KCONFIG_${name} ${PA_KCONFIG} PARENT_SCOPE)
  set(UFW_ZEPHYR_OPTIONS_${name} ${PA_OPTIONS} PARENT_SCOPE)

  set(UFW_ZEPHYR_MODULE_ROOT_${name} ${PA_MODULE_ROOT} PARENT_SCOPE)
  set(UFW_ZEPHYR_MODULES_${name} ${PA_MODULES} PARENT_SCOPE)
  if (DEFINED PA_BUILDSYSTEM)
    set(UFW_TOPLEVEL_ZEPHYR_BUILDSYSTEM_${PA_APPLICATION} True)
  else()
    set(UFW_TOPLEVEL_ZEPHYR_BUILDSYSTEM_${PA_APPLICATION} False)
  endif()
endfunction()

macro(ufw_recursive_dispatch)
  if (NOT UFW_RECURSIVE_RUN)
    if (NOT ((DEFINED UFW_PICK_BOARD) OR (DEFINED UFW_PICK_ZEPHYR)))
      foreach (board ${UFW_TOPLEVEL_BOARDS})
        foreach (chain ${UFW_TOPLEVEL_BOARD_TOOLCHAINS_${board}})
          foreach (cfg ${UFW_TOPLEVEL_BOARD_BUILDTYPES_${board}})
            build_in_target_dir(
              BOARD ${board}
              OPTIONS "${UFW_TOPLEVEL_BOARD_OPTIONS_${board}}"
              TOOLCHAIN ${chain}
              BUILDCFG ${cfg})
          endforeach()
        endforeach()
      endforeach() # UFW_TOPLEVEL_BOARDS

      if (NOT DEFINED UFW_ZEPHYR_DEBUG)
        set(UFW_ZEPHYR_DEBUG False)
      endif()
      foreach (zapp ${UFW_ZEPHYR_APPLICATIONS})
        foreach (board ${UFW_ZEPHYR_BOARDS_${zapp}})
          foreach (chain ${UFW_ZEPHYR_TOOLCHAINS_${zapp}})
            foreach (cfg ${UFW_ZEPHYR_BUILDTYPES_${zapp}})
              ufw_build_zephyr(
                APPLICATION ${zapp}
                BOARD ${board}
                TOOLCHAIN ${chain}
                BUILDCFG ${cfg}
                ROOT "${UFW_ZEPHYR_APPLICATION_${zapp}}"
                KERNEL "${UFW_ZEPHYR_KERNEL_${zapp}}"
                KCONFIG "${UFW_ZEPHYR_KCONFIG_${zapp}}"
                OPTIONS "${UFW_ZEPHYR_OPTIONS_${zapp}}"
                MODULE_ROOT "${UFW_ZEPHYR_MODULE_ROOT_${zapp}}"
                MODULES "${UFW_ZEPHYR_MODULES_${zapp}}")
            endforeach()
          endforeach()
        endforeach()
      endforeach() # ZEPHYR_APPLICATIONS
      return()
    elseif (DEFINED UFW_PICK_ZEPHYR)
      ufw_setup_zephyr(
        APPLICATION ${UFW_PICK_ZEPHYR}
        BOARDS "${UFW_ZEPHYR_BOARDS_${UFW_PICK_ZEPHYR}}"
        TOOLCHAINS "${UFW_ZEPHYR_TOOLCHAINS_${UFW_PICK_ZEPHYR}}"
        BUILDCFGS "${UFW_ZEPHYR_BUILDTYPES_${UFW_PICK_ZEPHYR}}"
        ROOT "${UFW_ZEPHYR_APPLICATION_${UFW_PICK_ZEPHYR}}"
        KERNEL "${UFW_ZEPHYR_KERNEL_${UFW_PICK_ZEPHYR}}"
        KCONFIG "${UFW_ZEPHYR_KCONFIG_${UFW_PICK_ZEPHYR}}"
        OPTIONS "${UFW_ZEPHYR_OPTIONS_${UFW_PICK_ZEPHYR}}"
        MODULE_ROOT "${UFW_ZEPHYR_MODULE_ROOT_${UFW_PICK_ZEPHYR}}"
        MODULES "${UFW_ZEPHYR_MODULES_${UFW_PICK_ZEPHYR}}")
    endif() # UFW_PICK_*
  endif() # UFW_RECURSIVE_RUN
  if (DEFINED UFW_ZEPHYR_KERNEL)
    set(APPLICATION_SOURCE_DIR ${APPLICATION_SOURCE_DIR} CACHE PATH "Application Source Directory")
    set(Zephyr_ROOT "${UFW_ZEPHYR_KERNEL}")
    find_package(Zephyr REQUIRED)
  endif()
endmacro()

function(ufw_zephyr_dispatch fw)
  set(__subdir ${UFW_ZEPHYR_APPLICATION_${fw}})
  setup_ufw()
  add_subdirectory(${__subdir})
endfunction()

macro(ufw_subtree_build)
  if (UFW_TOPLEVEL_BOARD_BUILDSYSTEM_${TARGET_BOARD})
    include(${UFW_TOPLEVEL_BOARD_BUILDSYSTEM_${TARGET_BOARD}})
    return()
  endif()
  if (UFW_TOPLEVEL_ZEPHYR_BUILDSYSTEM_${UFW_ZEPHYR_APPLICATION})
    include(${UFW_TOPLEVEL_BOARD_BUILDSYSTEM_${TARGET_BOARD}})
    return()
  endif()
  if (UFW_ZEPHYR_APPLICATION)
    ufw_zephyr_dispatch("${UFW_ZEPHYR_APPLICATION}")
  endif()
endmacro()

function(__ufw_toplevel_args)
  cmake_parse_arguments(PA "" "ROOT;ARTIFACTS" "MODULES" ${ARGN})
  if (NOT PA_ARTIFACTS)
    message(FATAL_ERROR "ufw_toplevel: Please specify ARTIFACTS destination!")
  endif()
  if (NOT PA_ROOT)
    message(FATAL_ERROR "ufw_toplevel: Please specify UFW ROOT directory!")
  endif()
  set(EMBEDDED_CMAKE 1 PARENT_SCOPE)
  set(MICROFRAMEWORK_ROOT "${PA_ROOT}" PARENT_SCOPE)
  set(UFW_ARTIFACTS_DIRECTORY "${PA_ARTIFACTS}" PARENT_SCOPE)
  if (PA_MODULES)
    set(loadpath ${CMAKE_MODULE_PATH})
    list(APPEND loadpath ${PA_MODULES})
    set(CMAKE_MODULE_PATH ${loadpath} PARENT_SCOPE)
  endif()
endfunction()

macro(ufw_toplevel)
  __ufw_toplevel_args(${ARGV})
  include(BuildInTargetDir)
  include(BuildInZephyrDir)
  include(HardwareAbstraction)
  include(CTest)
endmacro()

function(setup_ufw)
  if (UFW_SETUP_DONE)
    return()
  endif()
  if (MICROFRAMEWORK_ROOT)
    set(_dir "${MICROFRAMEWORK_ROOT}")
  else()
    if (NOT ARGV0)
      message(FATAL_ERROR "setup_ufw used without ufw root directory!")
    endif()
    set(_dir "${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}")
    set(MICROFRAMEWORK_ROOT "${_dir}" PARENT_SCOPE)
  endif()
  set(UFW_SETUP_DONE True PARENT_SCOPE)
  add_subdirectory(${_dir})
endfunction()
