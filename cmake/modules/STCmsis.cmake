if(__UFW_STCmsis)
  return()
endif()
set(__UFW_STCmsis 1)

include(UFWTools)

function(add_st_cmsis root)
  ufw_get_property(board-objects BOARD_CPU_FAMILY cpu)
  set(library "st-cmsis-${cpu}")
  add_library(${library} INTERFACE)
  target_include_directories(${library} INTERFACE ${root}/Include)
  target_link_libraries(${library} INTERFACE arm-cmsis-core-m)
endfunction()
