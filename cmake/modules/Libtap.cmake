if(__UFW_Libtap)
  return()
endif()
set(__UFW_Libtap 1)

function(add_libtap _dir)
  add_library(tap "${_dir}/tap.c")
  target_include_directories(tap PUBLIC "${_dir}")
endfunction()
