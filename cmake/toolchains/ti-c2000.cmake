set(CGT_TOOLCHAIN_ROOT "/opt/ti/ti-cgt-c2000_18.1.2.LTS")

# set target system
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR c2000)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# toolchain paths
find_program(TI_CC NAMES cl2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_CXX NAMES cl2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_AS NAMES cl2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_AR NAMES ar2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_OBJCOPY NAMES ofd2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_OBJDUMP NAMES hex2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_SIZE NAMES size2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_LD NAMES cl2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)
find_program(TI_DIS NAMES dis2000 PATHS ${CGT_TOOLCHAIN_ROOT}/bin NO_DEFAULT_PATH)

include_directories(${CGT_TOOLCHAIN_ROOT}/include)
link_directories(${CGT_TOOLCHAIN_ROOT}/lib)

# This forces the C++ compiler's identification without using
# CMakeForceCompiler, which is deprecated. At some point we'll
# need to understand how compiler identification works, so we
# can fix that.
set(CMAKE_CXX_COMPILER_ID_RUN TRUE)
set(CMAKE_CXX_COMPILER_ID "TI")

set(CMAKE_C_FLAGS_INIT "--c99 --preproc_with_compile")
set(CMAKE_CXX_FLAGS_INIT "--c++03 --preproc_with_compile")

# set executables settings
set(CMAKE_C_COMPILER ${TI_CC})
set(CMAKE_CXX_COMPILER ${TI_CXX})
set(CMAKE_C_COMPILER_RANLIB "touch")
set(CMAKE_CXX_COMPILER_RANLIB "touch")
set(CMAKE_RANLIB "touch")
set(AS ${TI_AS})
set(AR ${TI_AR})
set(OBJCOPY ${TI_OBJCOPY})
set(OBJDUMP ${TI_OBJDUMP})
set(SIZE ${TI_SIZE})
set(LD ${TI_LD})

set(TOOLCHAIN_ID "ti-c2000")
set(COMPILER_API "ti")
