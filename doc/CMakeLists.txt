option(GENERATE_API_DOCUMENTATION "Generate API Documentation" OFF)
find_package(Doxygen)

if (DOXYGEN_FOUND)
  if (GENERATE_API_DOCUMENTATION)
    message(STATUS "Setting up API documentation generation")
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT})

    add_custom_target(ufw-api-documentation ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  endif()
else()
  message(STATUS "CMake Doxygen Module missing, cannot build API docs.")
endif()
