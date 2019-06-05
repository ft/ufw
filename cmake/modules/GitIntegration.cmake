if(__UFW_GitIntegration)
  return()
endif()
set(__UFW_GitIntegration 1)

macro(generate_version_h)
  cmake_parse_arguments(
    PARSED_ARGS
    ""
    "TARGET;TEMPLATE;OUTPUT"
    "SCRIPTS"
    ${ARGN})
  if (NOT MICROFRAMEWORK_ROOT)
    message(FATAL_ERROR "MICROFRAMEWORK_ROOT is not set! Cannot continue.")
  endif()
  if (NOT PARSED_ARGS_TARGET)
    message(FATAL_ERROR "TARGET is not set!")
  endif()
  if (NOT PARSED_ARGS_TEMPLATE)
    message(FATAL_ERROR "TEMPLATE is not set!")
  endif()
  if (NOT PARSED_ARGS_OUTPUT)
    message(FATAL_ERROR "OUTPUT is not set!")
  endif()

  add_custom_target(${PARSED_ARGS_TARGET})
  add_custom_command(
    TARGET "${PARSED_ARGS_TARGET}"
    COMMAND
    "${MICROFRAMEWORK_ROOT}/bin/generate-version-h"
    "${PARSED_ARGS_TEMPLATE}"
    "${PARSED_ARGS_OUTPUT}"
    ${PARSED_ARGS_SCRIPTS}
    "${MICROFRAMEWORK_ROOT}/vcs-integration/git.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endmacro()

macro(generate_revision_file)
  cmake_parse_arguments(
    PARSED_ARGS
    ""
    "TARGET;OUTPUT"
    ""
    ${ARGN})
  if (NOT MICROFRAMEWORK_ROOT)
    message(FATAL_ERROR "MICROFRAMEWORK_ROOT is not set! Cannot continue.")
  endif()
  if (NOT PARSED_ARGS_TARGET)
    message(FATAL_ERROR "TARGET is not set!")
  endif()
  if (NOT PARSED_ARGS_OUTPUT)
    message(FATAL_ERROR "OUTPUT is not set!")
  endif()

  add_custom_target(${PARSED_ARGS_TARGET})
  add_custom_command(
    TARGET "${PARSED_ARGS_TARGET}"
    COMMAND
    "${MICROFRAMEWORK_ROOT}/bin/print-version"
    "${MICROFRAMEWORK_ROOT}"
    "${PARSED_ARGS_OUTPUT}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endmacro()

macro(gitint_install)
  cmake_parse_arguments(
    PARSED_ARGS
    ""
    "DESTINATION;BUILD_VARIANT"
    "FILES;SCRIPTS"
    ${ARGN})
  if (NOT MICROFRAMEWORK_ROOT)
    message(FATAL_ERROR "MICROFRAMEWORK_ROOT is not set! Cannot continue.")
  endif()
  if (NOT PARSED_ARGS_DESTINATION)
    set(PARSED_ARGS_DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()

  if (NOT PARSED_ARGS_BUILD_VARIANT)
    set(PARSED_ARGS_BUILD_VARIANT "")
  endif()
  install(CODE "
set(ENV{BUILD_VARIANT} \"${PARSED_ARGS_BUILD_VARIANT}\")
execute_process(
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
  \"${MICROFRAMEWORK_ROOT}/generate-artifacts\"
  doesnotmatter
  ${PARSED_ARGS_DESTINATION}
  \"++\"
  \"--\"
  \"${MICROFRAMEWORK_ROOT}/git.sh\"
  ${PARSED_ARGS_SCRIPTS}
  OUTPUT_VARIABLE GITINTNAME
  OUTPUT_STRIP_TRAILING_WHITESPACE)
foreach (srcfile ${PARSED_ARGS_FILES})
  if(NOT EXISTS \${srcfile})
    message(SEND_ERROR \"File \${srcfile} not found.\")
  endif()
  get_filename_component(EXTENSION \"\${srcfile}\" EXT)
  get_filename_component(_REALPATH_ \"\${srcfile}\" REALPATH)
  message(\"-- Installing to \${GITINTNAME}\${EXTENSION}\")
  execute_process(
    COMMAND cmake -E copy_if_different
                  \${_REALPATH_}
                  \${GITINTNAME}\${EXTENSION})
endforeach()
")

endmacro(gitint_install)
