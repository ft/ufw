if(__UFW_GNUAttributes)
  return()
endif()
set(__UFW_GNUAttributes 1)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

set(__UFW_GNUAttributes_warning_fail "warning.*attribute")

set(__UFW_GNUAttribute_cold
  "__attribute__((cold)) int rare(int);
   int main(void) { return 0; }")

set(__UFW_GNUAttribute_deprecated
  "void old(int horrible, char *api)
   __attribute__((deprecated));
   int main(void) { return 0; }")

set(__UFW_GNUAttribute_hot
  "__attribute__((hot)) int often(int);
   int main(void) { return 0; }")

set(__UFW_GNUAttribute_noreturn
  "__attribute__((noreturn)) void die(void);
   int main(void) { return 0; }")

set(__UFW_GNUAttribute_packed
  "struct s { int a; int b; } __attribute__((packed));
   int main(void) { return 0; }")

set(__UFW_GNUAttribute_section
  "__attribute__((section (\".bss\"))) int foo = 0;
   int main(void) { return 0; }")

set(__UFW_GNUAttribute_unused
  "int foo __attribute__((unused));
   int main(void) { return 0; }")

set(__UFW_GNUAttribute_weak_alias
  "void bar(void);
   void bar(void) { return; }
   void foo(void) __attribute__((weak, alias(\"bar\")));
   int main(void) { return 0; }")

set(__UFW_GNUAttribute_warn_unused_result
  "struct foo { int a; int b; };
   __attribute__((warn_unused_result))
   struct foo *new_foo(const char *bar);
   int main(void) { return 0; }")

macro(CheckGNUAttribute_C_cold)
  check_c_source_compiles("${__UFW_GNUAttribute_cold}"
    UFW_CC_HAS_ATTRIBUTE_COLD
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_C_deprecated)
  check_c_source_compiles("${__UFW_GNUAttribute_deprecated}"
    UFW_CC_HAS_ATTRIBUTE_DEPRECATED
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_C_hot)
  check_c_source_compiles("${__UFW_GNUAttribute_hot}"
    UFW_CC_HAS_ATTRIBUTE_HOT
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_C_noreturn)
  check_c_source_compiles("${__UFW_GNUAttribute_noreturn}"
    UFW_CC_HAS_ATTRIBUTE_NORETURN
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_C_packed)
  check_c_source_compiles("${__UFW_GNUAttribute_packed}"
    UFW_CC_HAS_ATTRIBUTE_PACKED
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_C_section)
  check_c_source_compiles("${__UFW_GNUAttribute_section}"
    UFW_CC_HAS_ATTRIBUTE_SECTION
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_C_unused)
  check_c_source_compiles("${__UFW_GNUAttribute_unused}"
    UFW_CC_HAS_ATTRIBUTE_UNUSED
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_C_weak_alias)
  check_c_source_compiles("${__UFW_GNUAttribute_weak_alias}"
    UFW_CC_HAS_ATTRIBUTE_WEAK_ALIAS
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_C_warn_unused_result)
  check_c_source_compiles("${__UFW_GNUAttribute_warn_unused_result}"
    UFW_CC_HAS_ATTRIBUTE_WARN_UNUSED_RESULT
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckAllGNUAttributes_C)
  CheckGNUAttribute_C_cold()
  CheckGNUAttribute_C_deprecated()
  CheckGNUAttribute_C_hot()
  CheckGNUAttribute_C_noreturn()
  CheckGNUAttribute_C_packed()
  CheckGNUAttribute_C_section()
  CheckGNUAttribute_C_unused()
  CheckGNUAttribute_C_weak_alias()
  CheckGNUAttribute_C_warn_unused_result()
endmacro()


macro(CheckGNUAttribute_CXX_cold)
  check_cxx_source_compiles("${__UFW_GNUAttribute_cold}"
    UFW_CXX_HAS_ATTRIBUTE_COLD
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_CXX_deprecated)
  check_cxx_source_compiles("${__UFW_GNUAttribute_deprecated}"
    UFW_CXX_HAS_ATTRIBUTE_DEPRECATED
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_CXX_hot)
  check_cxx_source_compiles("${__UFW_GNUAttribute_hot}"
    UFW_CXX_HAS_ATTRIBUTE_HOT
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_CXX_noreturn)
  check_cxx_source_compiles("${__UFW_GNUAttribute_noreturn}"
    UFW_CXX_HAS_ATTRIBUTE_NORETURN
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_CXX_packed)
  check_cxx_source_compiles("${__UFW_GNUAttribute_packed}"
    UFW_CXX_HAS_ATTRIBUTE_PACKED
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_CXX_section)
  check_cxx_source_compiles("${__UFW_GNUAttribute_section}"
    UFW_CXX_HAS_ATTRIBUTE_SECTION
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_CXX_unused)
  check_cxx_source_compiles("${__UFW_GNUAttribute_unused}"
    UFW_CXX_HAS_ATTRIBUTE_UNUSED
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_CXX_weak_alias)
  check_c_source_compiles("${__UFW_GNUAttribute_weak_alias}"
    UFW_CXX_HAS_ATTRIBUTE_WEAK_ALIAS
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckGNUAttribute_CXX_warn_unused_result)
  check_cxx_source_compiles("${__UFW_GNUAttribute_warn_unused_result}"
    UFW_CXX_HAS_ATTRIBUTE_WARN_UNUSED_RESULT
    FAIL_REGEX "${__UFW_GNUAttributes_warning_fail}")
endmacro()

macro(CheckAllGNUAttributes_CXX)
  CheckGNUAttribute_CXX_cold()
  CheckGNUAttribute_CXX_deprecated()
  CheckGNUAttribute_CXX_hot()
  CheckGNUAttribute_CXX_noreturn()
  CheckGNUAttribute_CXX_packed()
  CheckGNUAttribute_CXX_section()
  CheckGNUAttribute_CXX_unused()
  CheckGNUAttribute_CXX_weak_alias()
  CheckGNUAttribute_CXX_warn_unused_result()
endmacro()
