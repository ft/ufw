set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_ID "ti-arm")
set(COMPILER_API "ti")

set(cc armcl)
if (DEFINED TOOLCHAIN_CC_TI_ARM)
  set(cc ${TOOLCHAIN_CC_TI_ARM})
endif()
set(cxx armcl)
if (DEFINED TOOLCHAIN_CXX_TI_ARM)
  set(cxx ${TOOLCHAIN_CXX_TI_ARM})
endif()

if (DEFINED ENV{TOOLCHAIN_ROOT_TI_ARM})
  find_program(COMPILER_BINARY ${cc}
    PATH_SUFFIXES "bin"
    HINTS ENV TOOLCHAIN_ROOT_TI_ARM
    REQUIRED)
else()
  # This resembles the old toolchain file's default behaviour.
  find_program(COMPILER_BINARY ${cc}
    PATH_SUFFIXES "bin"
    HINTS "/opt/ti/ti-cgt-arm_18.12.0.LTS"
    REQUIRED)
endif()
get_filename_component(TOOLCHAIN_BIN ${COMPILER_BINARY} DIRECTORY)
get_filename_component(TOOLCHAIN_ROOT ${TOOLCHAIN_BIN} DIRECTORY)

set(CMAKE_C_COMPILER          ${TOOLCHAIN_BIN}/${cc})
set(CMAKE_CXX_COMPILER        ${TOOLCHAIN_BIN}/${cxx})
set(AS                        ${TOOLCHAIN_BIN}/${cc})
set(AR                        ${TOOLCHAIN_BIN}/armar)
set(OBJCOPY                   ${TOOLCHAIN_BIN}/armofd)
set(OBJDUMP                   ${TOOLCHAIN_BIN}/armhex)
set(SIZE                      arm-none-eabi-size)
set(DISASSEMBLER              ${TOOLCHAIN_BIN}/armdis)
set(LD                        ${TOOLCHAIN_BIN}/${cc})
# Setting ranlib to touch, since TI's toolchain doesn't have this, and CMake
# tends to pick up the one from the host's native toolchain, which in turn
# breaks the libraries produced by the build system.
set(CMAKE_C_COMPILER_RANLIB   "touch")
set(CMAKE_CXX_COMPILER_RANLIB "touch")
set(CMAKE_RANLIB              "touch")

include_directories(SYSTEM ${TOOLCHAIN_ROOT}/include)
link_directories(${TOOLCHAIN_ROOT}/lib)

# Seems like CMake's compiler integration for TI's compilers lacks support for
# adhering to the C_STANDARD property (analogous for C++). Would be better if
# this was fixed upstream. Don't know if anyone will have time for this in
# short time... The same is true for the C2000 toolchain too. :-/
set(CMAKE_C_FLAGS_INIT   "--c99   --diag_warning=225 --diag_warning=255 --gen_func_subsections=on --preproc_with_compile")
set(CMAKE_CXX_FLAGS_INIT "--c++14 --diag_warning=225 --diag_warning=255 --gen_func_subsections=on --preproc_with_compile")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
