if (NOT CGT_TOOLCHAINS)
  set(CGT_TOOLCHAINS "/opt/ti/")
endif()

if (NOT CGT_TOOLCHAIN_ARCH)
  set(CGT_TOOLCHAIN_ARCH "arm")
endif()

if (NOT CGT_TOOLCHAIN_VERSION)
  set(CGT_TOOLCHAIN_VERSION "18.12.0.LTS")
endif()

if (NOT CGT_TOOLCHAIN_ROOT)
  set(CGT_TOOLCHAIN_ROOT
    "${CGT_TOOLCHAINS}/ti-cgt-${CGT_TOOLCHAIN_ARCH}_${CGT_TOOLCHAIN_VERSION}")
endif()

# set target system
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# toolchain paths
find_program(TI_CC NAMES armcl PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_CXX NAMES armcl PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_AS NAMES armcl PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_AR NAMES armar PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_OBJCOPY NAMES armofd PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_OBJDUMP NAMES armhex PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_LD NAMES armcl PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_DIS NAMES armdis PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)

include_directories(SYSTEM ${CGT_TOOLCHAIN_ROOT}/include)
link_directories(${CGT_TOOLCHAIN_ROOT}/lib)

# Seems like CMake's compiler integration for TI's compilers lacks support for
# adhering to the C_STANDARD property (analogous for C++). Would be better if
# this was fixed upstream. Don't know if anyone will have time for this in
# short time... The same is true for the C2000 toolchain too. :-/
set(CMAKE_C_FLAGS_INIT   "--c99   --diag_warning=225 --diag_warning=255 --gen_func_subsections=on --preproc_with_compile")
set(CMAKE_CXX_FLAGS_INIT "--c++14 --diag_warning=225 --diag_warning=255 --gen_func_subsections=on --preproc_with_compile")

# set executables settings
set(CMAKE_C_COMPILER ${TI_CC})
set(CMAKE_CXX_COMPILER ${TI_CXX})
set(CMAKE_C_COMPILER_RANLIB "touch")
set(CMAKE_CXX_COMPILER_RANLIB "touch")
set(CMAKE_RANLIB "touch")
set(AS ${TI_AS})
set(AR ${TI_AR})
# We can use the system's variant of the command, since ti-arm produces elf.
set(OBJCOPY objcopy)
set(OBJDUMP ${TI_OBJDUMP})
set(SIZE size)
set(LD ${TI_LD})

set(TOOLCHAIN_ID "ti-arm")
set(COMPILER_API "ti")
