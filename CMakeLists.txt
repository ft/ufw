cmake_minimum_required (VERSION 3.1.0)
project(ufw)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

include(GNUCompilerWarnings)
include(GNUAttributes)
include(GNUBuiltins)

GNUCCompilerWarnings()
CheckAllGNUAttributes_C()
CheckAllGNUBuiltins_C()
MakeStrictCCompiler()

GNUCXXCompilerWarnings()
CheckAllGNUAttributes_CXX()
CheckAllGNUBuiltins_CXX()
MakeStrictCXXCompiler()

configure_file("${PROJECT_SOURCE_DIR}/common/ufw-config.h.in"
               "${PROJECT_BINARY_DIR}/common/ufw-config.h" )

add_subdirectory(c)
add_subdirectory(c++)
