cmake_minimum_required (VERSION 3.1.0)
project(ufw)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

include(GNUCompilerWarnings)
include(GNUAttributes)
include(GNUBuiltins)

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

configure_file("${PROJECT_SOURCE_DIR}/include/common/toolchain.h.in"
               "${PROJECT_BINARY_DIR}/include/common/toolchain.h" )

add_library(ufw STATIC src/c/ring-buffer-iter.c
                       src/c/register-table.c)
target_include_directories(ufw PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(ufw++ INTERFACE)
target_include_directories(ufw++ INTERFACE ${CMAKE_SOURCE_DIR}/include)

add_library(ufw-common INTERFACE)
target_include_directories(ufw-common
  INTERFACE ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_BINARY_DIR}/include)

add_subdirectory(test)
