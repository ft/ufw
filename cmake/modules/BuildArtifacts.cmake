# Copyright (c) 2019-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_BuildArtifacts)
  return()
endif()
set(__UFW_BuildArtifacts 1)

include(UFWTools)

function(build_artifacts source)
  cmake_parse_arguments(PA "" "SDK_DIR" "VARIANTS;WITHOUT_SECTIONS" ${ARGN})
  get_filename_component(basename ${source} NAME_WE)
  set(artifacts)
  if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()
  if (${TOOLCHAIN_ID} MATCHES "-arm$" OR ${TOOLCHAIN_ID} STREQUAL "ti-arm-clang")
    foreach (variant ${PA_VARIANTS})
      set(dest "${basename}.${variant}")

      if (${variant} STREQUAL "bin")
        set(__remove)
        if (PA_WITHOUT_SECTIONS)
          foreach (section ${PA_WITHOUT_SECTIONS})
            list(APPEND __remove "-R" "${section}")
          endforeach()
        endif()
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS $<TARGET_FILE:${source}>
          COMMENT "Building binary file: ${dest}"
          COMMAND ${OBJCOPY} ${__remove} -I elf32-little -O binary
                             $<TARGET_FILE:${source}>
                             ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})

      elseif (${TOOLCHAIN_ID} STREQUAL "ti-arm" AND ${variant} STREQUAL "hex")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${source}
          COMMENT "Building intel-hex file: ${dest}"
          COMMAND ${OBJDUMP} --intel
                             --diag_wrap=off
                             --quiet
                             --memwidth=8
                             --romwidth=32
                             -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
                             $<TARGET_FILE:${source}>)

      elseif (${TOOLCHAIN_ID} STREQUAL "ti-arm" AND ${variant} STREQUAL "oad")

        set(dest ${basename}.ti-oad.bin)
        if (NOT PA_SDK_DIR)
          message(FATAL_ERROR "build_artifacts, variant=oad: SDK_DIR not specifed.")
        endif()

        set(oad_image_tool "${PA_SDK_DIR}/tools/common/oad/oad_image_tool")
        if (NOT EXISTS "${oad_image_tool}")
          message(FATAL_ERROR "build_artifacts, variant=oad: ${oad_image_tool} does not exist")
        endif()

        set(oad_private_key "${PA_SDK_DIR}/tools/common/oad/private.pem")
        if (NOT EXISTS "${oad_private_key}")
          message(FATAL_ERROR "build_artifacts, variant=oad: ${oad_private_key} does not exist")
        endif()

        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${basename}.hex
          COMMENT "Building OAD-compatible binary file: ${dest}"
          COMMAND ${oad_image_tool} --verbose
                                    --HexPath1 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${basename}.hex
                                    --keyFile ${oad_private_key}
                                    -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${basename}.ti-oad
                                    ccs ${CMAKE_CURRENT_SOURCE_DIR} 7)

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
          COMMAND ${DISASSEMBLER} $<TARGET_FILE:${source}>
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
          COMMAND ${OBJDUMP} --boot --binary --sci8
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
          COMMENT "Building binary-image file: ${dest}"
          COMMAND objcopy --gap-fill 0xff -I ihex -O binary
                          ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${basename}.hex
                          ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest})

      elseif (${variant} STREQUAL "hex")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${source}
          COMMENT "Building intel-hex file: ${dest}"
          COMMAND ${OBJDUMP} --intel --romwidth 16 --byte --swapbytes
                             -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
                             $<TARGET_FILE:${source}>)

      elseif (${variant} STREQUAL "lst")
        add_custom_command(
          OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dest}
          DEPENDS ${source}
          COMMENT "Building disassembly file: ${dest}"
          COMMAND ${DISASSEMBLER} $<TARGET_FILE:${source}>
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
    DESTINATION latest
    COMPONENT ${UFW_INSTALL_COMPONENT})
endfunction(build_artifacts)
