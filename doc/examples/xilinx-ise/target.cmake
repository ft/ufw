set(__fpga_device xc6slx45)
set(__fpga_package csg324)
set(__fpga_speed 3)
set(FPGA_PART ${__fpga_device}-${__fpga_package}-${__fpga_speed})
set(XTCL_PROJECT_PROPERTIES
    "${BOARDS_ROOT}/${TARGET_BOARD}/tcl/project_properties.tcl")
set(XTCL_PROCESS_PROPERTIES
    "${BOARDS_ROOT}/${TARGET_BOARD}/tcl/process_properties.tcl")
set(XST_BUILD_SCRIPT_INPUT
    "${BOARDS_ROOT}/${TARGET_BOARD}/xst/synthesis_script.xst.in")

set(NGD_COMMAND_FILE "${BOARDS_ROOT}/${TARGET_BOARD}/cmd/ngd_command_file.cmd")
set(MAP_COMMAND_FILE "${BOARDS_ROOT}/${TARGET_BOARD}/cmd/map_command_file.cmd")
set(PAR_COMMAND_FILE "${BOARDS_ROOT}/${TARGET_BOARD}/cmd/par_command_file.cmd")
set(TWR_COMMAND_FILE "${BOARDS_ROOT}/${TARGET_BOARD}/cmd/twr_command_file.cmd")
set(BITGEN_COMMAND_FILE
    "${BOARDS_ROOT}/${TARGET_BOARD}/cmd/bitgen_command_file.cmd")
set(IMPACT_JTAG_SCRIPT_INPUT
    "${BOARDS_ROOT}/${TARGET_BOARD}/cmd/impact_program_jtag.cmd.in")
