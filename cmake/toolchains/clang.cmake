set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR native)

find_program(_clang_no_version_ clang)
if (NOT ("${_clang_no_version_}" STREQUAL "${_clang_no_version_}-NOTFOUND"))
  message(STATUS "Found clang: clang")
  set(HAVE_CLANG_NOVERSION True)
else()
  set(HAVE_CLANG_NOVERSION True)
endif()

set(_clang_versions_)
set(_clang_version_ 1)
while ("${_clang_version_}" LESS 100)
  set(_clang_name_ "clang-${_clang_version_}")
  find_program(_clang_version_${_clang_version_}_ "${_clang_name_}")
  set(_result_ "_clang_version_${_clang_version_}_")
  if (NOT ("${${_result_}}" STREQUAL "${_result_}-NOTFOUND"))
    list(APPEND _clang_versions_ "${_clang_version_}")
  endif()
  math(EXPR _clang_version_ "${_clang_version_} + 1")
endwhile()

if (DEFINED TOOLCHAIN_SELECT_VERSION)
  if ("${TOOLCHAIN_SELECT_VERSION}" STREQUAL newest)
    list(GET _clang_versions_ -1 _clang_version_)
    set(TOOLCHAIN_CLANG_NAME   clang-${_clang_version_})
    set(TOOLCHAIN_CLANGXX_NAME clang++-${_clang_version_})
  elseif ("${TOOLCHAIN_SELECT_VERSION}" STREQUAL oldest)
    list(GET _clang_versions_ 0 _clang_version_)
    set(TOOLCHAIN_CLANG_NAME   clang-${_clang_version_})
    set(TOOLCHAIN_CLANGXX_NAME clang++-${_clang_version_})
  elseif ("${TOOLCHAIN_SELECT_VERSION}" STREQUAL default)
    set(TOOLCHAIN_CLANG_NAME   clang)
    set(TOOLCHAIN_CLANGXX_NAME clang++)
  else()
    set(TOOLCHAIN_CLANG_NAME   clang-${TOOLCHAIN_SELECT_VERSION})
    set(TOOLCHAIN_CLANGXX_NAME clang++-${TOOLCHAIN_SELECT_VERSION})
  endif()
else()
  set(TOOLCHAIN_CLANG_NAME   clang)
  set(TOOLCHAIN_CLANGXX_NAME clang++)
endif()

set(CMAKE_C_COMPILER "${TOOLCHAIN_CLANG_NAME}")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_CLANGXX_NAME}")

set(TOOLCHAIN_ID "clang-native")
set(COMPILER_API "gnu")

set(TOOLCHAIN_FEATURES
  sanitize-address
  sanitize-coverage-thorough
  sanitize-integer
  sanitize-undefined-behaviour
  fuzzer-libfuzzer)

set(TOOLCHAIN_FEATURE_SANITIZE_ADDRESS
  COMPILER_OPTIONS -fsanitize=address -fno-omit-frame-pointer
  LINKER_OPTIONS   -fsanitize=address -fno-omit-frame-pointer)

set(TOOLCHAIN_FEATURE_SANITIZE_COVERAGE_FAST
  COMPILER_OPTIONS -fsanitize-coverage=func
  LINKER_OPTIONS   -fsanitize-coverage=func)

set(TOOLCHAIN_FEATURE_SANITIZE_COVERAGE_BASIC
  COMPILER_OPTIONS -fsanitize-coverage=bb
  LINKER_OPTIONS   -fsanitize-coverage=bb)

set(TOOLCHAIN_FEATURE_SANITIZE_COVERAGE_THOROUGH
  COMPILER_OPTIONS -fsanitize-coverage=edge
  LINKER_OPTIONS   -fsanitize-coverage=edge)

set(TOOLCHAIN_FEATURE_SANITIZE_INTEGER
  COMPILER_OPTIONS -fsanitize=integer
  LINKER_OPTIONS   -fsanitize=integer)

set(TOOLCHAIN_FEATURE_SANITIZE_UNDEFINED_BEHAVIOUR
  COMPILER_OPTIONS -fsanitize=undefined
  LINKER_OPTIONS   -fsanitize=undefined)

set(TOOLCHAIN_FEATURE_FUZZER_LIBFUZZER
  LINKER_OPTIONS -lFuzzer)
