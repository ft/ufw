set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR native)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(TOOLCHAIN_ID "gcc-native")
set(COMPILER_API "gnu")


set(TOOLCHAIN_FEATURES
  sanitize-address
  sanitize-integer
  sanitize-undefined-behaviour)

set(TOOLCHAIN_FEATURE_SANITIZE_ADDRESS
  COMPILER_OPTIONS -fsanitize=address -fno-omit-frame-pointer
  LINKER_OPTIONS   -fsanitize=address -fno-omit-frame-pointer
  LINKER_LIBRARIES -lasan)

set(TOOLCHAIN_FEATURE_SANITIZE_INTEGER
  COMPILER_OPTIONS -fsanitize=integer-divide-by-zero
  LINKER_OPTIONS   -fsanitize=integer-divide-by-zero)

set(TOOLCHAIN_FEATURE_SANITIZE_UNDEFINED_BEHAVIOUR
  COMPILER_OPTIONS -fsanitize=undefined
  LINKER_OPTIONS   -fsanitize=undefined)
