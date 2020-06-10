if(__UFW_TICompilerWarnings)
  return()
endif()
set(__UFW_TICompilerWarnings 1)

set(__UFW_TI_MISRA_SECTIONS
  3
  8.1 8.2 8.11 8.12
  9.1 9.2 9.3
  12.1 12.2 12.3 12.7 12.8 12.9
  13.1 13.2 13.3
  14.1 14.2 14.3
  15.2 15.3 15.4 15.5
  16.5 16.7 16.8 16.9
  19.1 19.2 19.3 19.11 19.15 19.16 19.17
  20.1 20.2)

set(__UFW_TI_MISRA_STRING "--check_misra=")
set(__UFW_TI_MISRA_STRING_FIRST 1)
foreach (section ${__UFW_TI_MISRA_SECTIONS})
  if (${__UFW_TI_MISRA_STRING_FIRST} EQUAL 1)
    set(__UFW_TI_MISRA_STRING_FIRST 0)
  else()
    set(__UFW_TI_MISRA_STRING "${__UFW_TI_MISRA_STRING},")
  endif()
  set(__UFW_TI_MISRA_STRING "${__UFW_TI_MISRA_STRING}${section}")
endforeach()

list(APPEND __UFW_TI_DEFAULT_FLAGS
  --issue_remarks
  --diag_wrap=off
  --define=xdc__strict
  --advice:performance=all)

function(TIMakeStrictCCompiler target)
  list(APPEND _flags ${__UFW_TI_DEFAULT_FLAGS} ${__UFW_TI_MISRA_STRING})
  if (_flags)
    target_compile_options(${target} PRIVATE ${_flags})
  endif()
endfunction()

function(TIMakeStrictCXXCompiler target)
  TIMakeStrictCCompiler(${target})
endfunction()

function(TIMakeFatallyStrictCCompiler target)
  cmake_parse_arguments(PARSED_ARGS "" "LEVEL" "" ${ARGN})
  if (NOT PARSED_ARGS_LEVEL)
    set(PARSED_ARGS_LEVEL "fatal")
  endif()

  list(APPEND _flags --emit_warnings_as_errors --misra_required=error)
  if (${PARSED_ARGS_LEVEL} STREQUAL "fatal")
    list(APPEND _flags --misra_advisory=warning)
  endif()
  if (${PARSED_ARGS_LEVEL} STREQUAL "fatal-misra")
    list(APPEND _flags --misra_advisory=error)
  endif()

  target_compile_options(${target} PRIVATE ${_flags})
endfunction()

function(TIMakeFatallyStrictCXXCompiler target)
  TIMakeFatallyStrictCCompiler(${target} ${ARGN})
endfunction()
