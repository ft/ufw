set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR c2000)

# TODO: Much too simplistic, but works on my system. Good enough for a proof of concept
set(CMAKE_SYSROOT /opt/ti/ccsv7/tools/compiler/ti-cgt-c2000_16.9.6.LTS)

set(CMAKE_C_COMPILER cl2000)
set(CMAKE_CXX_COMPILER cl2000)

string(APPEND CMAKE_C_FLAGS_INIT " --c99 -I${CMAKE_SYSROOT}/include")
string(APPEND CMAKE_CXX_FLAGS_INIT " --c++03 -I${CMAKE_SYSROOT}/include")
