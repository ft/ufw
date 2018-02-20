if(__UFW_ToolchainTIC2000)
  return()
endif()
set(__UFW_ToolchainTIC2000 1)

if (${CMAKE_TOOLCHAIN_FILE})
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR c2000)

# TODO: Much too simplistic, but works on my system. Good enough for a proof of concept
set(CMAKE_SYSROOT /opt/ti/ccsv7/tools/compiler/ti-cgt-c2000_16.9.6.LTS)

set(CMAKE_C_COMPILER cl2000)
set(CMAKE_CXX_COMPILER cl2000)

set(CMAKE_C_FLAGS_INIT "--diag_wrap=off --display_error_number")
set(CMAKE_CXX_FLAGS_INIT "--diag_wrap=off --display_error_number")

if ("${CMAKE_TI_TARGET}" STREQUAL "c28x")
  message("-- TI Target Processor: c28x")
  string(APPEND CMAKE_C_FLAGS_INIT " --silicon_version=28 --large_memory_model")
  string(APPEND CMAKE_C_FLAGS_INIT " --unified_memory")
  string(APPEND CMAKE_CXX_FLAGS_INIT " --silicon_version=28 --large_memory_model")
  string(APPEND CMAKE_CXX_FLAGS_INIT " --unified_memory")
endif()

string(APPEND CMAKE_C_FLAGS_INIT " --c99 -I${CMAKE_SYSROOT}/include")
string(APPEND CMAKE_CXX_FLAGS_INIT " --c++03 -I${CMAKE_SYSROOT}/include")
