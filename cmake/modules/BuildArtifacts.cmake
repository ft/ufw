if(__UFW_BuildArtifacts)
  return()
endif()
set(__UFW_BuildArtifacts 1)

function(build_artifacts source)
  cmake_parse_arguments(PA "" "" "VARIANTS" ${ARGN})
  get_filename_component(basename ${source} NAME_WE)
  set(artifacts)
  if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()
  if (${TOOLCHAIN_ID} MATCHES "-arm$")
    foreach (variant ${PA_VARIANTS})
      set(dest "${basename}.${variant}")

      if (${variant} STREQUAL "bin")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS $<TARGET_FILE:${source}>
          COMMENT "Building binary file: ${dest}"
          COMMAND ${OBJCOPY} -I elf32-little -O binary
                             $<TARGET_FILE:${source}>
                             ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})

      elseif (${TOOLCHAIN_ID} STREQUAL "ti-arm" AND ${variant} STREQUAL "hex")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${source}
          COMMENT "Building intel-hex file: ${dest}"
          COMMAND ${TI_OBJDUMP} --intel
                                --diag_wrap=off
                                --quiet
                                --memwidth=8
                                --romwidth=32
                                -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
                                $<TARGET_FILE:${source}>)

      elseif (${variant} STREQUAL "hex")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS $<TARGET_FILE:${source}>
          COMMENT "Building intel-hex file: ${dest}"
          COMMAND ${OBJCOPY} -O ihex
                             $<TARGET_FILE:${source}>
                             ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})

      elseif (${variant} STREQUAL "srec")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS $<TARGET_FILE:${source}>
          COMMENT "Building motorola-s-record file: ${dest}"
          COMMAND ${OBJCOPY} -O srec
                             $<TARGET_FILE:${source}>
                             ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})

      elseif (${TOOLCHAIN_ID} STREQUAL "ti-arm" AND ${variant} STREQUAL "lst")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${source}
          COMMENT "Building disassembly file: ${dest}"
          COMMAND ${TI_DIS} $<TARGET_FILE:${source}>
                            > ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})
      elseif (${variant} STREQUAL "lst")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS $<TARGET_FILE:${source}>
          COMMENT "Building disassembly file: ${dest}"
          COMMAND ${OBJDUMP} -S $<TARGET_FILE:${source}>
                             > ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})

      else ()
        message(FATAL_ERROR "Unknown artifact variant: ${variant}")
      endif()

      list(APPEND artifacts ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})
    endforeach()
  elseif (${TOOLCHAIN_ID} MATCHES "-c2000$")
    foreach (variant ${PA_VARIANTS})
      set(dest "${basename}.${variant}")

      if (${variant} STREQUAL "b00")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${source}
          COMMENT "Building bootloader file: ${dest}"
          COMMAND ${TI_OBJDUMP} --boot --binary --sci8
                                -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
                                $<TARGET_FILE:${source}>)

      elseif (${variant} STREQUAL "bin")
        list(FIND PA_VARIANTS hex __with_hex)
        if (__with_hex LESS 0)
          message(FATAL_ERROR "Building binary files requires hex file generation in ${TOOLCHAIN_ID}!")
        endif()
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${basename}.hex
          COMMENT "Building bootloader file: ${dest}"
          COMMAND objcopy --gap-fill 0xff -I ihex -O binary
                          ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${basename}.hex
                          ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})

      elseif (${variant} STREQUAL "hex")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${source}
          COMMENT "Building intel-hex file: ${dest}"
          COMMAND ${TI_OBJDUMP} --intel --romwidth 16 --byte --swapbytes
                                -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
                                $<TARGET_FILE:${source}>)

      elseif (${variant} STREQUAL "lst")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${source}
          COMMENT "Building disassembly file: ${dest}"
          COMMAND ${TI_DIS} $<TARGET_FILE:${source}>
                            > ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})

      else ()
        message(FATAL_ERROR "Unknown artifact variant: ${variant}")
      endif()

      list(APPEND artifacts ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})
    endforeach()
  else ()
    message(FATAL_ERROR "Unsupported TOOLCHAIN_ID: ${TOOLCHAIN_ID}")
  endif()
  add_custom_target(${basename}-formats ALL DEPENDS ${artifacts})
  install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${source} ${artifacts}
          DESTINATION latest)
endfunction(build_artifacts)
