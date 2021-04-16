if(__XilinxISEToolchain)
  return()
endif()
set(__XilinxISEToolchain 1)

function(xilinx_licence_file var)
  if (DEFINED ${var})
    return()
  endif()
  if (NOT ("$ENV{${var}}" STREQUAL ""))
    set(${var} $ENV{${var}} PARENT_SCOPE)
  else()
    set(${var} "/usr/share/xilinx-ise/xilinx.lic" PARENT_SCOPE)
  endif()
endfunction()

function(generate_project_file output sources)
    unset(prj_file_lines)

    set(ip_cores ${sources})
    set(vhdl_sources ${sources})
    set(verilog_sources ${sources})
    list(FILTER ip_cores INCLUDE REGEX "^.*\.xco$")
    list(FILTER vhdl_sources INCLUDE REGEX "^.*\.vhd$")
    list(FILTER verilog_sources INCLUDE REGEX "^.*\.v$")

    foreach(src ${vhdl_sources})
        set(prj_file_lines "${prj_file_lines}vhdl work \"${src}\"\n")
    endforeach()

    foreach(src ${verilog_sources})
        set(prj_file_lines "${prj_file_lines}verilog work \"${src}\"\n")
    endforeach()

    foreach(core ${ip_cores})
        get_filename_component(core_name ${core} NAME_WE)
        set(prj_file_lines "${prj_file_lines}vhdl work \"${CMAKE_CURRENT_BINARY_DIR}/ipcore_dir/${core_name}.vhd\"\n")
    endforeach()
    file(WRITE ${output} ${prj_file_lines})
endfunction()

# s. Xilinx UG628 and UG687
function(add_syntax_check top_module)
    cmake_parse_arguments(PA "" "VERBOSITY" "SOURCES" ${ARGN} )

    set(project_file "${CMAKE_CURRENT_BINARY_DIR}/syntax/${top_module}.prj")
    set(script "${CMAKE_CURRENT_BINARY_DIR}/syntax/${top_module}.xst")

    generate_project_file(${project_file} "${PA_SOURCES}")
    file(WRITE ${script} "elaborate\n-ifn ${project_file}\n-ifmt mixed")

    xilinx_licence_file(XILINXD_LICENSE_FILE)
    add_custom_target(syntax-check
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_XST}
        -intstyle ${PA_VERBOSITY}
        -ifn ${script}
        -ofn ${top_module}.stx
        DEPENDS
        ${sources}
        COMMENT
        "STX - Syntax check with the Xilinx synthesis tool"
        )
endfunction()

function(add_bitstream top_module uc_file)
    cmake_parse_arguments(PA "" "VERBOSITY" "SOURCES" ${ARGN} )

    set(required_vars
        top_module
        uc_file
        FPGA_PART
        XST_BUILD_SCRIPT_INPUT
        MAP_COMMAND_FILE
        PAR_COMMAND_FILE
        NGD_COMMAND_FILE
        TWR_COMMAND_FILE
        BITGEN_COMMAND_FILE
        XIL_TCLSH
        XIL_COREGEN
        XIL_XST
        XIL_NGDBUILD
        XIL_MAP
        XIL_PAR
        XIL_TRCE
        XIL_BITGEN
        XIL_PROMGEN
        XIL_IMPACT
        )

    foreach(var ${required_vars})
        if(NOT DEFINED ${var})
            message(SEND_ERROR "Required variable ${var} not set!")
        endif()
    endforeach()

    if(NOT PA_VERBOSITY)
        set(PA_VERBOSITY silent)
    endif()

    set(xst_build_script "${CMAKE_CURRENT_BINARY_DIR}/${top_module}.xst")
    set(project_file "${CMAKE_CURRENT_BINARY_DIR}/${top_module}.prj")
    set(output_bn "${CMAKE_CURRENT_BINARY_DIR}/${top_module}")

    configure_file(${XST_BUILD_SCRIPT_INPUT} ${xst_build_script} @ONLY)
    generate_project_file(${project_file} "${PA_SOURCES}")

    set(ip_cores ${PA_SOURCES})
    set(vhdl_sources ${PA_SOURCES})
    set(verilog_sources ${PA_SOURCES})
    list(FILTER ip_cores INCLUDE REGEX "^.*\.xco$")
    list(FILTER vhdl_sources INCLUDE REGEX "^.*\.vhd$")
    list(FILTER verilog_sources INCLUDE REGEX "^.*\.v$")

    foreach(core ${ip_cores})
        get_filename_component(core_name ${core} NAME_WE)
        list(APPEND vhdl_sources "${CMAKE_CURRENT_BINARY_DIR}/ipcore_dir/${core_name}.vhd")
    endforeach()

    xilinx_licence_file(XILINXD_LICENSE_FILE)
    add_custom_command(OUTPUT
        ${output_bn}.ngc
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_XST}
        -intstyle ${PA_VERBOSITY}
        -ifn ${xst_build_script}
        -ofn ${output_bn}.syr
        BYPRODUCTS
        ${output_bn}.syr
        DEPENDS
        ${xst_build_script}
        ${ip_cores}
        ${vhdl_sources}
        ${verilog_sources}
        COMMENT
        "XST - Xilinx synthesis tool to generate the netlist file"
        )

    if (PA_VERBOSITY STREQUAL silent)
        set(ngc_ngd_build_verbosity "-quiet")
    else ()
        set(ngc_ngd_build_verbosity "-verbose")
    endif ()

    add_custom_command(OUTPUT
        ${output_bn}.ngd
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_NGDBUILD}
        -intstyle ${PA_VERBOSITY}
        ${ngc_ngd_build_verbosity}              # ngcbuild and ngdbuild PA_VERBOSITY
        -p ${FPGA_PART}                         # FPGA part number
        -f ${NGD_COMMAND_FILE}                  # FPGA specific configuration
        -uc ${uc_file}                          # User constraints input file
        ${output_bn}.ngc                       # Netlist input file
        ${output_bn}.ngd                       # Native generic database (NGD) output file
        DEPENDS
        ${output_bn}.ngc
        ${uc_file}
        COMMENT
        "NGD - Generate the native generic database from the netlist file"
        )

    add_custom_command(OUTPUT
        ${output_bn}_map.ncd
        ${output_bn}.pcf
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_MAP}
        -intstyle ${PA_VERBOSITY}
        -w                          # Overwrite existing output files
        -p ${FPGA_PART}             # FPGA part number
        -f ${MAP_COMMAND_FILE}      # FPGA specific configuration
        -o ${output_bn}_map.ncd    # Mapped native circuit description (NCD) output file
        ${output_bn}.ngd           # NGD input file
        ${output_bn}.pcf           # Physical constraints output file
        DEPENDS
        ${output_bn}.ngd
        COMMENT
        "MAP - Map the logical design to the FPGA and generate physical constraints"
        )

    add_custom_command(OUTPUT
        ${output_bn}.ncd
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_PAR}
        -intstyle ${PA_VERBOSITY}
        -w                          # Overwrite existing output files
        -f ${PAR_COMMAND_FILE}      # FPGA specific configuration
        ${output_bn}_map.ncd       # Mapped NCD input file
        ${output_bn}.ncd           # Placed and routed NCD output file
        ${output_bn}.pcf           # PCF constraints file
        DEPENDS
        ${output_bn}_map.ncd
        ${output_bn}.pcf
        COMMENT
        "PAR - Place and route"
        )

    add_custom_command(OUTPUT
        ${output_bn}.twr
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_TRCE}
        -intstyle ${PA_VERBOSITY}
        -f ${TWR_COMMAND_FILE}  # FPGA specific configuration
        -xml ${output_bn}.twx  # Report file for the timing Analyzer GUI
        ${output_bn}.ncd       # NCD input file
        -o ${output_bn}.twr    # Timing report file
        ${output_bn}.pcf       # PCF constraints file
        DEPENDS
        ${output_bn}.ncd
        ${output_bn}.pcf
        COMMENT
        "TRACE - Evaluate circuit timing"
        )

    add_custom_command(OUTPUT
        ${output_bn}.bit
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_BITGEN}
        -intstyle ${PA_VERBOSITY}
        -w                              # Overwrite existing output files
        -f ${BITGEN_COMMAND_FILE}       # Configuration, s. UG628, p. 227 ff. and in build/<project>.ut
        ${output_bn}.ncd               # NCD input file
        ${output_bn}.bit               # Bitstream output file
        ${output_bn}.pcf               # PCF constraints file
        DEPENDS
        ${output_bn}.twr
        ${output_bn}.ncd
        ${output_bn}.pcf
        COMMENT
        "BIT - Generate bitstream"
        )
endfunction()

function(add_bin input_files output)
    cmake_parse_arguments(PA "WITHOUT_DEPENDENCIES" "" "" ${ARGN})
    set(deps)
    if (NOT PA_WITHOUT_DEPENDENCIES)
      set(deps ${input_files})
    endif()
    xilinx_licence_file(XILINXD_LICENSE_FILE)
    add_custom_command(OUTPUT
        ${output}
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_PROMGEN}
        -intstyle ${XILINX_VERBOSITY}
        -w                              # Overwrite existing output files
        -p bin                          # PROM format
        -b                              # -b or -spi to disable bit swapping
        -u 0                            # Load direction upwards
        ${input_files}
        -o "${output}"
        DEPENDS
        ${deps}
        COMMENT
        "PROMGen - Generate binary files"
        )
endfunction()

function(add_mcs source dest prom)
    xilinx_licence_file(XILINXD_LICENSE_FILE)
    add_custom_command(OUTPUT
        ${dest}
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_PROMGEN}
        -intstyle ${XILINX_VERBOSITY}
        -w                              # Overwrite existing output files
        -p mcs                          # PROM format
        -c FF                           # Checksum
        -o ${dest}                      # Output file base name
        -x ${prom}                      # Xilinx PROM, xcf04s on Nexys 2
        -u 0                            # Load direction upwards
        ${source}
        DEPENDS
        ${source}
        COMMENT
        "MCS - Generate Intel Micro Computer Set-86 PROM file"
        )
endfunction()

function(program_jtag file)
    if(NOT DEFINED IMPACT_JTAG_SCRIPT_INPUT)
        message(SEND_ERROR "Board variable IMPACT_JTAG_SCRIPT_INPUT not set!")
    endif()

    set(impact_jtag_script
        "${CMAKE_CURRENT_BINARY_DIR}/impact_program_jtag.cmd")
    configure_file(${IMPACT_JTAG_SCRIPT_INPUT} ${impact_jtag_script} @ONLY)

    xilinx_licence_file(XILINXD_LICENSE_FILE)
    add_custom_target(program-jtag
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_IMPACT}
        -batch ${impact_jtag_script}
        DEPENDS
        ${impact_jtag_script}
        ${file}
        COMMENT
        "IMPACT - Program the ${file} via JTAG"
        )
endfunction()

function(program_prom file)
    if(NOT DEFINED IMPACT_PROM_SCRIPT_INPUT)
        message(SEND_ERROR "Board variable IMPACT_PROM_SCRIPT_INPUT not set!")
    endif()

    set(impact_prom_script
        "${CMAKE_CURRENT_BINARY_DIR}/impact_program_prom.cmd")
    configure_file(${IMPACT_PROM_SCRIPT_INPUT} ${impact_prom_script} @ONLY)

    xilinx_licence_file(XILINXD_LICENSE_FILE)
    add_custom_target(program-prom
        COMMAND
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_IMPACT}
        -batch ${impact_prom_script}
        DEPENDS
        ${impact_prom_script}
        ${file}
        COMMENT
        "IMPACT - Program the ${file} PROM"
        )
endfunction()

function(add_ipcore ip_core dest)
    get_filename_component(project ${ip_core} DIRECTORY)
    get_filename_component(core_name ${ip_core} NAME_WE)

    configure_file("${project}/coregen.cgp"
        "${dest}/coregen.cgp" COPYONLY)

    set(deps)
    foreach (dep ${ARGN})
      get_filename_component(name ${dep} NAME_WE)
      list(APPEND deps ${dest}/${name}.vhd)
    endforeach()

    xilinx_licence_file(XILINXD_LICENSE_FILE)
    add_custom_command(OUTPUT ${dest}/${core_name}.vhd
        COMMAND
        [ -f ${dest}/${core_name}.vhd ] ||
        ${CMAKE_COMMAND} -E env --unset=_JAVA_OPTIONS
        ${CMAKE_COMMAND} -E env XILINXD_LICENSE_FILE="${XILINXD_LICENSE_FILE}"
        ${XIL_COREGEN}
        -p ${dest}
        -b ${ip_core}
        -intstyle xflow
        DEPENDS
        ${dest}/coregen.cgp
        ${ip_core}
        ${deps})
endfunction()

function(add_ipcores dest)
  set(prev)
  foreach (core ${ARGN})
    if ("${prev}" STREQUAL "")
      add_ipcore(${core} ${dest})
    else()
      add_ipcore(${core} ${dest} ${prev})
    endif()
    set(prev ${core})
  endforeach()
endfunction()
