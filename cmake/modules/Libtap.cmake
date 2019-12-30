if(__UFW_Libtap)
  return()
endif()
set(__UFW_Libtap 1)

macro(add_libtap libtap_root)
  if ("${PROJECT_TARGET_CPU}" STREQUAL "native")
    include(InitialiseToolchain)
    initialise_toolchain()

    add_library(tap STATIC ${libtap_root}/tap.c)
    target_include_directories(tap PUBLIC ${libtap_root})

    # There are a couple of warnings in tap.c (mainly about unused parameters).
    # I won't bother fixing them in third-party software and since there is no
    # point in being distracted by them, I am not enabling strict warnings in
    # this code.
    #MakeStrictCompilerC(tap)

    include(SetupTargetCPU)
    set_target_cpu(tap)
  else()
    message(STATUS "The libtap library is only built on native targets")
  endif()
endmacro()
