
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED SimpleITK_DIR AND NOT EXISTS ${SimpleITK_DIR})
  message(FATAL_ERROR "SimpleITK_DIR variable is defined but corresponds to non-existing directory")
endif()

if(WIN32 AND MSVC)
  if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0.30729.1" )
    message( WARNING "SimpleITK requires Microsoft Visual Studio 2008 (VS9) SP1 or greater!"
      "http://www.itk.org/Wiki/SimpleITK/FAQ#How_do_I_build_with_Visual_Studio_2008.3F" )
  endif()
endif()

# Set dependency list
set(SimpleITK_DEPENDENCIES ITKv4 Swig python)

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(SimpleITK)

#
#  SimpleITK externalBuild
#
include(ExternalProject)

set(EXTERNAL_PROJECT_OPTIONAL_ARGS)

# Set CMake OSX variable to pass down the external project
if(APPLE)
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

if(NOT CMAKE_CONFIGURATION_TYPES)
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE})
endif()

if(APPLE)
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DPYTHON_EXECUTABLE:PATH=${slicer_PYTHON_EXECUTABLE}
    -DPYTHON_FRAMEWORKS:PATH=${slicer_PYTHON_FRAMEWORK}
    -DPYTHON_LIBRARY:FILEPATH=${slicer_PYTHON_LIBRARY}
    -DPYTHON_INCLUDE_DIR:PATH=${slicer_PYTHON_INCLUDE}
    )
else()
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DPYTHON_EXECUTABLE:PATH=${slicer_PYTHON_EXECUTABLE}
    -DPYTHON_LIBRARY:FILEPATH=${slicer_PYTHON_LIBRARY}
    -DPYTHON_INCLUDE_DIR:PATH=${slicer_PYTHON_INCLUDE}
    )
endif()

configure_file(SuperBuild/SimpleITK_install_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/SimpleITK_install_step.cmake
  @ONLY)

set(SimpleITK_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/SimpleITK_install_step.cmake)


ExternalProject_add(SimpleITK
  SOURCE_DIR SimpleITK
  BINARY_DIR SimpleITK-build
  GIT_REPOSITORY ${git_protocol}://itk.org/SimpleITK.git
  GIT_TAG v0.6.1
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
    # SimpleITK does not work with shared libs turned on
    -DBUILD_SHARED_LIBS:BOOL=OFF
    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}
    -DITK_DIR:PATH=${ITK_DIR}
    -DBUILD_EXAMPLES:BOOL=OFF
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
    ${EXTERNAL_PROJECT_OPTIONAL_ARGS}
  #
  INSTALL_COMMAND ${SimpleITK_INSTALL_COMMAND}
  #
  DEPENDS ${SimpleITK_DEPENDENCIES}
  )
