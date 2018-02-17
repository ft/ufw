if(__UFW_GNUBuiltins)
  return()
endif()
set(__UFW_GNUBuiltins 1)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

set(__UFW_GNUBuiltin_expect
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

macro(CheckAllGNUBuiltins_C)
  CheckGNUBuiltin_C_expect()
endmacro()

macro(CheckGNUBuiltin_CXX_expect)
  check_cxx_source_compiles("${__UFW_GNUBuiltin_expect}"
    UFW_CXX_HAS_BUILTIN_EXPECT)
endmacro()

macro(CheckAllGNUBuiltins_CXX)
  CheckGNUBuiltin_CXX_expect()
endmacro()
