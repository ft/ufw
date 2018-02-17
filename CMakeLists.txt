cmake_minimum_required (VERSION 3.1.0)
project(ufw)

include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

check_c_compiler_flag(-Wall UFW_CC_HAS_Wall)
check_c_compiler_flag(-Wextra UFW_CC_HAS_Wextra)
check_c_compiler_flag(-Werror UFW_CC_HAS_Werror)
check_c_compiler_flag(-Wasm UFW_CC_HAS_Wasm)
check_c_compiler_flag(-Wimplicit-fallthrough=3 UFW_CC_HAS_Wimplicit_fallthrough)
check_c_compiler_flag(-Wimplicit-function-declaration UFW_CC_HAS_Wimplicit_function_declaration)
check_c_compiler_flag(-Wmissing-prototypes UFW_CC_HAS_Wmissing_prototypes)
check_c_compiler_flag(-Wnewline-eof UFW_CC_HAS_Wnewline_eof)
check_c_compiler_flag(-Wpacked UFW_CC_HAS_Wpacked)
check_c_compiler_flag(-Wpadded UFW_CC_HAS_Wpadded)
check_c_compiler_flag(-Wpedantic UFW_CC_HAS_Wpedantic)
check_c_compiler_flag(-Wredundant-decls UFW_CC_HAS_Wredundant_decls)
check_c_compiler_flag(-Wsequence-point UFW_CC_HAS_Wsequence_point)
check_c_compiler_flag(-Wshadow UFW_CC_HAS_Wshadow)
check_c_compiler_flag(-Wshift-sign-overflow UFW_CC_HAS_Wshift_sign_overflow)
check_c_compiler_flag(-Wstrict-prototypes UFW_CC_HAS_Wstrict_prototypes)
check_c_compiler_flag(-Wundef UFW_CC_HAS_Wundef)
check_c_compiler_flag(-Wused-but-marked-unused UFW_CC_HAS_Wused_but_marked_unused)

check_c_source_compiles("__attribute__((cold)) int rare(int);
                         int main(void) { return 0; }"
  UFW_CC_HAS_ATTRIBUTE_COLD
  FAIL_REGEX "warning.*attribute")

check_c_source_compiles("void old(int horrible, char *api)
                         __attribute__((deprecated));
                         int main(void) { return 0; }"
  UFW_CC_HAS_ATTRIBUTE_DEPRECATED
  FAIL_REGEX "warning.*attribute")

check_c_source_compiles("__attribute__((hot)) int often(int);
                         int main(void) { return 0; }"
  UFW_CC_HAS_ATTRIBUTE_HOT
  FAIL_REGEX "warning.*attribute")

check_c_source_compiles("__attribute__((noreturn)) void die(void);
                         int main(void) { return 0; }"
  UFW_CC_HAS_ATTRIBUTE_NORETURN
  FAIL_REGEX "warning.*attribute")

check_c_source_compiles("struct s { int a; int b; } __attribute__((packed));
                         int main(void) { return 0; }"
  UFW_CC_HAS_ATTRIBUTE_PACKED
  FAIL_REGEX "warning.*attribute")

check_c_source_compiles("__attribute__((section (\".bss\"))) int foo = 0;
                         int main(void) { return 0; }"
  UFW_CC_HAS_ATTRIBUTE_SECTION
  FAIL_REGEX "warning.*attribute")

check_c_source_compiles("int foo __attribute__((unused));
                         int main(void) { return 0; }"
  UFW_CC_HAS_ATTRIBUTE_UNUSED
  FAIL_REGEX "warning.*attribute")

check_c_source_compiles("struct foo { int a; int b; };
                         __attribute__((warn_unused_result))
                         struct foo *new_foo(const char *bar);
                         int main(void) { return 0; }"
  UFW_CC_HAS_ATTRIBUTE_WARN_UNUSED_RESULT
  FAIL_REGEX "warning.*attribute")

check_c_source_compiles("int main(void) {
                           int i;
                           if (__builtin_expect(1,1))
                             i = 1;
                           return 0;
                         }"
  UFW_CC_HAS_BUILTIN_EXPECT)

check_cxx_compiler_flag(-Wall UFW_CXX_HAS_Wall)
check_cxx_compiler_flag(-Wextra UFW_CXX_HAS_Wextra)
check_cxx_compiler_flag(-Werror UFW_CXX_HAS_Werror)
check_cxx_compiler_flag(-Wasm UFW_CXX_HAS_Wasm)
check_cxx_compiler_flag(-Wimplicit-fallthrough=3 UFW_CXX_HAS_Wimplicit_fallthrough)
check_cxx_compiler_flag(-Wnewline-eof UFW_CXX_HAS_Wnewline_eof)
check_cxx_compiler_flag(-Wpacked UFW_CXX_HAS_Wpacked)
check_cxx_compiler_flag(-Wpadded UFW_CXX_HAS_Wpadded)
check_cxx_compiler_flag(-Wpedantic UFW_CXX_HAS_Wpedantic)
check_cxx_compiler_flag(-Wredundant-decls UFW_CXX_HAS_Wredundant_decls)
check_cxx_compiler_flag(-Wsequence-point UFW_CXX_HAS_Wsequence_point)
check_cxx_compiler_flag(-Wshadow UFW_CXX_HAS_Wshadow)
check_cxx_compiler_flag(-Wshift-sign-overflow UFW_CXX_HAS_Wshift_sign_overflow)
check_cxx_compiler_flag(-Wundef UFW_CXX_HAS_Wundef)
check_cxx_compiler_flag(-Wused-but-marked-unused UFW_CXX_HAS_Wused_but_marked_unused)

check_cxx_source_compiles("__attribute__((cold)) int rare(int);
                           int main(void) { return 0; }"
  UFW_CXX_HAS_ATTRIBUTE_COLD
  FAIL_REGEX "warning.*attribute")

check_cxx_source_compiles("void old(int horrible, char *api)
                           __attribute__((deprecated));
                           int main(void) { return 0; }"
  UFW_CXX_HAS_ATTRIBUTE_DEPRECATED
  FAIL_REGEX "warning.*attribute")

check_cxx_source_compiles("__attribute__((hot)) int often(int);
                           int main(void) { return 0; }"
  UFW_CXX_HAS_ATTRIBUTE_HOT
  FAIL_REGEX "warning.*attribute")

check_cxx_source_compiles("__attribute__((noreturn)) void die(void);
                           int main(void) { return 0; }"
  UFW_CXX_HAS_ATTRIBUTE_NORETURN
  FAIL_REGEX "warning.*attribute")

check_cxx_source_compiles("struct s { int a; int b; } __attribute__((packed));
                           int main(void) { return 0; }"
  UFW_CXX_HAS_ATTRIBUTE_PACKED
  FAIL_REGEX "warning.*attribute")

check_cxx_source_compiles("__attribute__((section (\".bss\"))) int foo = 0;
                           int main(void) { return 0; }"
  UFW_CXX_HAS_ATTRIBUTE_SECTION
  FAIL_REGEX "warning.*attribute")

check_cxx_source_compiles("int foo __attribute__((unused));
                           int main(void) { return 0; }"
  UFW_CXX_HAS_ATTRIBUTE_UNUSED
  FAIL_REGEX "warning.*attribute")

check_cxx_source_compiles("struct foo { int a; int b; };
                           __attribute__((warn_unused_result))
                           struct foo *new_foo(const char *bar);
                           int main(void) { return 0; }"
  UFW_CXX_HAS_ATTRIBUTE_WARN_UNUSED_RESULT
  FAIL_REGEX "warning.*attribute")

check_cxx_source_compiles("int main(void) {
                             int i;
                             if (__builtin_expect(1,1))
                               i = 1;
                             return 0;
                           }"
  UFW_CXX_HAS_BUILTIN_EXPECT)

configure_file("${PROJECT_SOURCE_DIR}/common/config.h.in"
               "${PROJECT_BINARY_DIR}/common/config.h" )

add_subdirectory(c)
add_subdirectory(c++)
