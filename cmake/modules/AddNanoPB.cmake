if(__UFW_AddNanoPB)
  return()
endif()
set(__UFW_AddNanoPB 1)

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
  set(NANOPB_PROTOC ${CMAKE_SOURCE_DIR}/${nanopb_root}/generator/protoc)
  add_subdirectory(${nanopb_root})
endmacro()
