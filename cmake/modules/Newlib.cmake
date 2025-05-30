# Copyright (c) 2020-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_Newlib)
  return()
endif()
set(__UFW_Newlib 1)

function(newlib_target_spec target spec)
  set(_spec "--specs=${spec}.specs")
  if (${spec} STREQUAL rdimon)
    list(APPEND _spec -Wl,--start-group -lc -lrdimon -lgcc -Wl,--end-group)
  endif()
  if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13.0")
    target_link_options(${target} PUBLIC ${_spec})
  else()
    target_link_libraries(${target} PUBLIC ${_spec})
  endif()
endfunction()
