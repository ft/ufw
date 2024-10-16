# This makes ufw inherit the toolchain setup of the Zephyr build system.
foreach (_lib ${__ufw_targets})
  message(STATUS "ufw,zephyr: Toolchain setup for ${_lib}")
  target_link_libraries("${_lib}" PRIVATE zephyr_interface)
endforeach()
