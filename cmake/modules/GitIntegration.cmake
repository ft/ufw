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

macro(generate_commitdate_file)
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
    "${MICROFRAMEWORK_ROOT}/bin/print-commitdate"
    "${MICROFRAMEWORK_ROOT}"
    "${PARSED_ARGS_OUTPUT}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endmacro()

macro(gitint_install)
  cmake_parse_arguments(
    PARSED_ARGS
    "LATEST_ARTIFACTS"
    "DESTINATION;BUILD_VARIANT;NAME;TARGET_MCU;TYPE"
    "FILES;SCRIPTS"
    ${ARGN})
  if (NOT MICROFRAMEWORK_ROOT)
    message(FATAL_ERROR "MICROFRAMEWORK_ROOT is not set! Cannot continue.")
  endif()
  set(GENERATE_OPTIONS)
  if (NOT PARSED_ARGS_DESTINATION)
    set(PARSED_ARGS_DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()

  if (DEFINED PARSED_ARGS_BUILD_VARIANT)
    list(APPEND GENERATE_OPTIONS -b "${PARSED_ARGS_BUILD_VARIANT}")
  endif()

  if (DEFINED PARSED_ARGS_TARGET_MCU)
    list(APPEND GENERATE_OPTIONS -m "${PARSED_ARGS_TARGET_MCU}")
  endif()

  if (DEFINED PARSED_ARGS_NAME)
    list(APPEND GENERATE_OPTIONS -n "${PARSED_ARGS_NAME}")
  else()
    set(PARSED_ARGS_NAME "${PROJECT_NAME}")
  endif()

  if (DEFINED PARSED_ARGS_TYPE)
    list(APPEND GENERATE_OPTIONS -t "${PARSED_ARGS_TARGET_TYPE}")
  endif()

  if (DEFINED CMAKE_BUILD_TYPE)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" profile)
    list(APPEND GENERATE_OPTIONS -p "${profile}")
  endif()

  if (PARSED_ARGS_LATEST_ARTIFACTS)
  install(CODE "
foreach (srcfile ${PARSED_ARGS_FILES})
  if(NOT EXISTS \${srcfile})
    message(SEND_ERROR \"File \${srcfile} not found.\")
  endif()
  get_filename_component(_EXT_ \"\${srcfile}\" EXT)
  get_filename_component(_REALPATH_ \"\${srcfile}\" REALPATH)
  set(_OFILE_ \"${PARSED_ARGS_DESTINATION}/latest/${PARSED_ARGS_NAME}\${_EXT_}\")
  message(\"-- Installing to \${_OFILE_}\")
  execute_process(COMMAND cmake -E copy_if_different \${_REALPATH_} \${_OFILE_})
endforeach()
")
  endif()

  install(CODE "
execute_process(
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
  \"${MICROFRAMEWORK_ROOT}/bin/generate-artifacts\"
  ${GENERATE_OPTIONS}
  doesnotmatter
  ${PARSED_ARGS_DESTINATION}
  \"++\"
  \"--\"
  \"${MICROFRAMEWORK_ROOT}/vcs-integration/git.sh\"
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
