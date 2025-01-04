# If this ufw is built as a zephyr module. Override cmake cache values with
# data from Kconfig now.
if (ZEPHYR_CURRENT_MODULE_NAME AND ("${ZEPHYR_CURRENT_MODULE_NAME}" STREQUAL "ufw"))
  include(UFWTools)

  # UFW_USE_BUILTIN_SWAP
  set(_tmp OFF)
  if (CONFIG_UFW_USE_BUILTIN_SWAP)
    set(_tmp ON)
  endif()
  set(UFW_USE_BUILTIN_SWAP "${_tmp}"
    CACHE BOOL "Use __builtin_bswapXX() if available." FORCE)

  # UFW_WITH_EP_CORE_TRACE
  set(_tmp OFF)
  if (CONFIG_UFW_WITH_EP_CORE_TRACE)
    set(_tmp ON)
  endif()
  set(UFW_WITH_EP_CORE_TRACE "${_tmp}"
    CACHE BOOL "Enable printf() trace in endpoints/core.c" FORCE)

  # UFW_WITH_RUNTIME_ASSERT
  set(_tmp OFF)
  if (CONFIG_UFW_WITH_RUNTIME_ASSERT)
    set(_tmp ON)
  endif()
  set(UFW_WITH_RUNTIME_ASSERT "${_tmp}"
    CACHE BOOL "Enable assert() in ufw" FORCE)

  # UFW_PRIVATE_ERRNO_OFFSET
  if (CONFIG_UFW_PRIVATE_ERRNO_OFFSET)
    set(UFW_PRIVATE_ERRNO_OFFSET "${CONFIG_UFW_PRIVATE_ERRNO_OFFSET}"
      CACHE STRING "Offset for errno-extensions." FORCE)
  endif()

  # UFW_ENABLE_COVERAGE
  set(_tmp OFF)
  if (CONFIG_UFW_ENABLE_COVERAGE)
    set(_tmp ON)
  endif()
  set(UFW_ENABLE_COVERAGE "${_tmp}"
    CACHE BOOL "Enable coverage tracking in toolchain" FORCE)

  # UFW_INSTALL_COMPONENT
  set(_tmp OFF)
  if (CONFIG_UFW_INSTALL_COMPONENT)
    set(_tmp ON)
  endif()
  set(UFW_INSTALL_COMPONENT "${_tmp}"
    CACHE BOOL "CMake Installation Component name for ufw installation items."
    FORCE)

  # GENERATE_API_DOCUMENTATION
  set(_tmp OFF)
  if (CONFIG_GENERATE_API_DOCUMENTATION)
    set(_tmp ON)
  endif()
  set(GENERATE_API_DOCUMENTATION "${_tmp}"
    CACHE BOOL "Generate API Documentation" FORCE)
endif()
