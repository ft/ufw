if(__UFW_ArmCmsis)
  return()
endif()
set(__UFW_ArmCmsis 1)

function(add_arm_cmsis root)
  set(library "arm-cmsis-core-m")
  add_library(${library} INTERFACE)
  target_include_directories(${library} INTERFACE ${root}/CMSIS/Core/Include)
endfunction()
