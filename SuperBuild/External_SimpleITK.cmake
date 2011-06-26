#
#  SimpleITK externalBuild
#
include(ExternalProject)

set(LOWERCASE_CMAKE_PROJECT_NAME slicer)

#if(APPLE)
#    set(SIMPLEITK_PYTHON_ARGS
#      -DPYTHON_EXECUTABLE:PATH=${${LOWERCASE_CMAKE_PROJECT_NAME}_PYTHON_EXECUTABLE}
#      -DPYTHON_FRAMEWORKS:PATH=${${LOWERCASE_CMAKE_PROJECT_NAME}_PYTHON_FRAMEWORK}
#      -DPYTHON_LIBRARY:FILEPATH=${${LOWERCASE_CMAKE_PROJECT_NAME}_PYTHON_LIBRARY}
#      -DPYTHON_INCLUDE_DIR:PATH=${${LOWERCASE_CMAKE_PROJECT_NAME}_PYTHON_INCLUDE}
#      )
#else()
message(STATUS "Slicer --- ${LOWERCASE_CMAKE_PROJECT_NAME} ${${LOWERCASE_CMAKE_PROJECT_NAME}_PYTHON_EXECUTABLE}!!!")
    set(SIMPLEITK_PYTHON_ARGS
      -DPYTHON_EXECUTABLE:PATH=${${LOWERCASE_CMAKE_PROJECT_NAME}_PYTHON_EXECUTABLE}
      -DPYTHON_LIBRARY:FILEPATH=${${LOWERCASE_CMAKE_PROJECT_NAME}_PYTHON_LIBRARY}
      -DPYTHON_INCLUDE_DIR:PATH=${${LOWERCASE_CMAKE_PROJECT_NAME}_PYTHON_INCLUDE}
      )
#endif()

ExternalProject_add(SimpleITK
  SOURCE_DIR SimpleITK
  BINARY_DIR SimpleITK-build
  GIT_REPOSITORY https://github.com/hjmjohnson/SimpleITK.git
  #  GIT_TAG NeverBranchFromThisTest
  GIT_TAG b4_sitk
  #GIT_TAG master
  UPDATE_COMMAND ""
  CMAKE_ARGS
    ${ep_common_flags}
  # SimpleITK does not work with shared libs turned on
  -DBUILD_SHARED_LIBS:BOOL=OFF
  -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}
  -DITK_DIR:PATH=${ITK_DIR}
  -DBUILD_EXAMPLES:BOOL=ON
  -DBUILD_TESTING:BOOL=ON
  -DBUILD_DOXYGEN:BOOL=OFF
  -DWRAP_PYTHON:BOOL=ON
  -DWRAP_TCL:BOOL=OFF
  -DWRAP_JAVA:BOOL=OFF
  -DWRAP_RUBY:BOOL=OFF
  -DWRAP_LUA:BOOL=OFF
  ${SIMPLEITK_PYTHON_ARGS}
  -DSWIG_EXECUTABLE:PATH=${SWIG_EXECUTABLE}
  #INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ITKv4 Swig
  #
  # NO INSTALL COMMAND YET!
  #INSTALL_COMMAND ${CMAKE_COMMAND} -Dsrc=<BINARY_DIR>/bin -Dprefix=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_LIST_DIR}/SimpleITKInstall.cmake
  INSTALL_COMMAND ""
  # CONFIGURE_COMMAND ../HDF5/configure
  # --prefix=${CMAKE_CURRENT_BINARY_DIR}
  # --enable-cxx
)

