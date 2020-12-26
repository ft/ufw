if(__UFW_InitialiseToolchain)
  return()
endif()
set(__UFW_InitialiseToolchain 1)

include(GNUAttributes)
include(GNUBuiltins)
include(GNUCompilerWarnings)
include(TICompilerWarnings)
include(ToolchainFeatures)

function(ufw_toolchain_id_adjust compiler outvar)
  if (${compiler} STREQUAL gnu)
    set(compiler gcc)
  endif()
  if (ARCH AND (${ARCH} MATCHES posix))
    set(${outvar} "${compiler}-native" PARENT_SCOPE)
  elseif (ARCH AND (${ARCH} MATCHES arm))
    set(${outvar} "${compiler}-arm" PARENT_SCOPE)
  endif()
endfunction()

function(ufw_compiler_api_adjust outvar)
  if (COMPILER AND (${COMPILER} MATCHES gcc))
    set(${outvar} "gnu" PARENT_SCOPE)
  elseif (COMPILER AND (${COMPILER} MATCHES clang))
    set(${outvar} "gnu" PARENT_SCOPE)
  else()
    message(WARNING "ufw: Zephyr detected but unsupported toolchain!")
    message(WARNING "ufw: TOOLCHAIN: ${ZEPHYR_TOOLCHAIN_VARIANT}")
    message(WARNING "ufw: COMPILER: ${COMPILER}")
  endif()
endfunction()

function(ufw_status_zephyr var name)
  if (${var})
    message(STATUS "ufw: Zephyr ${name}: ${${var}}")
  else()
    message(WARNING "ufw: Zephyr detected but ${name} unset (${var})")
  endif()
endfunction()

function(ufw_toolchain_adjust_zephyr)
  ufw_status_zephyr(COMPILER Compiler)
  ufw_status_zephyr(LINKER Linker)
  ufw_status_zephyr(BINTOOLS Bintools)
  ufw_status_zephyr(ARCH Architecture)
  ufw_compiler_api_adjust(_compiler_api)
  ufw_toolchain_id_adjust("${_compiler_api}" _toolchain_id)
  set(COMPILER_API "${_compiler_api}" PARENT_SCOPE)
  set(TOOLCHAIN_ID "${_toolchain_id}" PARENT_SCOPE)
endfunction()

function(ufw_toolchain_ensure)
  if (NOT COMPILER_API)
    message(STATUS "COMPILER_API unset; assuming GNU")
    set(COMPILER_API gnu PARENT_SCOPE)
  endif()
  if (NOT TOOLCHAIN_ID)
    message(STATUS "TOOLCHAIN_ID unset; assuming gcc-native")
    set(TOOLCHAIN_ID gcc-native PARENT_SCOPE)
  endif()
  if (NOT PROJECT_TARGET_CPU)
    message(STATUS "PROJECT_TARGET_CPU unset; assuming native")
    set(PROJECT_TARGET_CPU native PARENT_SCOPE)
  endif()
endfunction()

function(ufw_toolchain_build_type_zephyr)
  if (CONFIG_SPEED_OPTIMIZATIONS AND CONFIG_SPEED_OPTIMIZATIONS STREQUAL "y")
    message(STATUS "ufw: Zephyr SPEED optimisation: Release")
    set(CMAKE_BUILD_TYPE Release PARENT_SCOPE)
  elseif (CONFIG_SIZE_OPTIMIZATIONS AND CONFIG_SIZE_OPTIMIZATIONS STREQUAL "y")
    message(STATUS "ufw: Zephyr SIZE optimisation: MinSizeRel")
    set(CMAKE_BUILD_TYPE MinSizeRel PARENT_SCOPE)
  elseif (CONFIG_DEBUG_OPTIMIZATIONS AND CONFIG_DEBUG_OPTIMIZATIONS STREQUAL "y")
    message(STATUS "ufw: Zephyr DEBUG optimisation: Debug")
    set(CMAKE_BUILD_TYPE Debug PARENT_SCOPE)
  elseif (CONFIG_NO_OPTIMIZATIONS AND CONFIG_NO_OPTIMIZATIONS STREQUAL "y")
    message(STATUS "ufw: No Zephyr optimisation: Defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug PARENT_SCOPE)
  else ()
    message(WARNING
      "ufw: Could not determine Zephyr optimisation: Defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug PARENT_SCOPE)
  endif()
endfunction()

macro(initialise_toolchain)

  if (NOT __UFW_ToolchainInitialised)
    if (NOT PROJECT_TARGET_CPU AND TARGET board-objects)
      ufw_get_property(board-objects BOARD_ARCHITECTURE PROJECT_TARGET_CPU)
    endif()
    if (ZEPHYR_TOOLCHAIN_VARIANT)
      ufw_toolchain_adjust_zephyr()
      if (NOT CMAKE_BUILD_TYPE)
        ufw_toolchain_build_type_zephyr()
      else()
        message(STATUS "ufw: CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
      endif()
    else()
      ufw_toolchain_ensure()
    endif()
    get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    list(FIND languages "CXX" with_cxx_lang)
    list(FIND languages "C" with_c_lang)
    set(_tag "InitToolchain")

    if ("${COMPILER_API}" STREQUAL "gnu")
      message(STATUS "${_tag}: Detected GNU-like Toolchain")
      if (${with_c_lang} GREATER_EQUAL 0)
        CheckAllGNUAttributes_C()
        CheckAllGNUBuiltins_C()
        GNUCCompilerWarnings()
        # Clang only supports -Og starting at version 4.0, so:
        check_c_compiler_flag(-Og UFW_CC_HAS_Og)
        set(CMAKE_C_FLAGS_DEBUG "-DDEBUG -O0 -ggdb")
        set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2")
        if (UFW_CC_HAS_Og)
          set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -Og -ggdb")
        else()
          set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -O1 -ggdb")
        endif()
        set(CMAKE_C_FLAGS_MINSIZEREL "-DNDEBUG -Os")
      endif()
      if (${with_cxx_lang} GREATER_EQUAL 0)
        CheckAllGNUAttributes_CXX()
        CheckAllGNUBuiltins_CXX()
        GNUCXXCompilerWarnings()
        check_cxx_compiler_flag(-Og UFW_CXX_HAS_Og)
        set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -O0 -ggdb")
        set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2")
        if (UFW_CXX_HAS_Og)
          set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -Og -ggdb")
        else()
          set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -O1 -ggdb")
        endif()
        set(CMAKE_CXX_FLAGS_MINSIZEREL "-DNDEBUG -Os")
      endif()
    elseif ("${COMPILER_API}" STREQUAL "ti")
      message(STATUS "${_tag}: Detected Texas Instruments Toolchain")
      if (${with_c_lang} GREATER_EQUAL 0)
        CheckAllGNUAttributes_C()
        CheckAllGNUBuiltins_C()
        set(CMAKE_C_FLAGS_DEBUG "--define=DEBUG -Ooff -g")
        set(CMAKE_C_FLAGS_RELEASE "--define=NDEBUG -O3 --opt_for_speed=4")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "--define=NDEBUG -O0 -g")
        set(CMAKE_C_FLAGS_MINSIZEREL "--define=NDEBUG -O3 --opt_for_speed=0")
      endif()
      if (${with_cxx_lang} GREATER_EQUAL 0)
        CheckAllGNUAttributes_CXX()
        CheckAllGNUBuiltins_CXX()
        set(CMAKE_CXX_FLAGS_DEBUG "--define=DEBUG -Ooff -g")
        set(CMAKE_CXX_FLAGS_RELEASE "--define=NDEBUG -O3 --opt_for_speed=4")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "--define=NDEBUG -O0 -g")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "--define=NDEBUG -O3 --opt_for_speed=0")
      endif()
    else()
      message(WARNING "${_tag}: Unknown COMPILER_API: ${COMPILER_API}")
    endif()

    ufw_check_toolchain_features(${with_c_lang} ${with_cxx_lang})
    set(__UFW_ToolchainInitialised 1)
  endif()

endmacro()

function(MakeStrictCompilerC target)
  if ("${COMPILER_API}" STREQUAL "gnu")
    GNUMakeStrictCCompiler(${target} ${ARGN})
  elseif ("${COMPILER_API}" STREQUAL "ti")
    TIMakeStrictCCompiler(${target} ${ARGN})
  else()
    message(WARNING "${_tag}: Unknown COMPILER_API: ${COMPILER_API}")
  endif()
endfunction()

function(MakeStrictCompilerCXX target)
  if ("${COMPILER_API}" STREQUAL "gnu")
    GNUMakeStrictCXXCompiler(${target} ${ARGN})
  elseif ("${COMPILER_API}" STREQUAL "ti")
    TIMakeStrictCXXCompiler(${target} ${ARGN})
  else()
    message(WARNING "${_tag}: Unknown COMPILER_API: ${CXXOMPILER_API}")
  endif()
endfunction()

function(MakeFatalCompilerC target)
  if ("${COMPILER_API}" STREQUAL "gnu")
    GNUMakeFatallyStrictCCompiler(${target} ${ARGN})
  elseif ("${COMPILER_API}" STREQUAL "ti")
    TIMakeFatallyStrictCCompiler(${target} ${ARGN})
  else()
    message(WARNING "${_tag}: Unknown COMPILER_API: ${COMPILER_API}")
  endif()
endfunction()

function(MakeFatalCompilerCXX target)
  if ("${COMPILER_API}" STREQUAL "gnu")
    GNUMakeFatallyStrictCXXCompiler(${target} ${ARGN})
  elseif ("${COMPILER_API}" STREQUAL "ti")
    TIMakeFatallyStrictCXXCompiler(${target} ${ARGN})
  else()
    message(WARNING "${_tag}: Unknown COMPILER_API: ${CXXOMPILER_API}")
  endif()
endfunction()

function(target_linker_script target script)
  if ("${COMPILER_API}" STREQUAL "gnu")
    set(_script "-T${script}")
  else()
    message(WARNING "target_linker_script: Unsupported COMPILER_API: ${COMPILER_API}")
    return()
  endif()

  if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13.0")
    target_link_options(${target} PRIVATE ${_script})
  else()
    target_link_libraries(${target} PRIVATE ${_script})
  endif()
endfunction()
