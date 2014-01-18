
set(proj SimpleITK)

# Set dependency list
set(${proj}_DEPENDENCIES ITKv4 Swig python)

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED SimpleITK_DIR AND NOT EXISTS ${SimpleITK_DIR})
  message(FATAL_ERROR "SimpleITK_DIR variable is defined but corresponds to non-existing directory")
endif()

if(NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  configure_file(SuperBuild/SimpleITK_install_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SimpleITK_install_step.cmake
    @ONLY)

  set(SimpleITK_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/SimpleITK_install_step.cmake)

  set(SimpleITK_REPOSITORY ${git_protocol}://itk.org/SimpleITK.git)
  set(SimpleITK_GIT_TAG 83c344feff4b085df0879fd259337740f3f38c5d) # Patched v0.7.1

  ExternalProject_add(SimpleITK
    ${${proj}_EP_ARGS}
    SOURCE_DIR SimpleITK
    BINARY_DIR SimpleITK-build
    GIT_REPOSITORY ${SimpleITK_REPOSITORY}
    GIT_TAG ${SimpleITK_GIT_TAG}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DBUILD_SHARED_LIBS:BOOL=${Slicer_USE_SimpleITK_SHARED}
      -DSimpleITK_INSTALL_ARCHIVE_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      -DSimpleITK_INSTALL_LIBRARY_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      -DSITK_INT64_PIXELIDS:BOOL=OFF
      -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}
      -DITK_DIR:PATH=${ITK_DIR}
      -DPYTHON_EXECUTABLE:PATH=${PYTHON_EXECUTABLE}
      -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
      -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR}
      -DBUILD_TESTING:BOOL=OFF
      -DBUILD_DOXYGEN:BOOL=OFF
      -DWRAP_PYTHON:BOOL=ON
      -DWRAP_TCL:BOOL=OFF
      -DWRAP_JAVA:BOOL=OFF
      -DWRAP_RUBY:BOOL=OFF
      -DWRAP_LUA:BOOL=OFF
      -DWRAP_CSHARP:BOOL=OFF
      -DWRAP_R:BOOL=OFF
      -DSWIG_EXECUTABLE:PATH=${SWIG_EXECUTABLE}
      -DSimpleITK_BUILD_DISTRIBUTE:BOOL=ON # Shorten version and install path removing -g{GIT-HASH} suffix.
    #
    INSTALL_COMMAND ${SimpleITK_INSTALL_COMMAND}
    #
    DEPENDS ${${proj}_DEPENDENCIES}
    )
  set(SimpleITK_DIR ${CMAKE_BINARY_DIR}/SimpleITK-build)

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(
  VARS SimpleITK_DIR:PATH
  LABELS "FIND_PACKAGE"
  )
