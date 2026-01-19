# Copyright (c) 2019-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_GenerateGraphics)
  return()
endif()
set(__UFW_GenerateGraphics 1)

find_program(UFW_INKSCAPE_EXECUTABLE inkscape)

execute_process(
  COMMAND "${UFW_INKSCAPE_EXECUTABLE}" --version
  OUTPUT_VARIABLE UFW_INKSCAPE_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  RESULT_VARIABLE UFW_INKSCAPE_VERSION_RESULT)


if ("${UFW_INKSCAPE_VERSION_RESULT}" EQUAL 0)
  if ("${UFW_INKSCAPE_VERSION}" MATCHES "^.nkscape *([0-9]+)\.")
    set(UFW_INKSCAPE_VERSION "${CMAKE_MATCH_1}")
  else()
    set(UFW_INKSCAPE_VERSION 0)
  endif()
  message(STATUS
    "inkscape: ${UFW_INKSCAPE_EXECUTABLE}; major version: ${UFW_INKSCAPE_VERSION}")
else()
  message(WARNING "inkscape executable not found on system!")
  set(UFW_INKSCAPE_VERSION 0)
endif()


function(ufw_inkscape_make_command type input output outvar)
  if ("${UFW_INKSCAPE_VERSION}" EQUAL 0)
    set(cmd "${UFW_INKSCAPE_EXECUTABLE}"
            --export-${type} ${output} ${input})
  else()
    set(cmd "${UFW_INKSCAPE_EXECUTABLE}"
            --export-type=${type} --export-filename=${output} ${input})
  endif()
  set(${outvar} ${cmd} PARENT_SCOPE)
endfunction()

function(GenerateGraphics_svg2pdf target source output)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  ufw_inkscape_make_command(
    pdf "${CMAKE_CURRENT_SOURCE_DIR}/${source}" "${output}" cmd)
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${cmd})
endfunction(GenerateGraphics_svg2pdf)

function(GenerateGraphics_svg2png target source output)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  ufw_inkscape_make_command(
    png "${CMAKE_CURRENT_SOURCE_DIR}/${source}" "${output}" cmd)
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${cmd})
endfunction(GenerateGraphics_svg2png)

function(GenerateGraphics_tex2pdf target source output)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND pdflatex ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    && pdfcrop ${output} cropped-${output}
    && mv cropped-${output} ${output})
endfunction(GenerateGraphics_tex2pdf)

function(GenerateGraphics_pdf2png target source output)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND convert -verbose
                    -density 250
                    -trim ${source}
                    -quality 100
                    -flatten
                    -sharpen 0x1.0
                    ${output})
endfunction(GenerateGraphics_pdf2png)

function(GenerateGraphics_puml target source output type)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND plantuml -verbose -t${type}
                     -o ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/${source})
endfunction(GenerateGraphics_puml)

function(GenerateGraphics_uxf target source output type)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND umlet -action=convert -format=${type} -filename=${CMAKE_CURRENT_SOURCE_DIR}/${source}
                  -output=${CMAKE_CURRENT_BINARY_DIR}/${output})
endfunction(GenerateGraphics_uxf)

function(GenerateGraphics_drawio target source output type)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND drawio -x -f ${type} --crop -o ${CMAKE_CURRENT_BINARY_DIR}/${output}
                   ${CMAKE_CURRENT_SOURCE_DIR}/${source})
endfunction(GenerateGraphics_drawio)

set(GenerateGraphics_supported_sources .svg .tex .puml .uxf .drawio)

function(generate_graphic)
  cmake_parse_arguments(PA "" "SOURCE;TARGET" "FORMATS" ${ARGN})
  get_filename_component(basename ${PA_SOURCE} NAME_WE)
  get_filename_component(extension ${PA_SOURCE} EXT)
  list(FIND GenerateGraphics_supported_sources ${extension} SUPPORTED_EXT)
  if (${SUPPORTED_EXT} EQUAL -1)
    message(FATAL_ERROR "Unsupported source format: ${PA_SOURCE} (${extension})")
  endif()
  foreach (variant ${PA_FORMATS})
    set(output "${basename}.${variant}")
    if (${variant} STREQUAL "pdf")
      if (${extension} STREQUAL .tex)
        GenerateGraphics_tex2pdf(${PA_TARGET} ${PA_SOURCE} ${output})
      elseif (${extension} STREQUAL .svg)
        GenerateGraphics_svg2pdf(${PA_TARGET} ${PA_SOURCE} ${output})
      elseif (${extension} STREQUAL .puml)
        GenerateGraphics_puml(${PA_TARGET} ${PA_SOURCE} ${output} ${variant})
      elseif (${extension} STREQUAL .uxf)
        GenerateGraphics_uxf(${PA_TARGET} ${PA_SOURCE} ${output} ${variant})
      elseif (${extension} STREQUAL .drawio)
        GenerateGraphics_drawio(${PA_TARGET} ${PA_SOURCE} ${output} ${variant})
      endif()
    elseif (${variant} STREQUAL "png")
      if (${extension} STREQUAL .tex)
        if (NOT TARGET "gen_${basename}.pdf")
          GenerateGraphics_tex2pdf(${PA_TARGET} ${PA_SOURCE} "${basename}.pdf")
        endif()
        GenerateGraphics_pdf2png(${PA_TARGET} "${basename}.pdf" ${output})
      elseif (${extension} STREQUAL .svg)
        GenerateGraphics_svg2png(${PA_TARGET} ${PA_SOURCE} ${output})
      elseif (${extension} STREQUAL .puml)
        GenerateGraphics_puml(${PA_TARGET} ${PA_SOURCE} ${output} ${variant})
      elseif (${extension} STREQUAL .uxf)
        GenerateGraphics_uxf(${PA_TARGET} ${PA_SOURCE} ${output} ${variant})
      elseif (${extension} STREQUAL .drawio)
        GenerateGraphics_drawio(${PA_TARGET} ${PA_SOURCE} ${output} ${variant})
      endif()
    else()
      message(FATAL_ERROR "Unsupport target-format: ${variant}")
    endif()
  endforeach()
endfunction(generate_graphic)
