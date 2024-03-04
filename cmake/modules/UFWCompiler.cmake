if(__UFW_UFWCompiler)
  return()
endif()
set(__UFW_UFWCompiler 1)

include(CheckCSourceCompiles)

function(ufw_compiler_has_type type var)
  check_c_source_compiles("
#include <stdint.h>
#include <stddef.h>
int test(${type} *ptr) {
  return (ptr == NULL);
}
int main(void) {
  return 0;
}"
   ${var})
  if (${var})
    message(STATUS "Toolchain supports ${type}")
  else()
    message(STATUS "Toolchain does not support ${type}")
  endif()
  set(${var} ${${var}} PARENT_SCOPE)
endfunction()
