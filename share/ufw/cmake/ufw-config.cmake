message(STATUS "ufw: Found package data at ${ufw_DIR}")
message(STATUS "ufw: Locating project root directory...")

set(UFW_PACKAGE_RELATIVE "../../..")
get_filename_component(MICROFRAMEWORK_ROOT
  ${ufw_DIR}/${UFW_PACKAGE_RELATIVE}
  ABSOLUTE)

message(STATUS "ufw: MICROFRAMEWORK_ROOT at ${MICROFRAMEWORK_ROOT}")
message(STATUS "ufw: Configuring CMAKE_MODULE_PATH...")
list(INSERT CMAKE_MODULE_PATH 0 ${MICROFRAMEWORK_ROOT}/cmake/modules)
include(SetupUFW)
message(STATUS "ufw: Setting up via setup_ufw(PACKAGE)...")
setup_ufw(PACKAGE)
