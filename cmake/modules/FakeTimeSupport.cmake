# Copyright (c) 2021-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_FakeTimeSupport)
  return()
endif()
set(__UFW_FakeTimeSupport 1)

find_program(FAKETIME_PROGRAM faketime)
set(FAKETIME_TIMESTAMP "2020-01-01 00:00:01"
    CACHE STRING "Timestamp Argument for faketime Program")

set(FAKETIME_ACTIVE FALSE CACHE BOOL "Enable the use of faketime")

function(make_faketime_prefix var)
  if (FAKETIME_ACTIVE AND (NOT (FAKETIME_PROGRAM STREQUAL "FAKETIME_PROGRAM-NOTFOUND")))
    set(${var} ${FAKETIME_PROGRAM} -f "${FAKETIME_TIMESTAMP}" PARENT_SCOPE)
  else()
    set(${var} PARENT_SCOPE)
  endif()
endfunction()
