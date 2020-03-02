if(__UFW_BuildInTargetDir)
  return()
endif()
set(__UFW_BuildInTargetDir 1)

include(ExternalProject)

function(build_in_target_dir)
  if(NOT MICROFRAMEWORK_ROOT)
    message(FATAL_ERROR "MICROFRAMEWORK_ROOT is NOT set!")
  endif()

  set(__single_args BOARD TOOLCHAIN BUILDCFG)
  cmake_parse_arguments(PA "" "${__single_args}" "" ${ARGN})
  set(__install_prefix__
    "${UFW_ARTIFACTS_DIRECTORY}/${PA_BOARD}/${PA_TOOLCHAIN}/${PA_BUILDCFG}")
  ExternalProject_Add("build-${PA_BOARD}_${PA_TOOLCHAIN}_${PA_BUILDCFG}"
    PREFIX "build-${PA_BOARD}/${PA_TOOLCHAIN}/${PA_BUILDCFG}"
    SOURCE_DIR "${PROJECT_SOURCE_DIR}"
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${__install_prefix__}
    -DCMAKE_BUILD_TYPE=${PA_BUILDCFG}
    -DCMAKE_TOOLCHAIN_FILE=${MICROFRAMEWORK_ROOT}/cmake/toolchains/${PA_TOOLCHAIN}.cmake
    -DTARGET_BOARD=${PA_BOARD}
    -DCMAKE_EXPORT_COMPILE_COMMANDS=on
    INSTALL_DIR ${__install_prefix__}
    BUILD_ALWAYS 1
    TEST_AFTER_INSTALL 1
    TEST_COMMAND ctest --output-on-failure)
endfunction(build_in_target_dir)
