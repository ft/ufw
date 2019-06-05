if(__UFW_SetupUFW)
  return()
endif()
set(__UFW_SetupUFW 1)

macro(setup_ufw ufw_root)
  set(MICROFRAMEWORK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${ufw_root}")
  add_subdirectory(${ufw_root})
endmacro()
