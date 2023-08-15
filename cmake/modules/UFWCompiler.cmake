if(__UFW_UFWCompiler)
  return()
endif()
set(__UFW_UFWCompiler 1)

include(CheckCSourceCompiles)

function(ufw_compiler_has_type type var)
  # check_c_source_compiles() uses try_compile, which will use the toolchain
  # file we used in a build again. Unfortunately, it doesn't hand in any
  # parameters that were passed to the initial cmake call. Do when we're
  # adjusting CGT_TOOLCHAIN_ROOT for the TI toolchains on the command line,
  # that is not picked up by sub-build in try_compile(), which leads to the
  # compiler not finding its header files. This all seems to brittle. :-(
  if (DEFINED CGT_TOOLCHAIN_ROOT)
    set(CMAKE_REQUIRED_INCLUDES "${CGT_TOOLCHAIN_ROOT}/include")
  endif()
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
