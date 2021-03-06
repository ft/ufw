if(__UFW_GenerateGraphics)
  return()
endif()
set(__UFW_GenerateGraphics 1)

function(GenerateGraphics_svg2pdf target source output)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND inkscape --export-pdf ${output} ${CMAKE_CURRENT_SOURCE_DIR}/${source})
endfunction(GenerateGraphics_svg2pdf)

function(GenerateGraphics_svg2png target source output)
  add_custom_target(gen_${output} DEPENDS ${output})
  add_dependencies(${target} gen_${output})
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${source}
    COMMENT "Building graphic file: ${output}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND inkscape --export-png ${output} ${CMAKE_CURRENT_SOURCE_DIR}/${source})
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

set(GenerateGraphics_supported_sources .svg .tex .puml .uxf)

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
      endif()
    else()
      message(FATAL_ERROR "Unsupport target-format: ${variant}")
    endif()
  endforeach()
endfunction(generate_graphic)
