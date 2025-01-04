# Copyright (c) 2018-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_GNUBuiltins)
  return()
endif()
set(__UFW_GNUBuiltins 1)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

function(__ufw_gnu_builtin_test var builtin fallback)
  set(${var}
  "#if defined __has_builtin
   #if __has_builtin(${builtin})
   int main(void) { return 0; }
   #else
   #error \"${builtin} not available\"
   #endif
   #else
   ${fallback}
   #endif"
  PARENT_SCOPE)
endfunction()

__ufw_gnu_builtin_test(
  __UFW_GNUBuiltin_expect __builtin_expect
  "int main(void) {
     int i;
     if (__builtin_expect(1,1))
       i = 1;
     return 0;
   }")

macro(CheckGNUBuiltin_C_expect)
  check_c_source_compiles("${__UFW_GNUBuiltin_expect}"
    UFW_CC_HAS_BUILTIN_EXPECT)
endmacro()

macro(CheckGNUBuiltin_CXX_expect)
  check_cxx_source_compiles("${__UFW_GNUBuiltin_expect}"
    UFW_CXX_HAS_BUILTIN_EXPECT)
endmacro()

function(CheckGNUBuiltin_bswap_n compiler width)
  __ufw_gnu_builtin_test(
    __code "__builtin_bswap${width}"
    "int main(void) {
       __builtin_bswap${width}(0);
       return 0;
     }")
  set(_result "UFW_${compiler}_HAS_BUILTIN_BSWAP${width}")
  check_c_source_compiles("${__code}" ${_result})
  set(${_result} ${${_result}} PARENT_SCOPE)
endfunction()

macro(CheckGNUBuiltin_C_bswap)
  CheckGNUBuiltin_bswap_n(CC 16)
  CheckGNUBuiltin_bswap_n(CC 32)
  CheckGNUBuiltin_bswap_n(CC 64)
endmacro()

macro(CheckGNUBuiltin_CXX_bswap)
  CheckGNUBuiltin_bswap_n(CXX 16)
  CheckGNUBuiltin_bswap_n(CXX 32)
  CheckGNUBuiltin_bswap_n(CXX 64)
endmacro()

macro(CheckAllGNUBuiltins_C)
  CheckGNUBuiltin_C_expect()
  CheckGNUBuiltin_C_bswap()
endmacro()

macro(CheckAllGNUBuiltins_CXX)
  CheckGNUBuiltin_CXX_expect()
  CheckGNUBuiltin_CXX_bswap()
endmacro()
