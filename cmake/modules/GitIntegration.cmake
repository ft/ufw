if(__UFW_GitIntegration)
  return()
endif()
set(__UFW_GitIntegration 1)

include(UFWTools)

function(gitint_options prefix outvar)
  set(GENERATE_OPTIONS)

  if (DEFINED ${prefix}BUILD_VARIANT)
    list(APPEND GENERATE_OPTIONS -b "${${prefix}BUILD_VARIANT}")
  endif()

  if (DEFINED ${prefix}TARGET_MCU)
    list(APPEND GENERATE_OPTIONS -m "${${prefix}TARGET_MCU}")
  endif()

  if (DEFINED ${prefix}NAME)
    list(APPEND GENERATE_OPTIONS -n "${${prefix}NAME}")
  else()
    list(APPEND GENERATE_OPTIONS -n "${PROJECT_NAME}")
  endif()

  if (DEFINED ${prefix}TYPE)
    list(APPEND GENERATE_OPTIONS -t "${${prefix}TYPE}")
  endif()

  if (DEFINED CMAKE_BUILD_TYPE AND (NOT "${CMAKE_BUILD_TYPE}" STREQUAL ""))
    string(TOLOWER "${CMAKE_BUILD_TYPE}" profile)
    list(APPEND GENERATE_OPTIONS -p "${profile}")
  endif()

  if (DEFINED TARGET_BOARD AND (NOT "${TARGET_BOARD}" STREQUAL ""))
    list(APPEND GENERATE_OPTIONS -h "${TARGET_BOARD}")
  elseif (DEFINED BOARD AND (NOT "${BOARD}" STREQUAL ""))
    list(APPEND GENERATE_OPTIONS -h "${BOARD}")
  endif()

  set(${outvar} "${GENERATE_OPTIONS}" PARENT_SCOPE)
endfunction()

macro(generate_version_h)
  cmake_parse_arguments(
    PARSED_ARGS
    "SYSTEM_VERSION"
    "BUILD_VARIANT;NAME;TARGET;TARGET_MCU;TEMPLATE;OUTPUT;TYPE"
    "SCRIPTS;EXPANSIONS"
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

  set(GENERATE_OPTIONS)
  gitint_options(PARSED_ARGS_ GENERATE_OPTIONS)

  if (${PARSED_ARGS_SYSTEM_VERSION})
    set(UFW_SYSTEM_WITH_VERSION_H 1)
    set(UFW_SYSTEM_VERSION_HEADER ${PARSED_ARGS_OUTPUT})
  else()
    if (DEFINED UFW_SYSTEM_WITH_VERSION_H AND DEFINED UFW_SYSTEM_VERSION_HEADER)
      list(APPEND PARSED_ARGS_EXPANSIONS
        WITH_SYSTEM_VERSION_H=${UFW_SYSTEM_WITH_VERSION_H}
        SYSTEM_VERSION_H=${UFW_SYSTEM_VERSION_HEADER})
    endif()
  endif()

  add_custom_target(${PARSED_ARGS_TARGET})
  add_custom_command(
    TARGET "${PARSED_ARGS_TARGET}"
    COMMAND
    "${MICROFRAMEWORK_ROOT}/bin/generate-version-h"
    "${MICROFRAMEWORK_ROOT}"
    ${GENERATE_OPTIONS}
    "${PARSED_ARGS_TEMPLATE}"
    "${PARSED_ARGS_OUTPUT}"
    ${PARSED_ARGS_SCRIPTS}
    "${MICROFRAMEWORK_ROOT}/vcs-integration/git.sh"
    -- ${PARSED_ARGS_EXPANSIONS}
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

  if (NOT PARSED_ARGS_DESTINATION)
    set(PARSED_ARGS_DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()

  if (NOT PARSED_ARGS_NAME)
    set(PARSED_ARGS_NAME ${PROJECT_NAME})
  endif()

  set(GENERATE_OPTIONS)
  gitint_options(PARSED_ARGS_ GENERATE_OPTIONS)

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
"
  COMPONENT ${UFW_INSTALL_COMPONENT})
  endif()

  install(CODE "
execute_process(
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
  \"${MICROFRAMEWORK_ROOT}/bin/generate-artifacts\"
  \"${MICROFRAMEWORK_ROOT}\"
  ${GENERATE_OPTIONS}
  doesnotmatter
  \"${PARSED_ARGS_DESTINATION}\"
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
"
  COMPONENT ${UFW_INSTALL_COMPONENT})

endmacro(gitint_install)
