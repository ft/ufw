# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

cmake_minimum_required (VERSION 3.13.4)

if ((NOT "${PROJECT_TARGET_CPU}" STREQUAL "native")
    OR (DEFINED UFW_ZEPHYR_KERNEL))
  return()
endif()

set(examples
  ex-regp-parse-frame
  ex-rfc1055-parse-frame)

foreach (example ${examples})
  add_executable(${example} ${example}.c)
  target_link_libraries(${example} PRIVATE ufw-nosan -static)
  MakeStrictCompilerC(${example})
  set_target_cpu(${example})
endforeach()
