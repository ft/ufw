set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR native)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(TOOLCHAIN_ID "clang-native")
set(COMPILER_API "gnu")

set(TOOLCHAIN_FEATURES
  sanitize-address
  sanitize-coverage-thorough
  sanitize-integer
  sanitize-undefined-behaviour
  fuzzer-libfuzzer)

set(TOOLCHAIN_FEATURE_SANITIZE_ADDRESS
  COMPILER_OPTIONS -fsanitize=address
  LINKER_OPTIONS   -fsanitize=address)

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
