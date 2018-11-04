if (${CMAKE_CROSSCOMPILING})
  return()
endif()

find_package(GTest)

if (GTest_FOUND)
  add_executable(TrivialValidator TrivialValidator.cpp)
  target_link_libraries(TrivialValidator GTest::GTest GTest::Main)
endif()
