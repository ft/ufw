# Copyright (c) 2023-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_AddNanoPB)
  return()
endif()
set(__UFW_AddNanoPB 1)

macro(define_nanopb_protoc nanopb_root)
  set(NANOPB_PROTOC ${CMAKE_SOURCE_DIR}/${nanopb_root}/generator/protoc)
endmacro(define_nanopb_protoc)

macro(add_nanopb nanopb_root)
  option(nanopb_BUILD_RUNTIME
    "Build the headers and libraries needed at runtime"
    ON)
  option(nanopb_BUILD_GENERATOR
    "Build the protoc plugin for code generation"
    OFF)
  option(nanopb_MSVC_STATIC_RUNTIME
    "Link static runtime libraries"
    OFF)
  define_nanopb_protoc(${nanopb_root})
  add_subdirectory(${nanopb_root})
endmacro()
