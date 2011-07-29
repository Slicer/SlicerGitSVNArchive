
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED ITK_DIR AND NOT EXISTS ${ITK_DIR})
  message(FATAL_ERROR "ITK_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set dependency list
set(ITKv3_DEPENDENCIES "")

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(ITKv3)
set(proj ITKv3)

# Set CMake OSX variable to pass down the external project
set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
if(APPLE)
  list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

if(NOT DEFINED ITK_DIR)
  #message(STATUS "${__indent}Adding project ${proj}")
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${git_protocol}://${Slicer_ITK_GIT_REPOSITORY}"
    GIT_TAG ${Slicer_ITK_GIT_TAG}
    SOURCE_DIR ${proj}
    BINARY_DIR ${proj}-build
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
      ${ep_common_flags}
      ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_TESTING:BOOL=OFF
      -DITK_INSTALL_LIB_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      -DITK_USE_REVIEW:BOOL=ON
      -DITK_USE_REVIEW_STATISTICS:BOOL=ON
      -DITK_USE_OPTIMIZED_REGISTRATION_METHODS:BOOL=ON
      #-DITK_USE_PORTABLE_ROUND:BOOL=ON # Unused
      -DITK_USE_CENTERED_PIXEL_COORDINATES_CONSISTENTLY:BOOL=ON
      -DITK_USE_TRANSFORM_IO_FACTORIES:BOOL=ON
      -DITK_LEGACY_REMOVE:BOOL=ON
      -DKWSYS_USE_MD5:BOOL=ON # Required by SlicerExecutionModel
    INSTALL_COMMAND ""
    DEPENDS
      ${ITKv3_DEPENDENCIES}
    )
  set(ITK_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

else()
  # The project is provided using ITK_DIR, nevertheless since other project may depend on ITK,
  # let's add an 'empty' one
  SlicerMacroEmptyExternalProject(${proj} "${ITKv3_DEPENDENCIES}")
endif()

