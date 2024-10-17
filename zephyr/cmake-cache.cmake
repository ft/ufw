# If this ufw is built as a zephyr module. Override cmake cache values with
# data from Kconfig now.
if (ZEPHYR_CURRENT_MODULE_NAME AND ("${ZEPHYR_CURRENT_MODULE_NAME}" STREQUAL "ufw"))
  set(_tmp OFF)
  if (CONFIG_UFW_USE_BUILTIN_SWAP)
    set(_tmp ON)
  endif()
  set(UFW_USE_BUILTIN_SWAP "${_tmp}"
    CACHE BOOL "Use __builtin_bswapXX() if available." FORCE)

  if (CONFIG_UFW_PRIVATE_ERRNO_OFFSET)
    set(UFW_PRIVATE_ERRNO_OFFSET "${CONFIG_UFW_PRIVATE_ERRNO_OFFSET}"
      CACHE STRING "Offset for errno-extensions." FORCE)
  endif()
endif()
