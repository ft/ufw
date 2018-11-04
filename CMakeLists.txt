cmake_minimum_required (VERSION 3.1.0)
project(ufw)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

include(DetectMonolith)
include(GNUCompilerWarnings)
include(GNUAttributes)
include(GNUBuiltins)

DetectMonolith()

if ((${CMAKE_C_COMPILER_ID} MATCHES "GNU") OR
    (${CMAKE_C_COMPILER_ID} MATCHES "Clang"))
  GNUCCompilerWarnings()
  MakeStrictCCompiler()
endif()

CheckAllGNUAttributes_C()
CheckAllGNUBuiltins_C()

if ((${CMAKE_C_COMPILER_ID} MATCHES "GNU") OR
    (${CMAKE_C_COMPILER_ID} MATCHES "Clang"))
  GNUCXXCompilerWarnings()
  MakeStrictCXXCompiler()
endif()

CheckAllGNUAttributes_CXX()
CheckAllGNUBuiltins_CXX()

configure_file("${PROJECT_SOURCE_DIR}/common/toolchain.h.in"
               "${PROJECT_BINARY_DIR}/common/toolchain.h" )

add_subdirectory(c)
add_subdirectory(c++)
add_subdirectory(test)
