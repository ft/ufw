if (${CMAKE_TOOLCHAIN_FILE})
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(triple arm-none-eabi)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
SET(CMAKE_ASM_COMPILER clang)
SET(CMAKE_ASM_COMPILER_TARGET ${triple})

# Either use cmake -DTOOLCHAIN_PATH=/path/to/toolchain or set the environment
# variable CLANG_ARM_NONE_EABI_TOOLCHAIN_PATH
find_path(sysroot_root arm-none-eabi
  HINTS /usr/lib ${TOOLCHAIN_PATH} ENV CLANG_ARM_NONE_EABI_TOOLCHAIN_PATH REQUIRED)
set(CMAKE_SYSROOT "${sysroot_root}/arm-none-eabi")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld -nostdlib")

set(TOOLCHAIN_ID "clang-arm")
set(COMPILER_API "gnu")
