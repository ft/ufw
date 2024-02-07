set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR native)

# The AFL compiler instrumentation is based on clang, so this is very very
# similar to "clang.cmake".
set(TOOLCHAIN_ID "clang-native")
set(COMPILER_API "gnu")

set(cc afl-cc)
if (DEFINED TOOLCHAIN_CC_AFLPP)
  set(cc ${TOOLCHAIN_CC_AFLPP})
endif()
set(cxx afl-c++)
if (DEFINED TOOLCHAIN_CXX_AFLPP)
  set(cxx ${TOOLCHAIN_CXX_AFLPP})
endif()

find_program(COMPILER_BINARY ${cc}
  PATH_SUFFIXES "bin"
  HINTS ENV TOOLCHAIN_ROOT_AFLPP
  REQUIRED)
get_filename_component(TOOLCHAIN_ROOT ${COMPILER_BINARY} DIRECTORY)

set(CMAKE_C_COMPILER ${TOOLCHAIN_ROOT}/${cc})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/${cxx})

set(TOOLCHAIN_FEATURES
  sanitize-address
  sanitize-coverage-thorough
  sanitize-integer
  sanitize-undefined-behaviour)

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
