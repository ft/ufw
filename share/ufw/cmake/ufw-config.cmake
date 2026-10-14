macro(ufw_pkg_message)
  if (NOT ufw_FIND_QUIETLY)
    message(${ARGN})
  endif()
endmacro()

ufw_pkg_message(STATUS "ufw: Found package data at ${ufw_DIR}")
ufw_pkg_message(STATUS "ufw: Locating project root directory...")

set(UFW_PACKAGE_RELATIVE "../../..")
get_filename_component(MICROFRAMEWORK_ROOT
  ${ufw_DIR}/${UFW_PACKAGE_RELATIVE}
  ABSOLUTE)

ufw_pkg_message(STATUS "ufw: MICROFRAMEWORK_ROOT at ${MICROFRAMEWORK_ROOT}")

set(ufw_core_FOUND  False)
set(ufw_cmake_FOUND False)

if (ufw_FIND_COMPONENTS)
  ufw_pkg_message(STATUS "ufw: Processing COMPONENTS specification:")
  foreach(component IN LISTS ufw_FIND_COMPONENTS)
    if (component STREQUAL "core")
      ufw_pkg_message(STATUS "ufw:   Enabling ${component}")
      set(ufw_core_FOUND True)
    elseif (component STREQUAL "cmake")
      ufw_pkg_message(STATUS "ufw:   Enabling ${component}")
      set(ufw_cmake_FOUND True)
    else()
      ufw_pkg_message(WARNING "ufw: Unknown component: ${component}")
      set(MyPackage_${component}_FOUND False)
    endif()
  endforeach()
else()
  set(ufw_core_FOUND True)
  set(ufw_cmake_FOUND True)
endif()

if (ufw_core_FOUND AND (NOT ufw_cmake_FOUND))
  ufw_pkg_message(WARNING "ufw: core used without cmake, forcing cmake!")
  set(ufw_cmake_FOUND True)
endif()

if (ufw_cmake_FOUND)
  ufw_pkg_message(STATUS "ufw: Configuring CMAKE_MODULE_PATH...")
  list(INSERT CMAKE_MODULE_PATH 0 ${MICROFRAMEWORK_ROOT}/cmake/modules)
endif()

if (ufw_core_FOUND)
  include(SetupUFW)
  ufw_pkg_message(STATUS "ufw: Setting up via setup_ufw(PACKAGE)...")
  setup_ufw(PACKAGE)
endif()
