
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED SlicerBRAINSTools_DIR AND NOT EXISTS ${SlicerBRAINSTools_DIR})
  message(FATAL_ERROR "SlicerBRAINSTools_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set dependency list
set(SlicerBRAINSTools_DEPENDENCIES ${ITK_EXTERNAL_NAME} SlicerExecutionModel VTK )

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(SlicerBRAINSTools)
set(proj SlicerBRAINSTools)

# Set CMake OSX variable to pass down the external project
set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
if(APPLE)
  list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

if(NOT DEFINED SlicerBRAINSTools_DIR)
  #message(STATUS "${__indent}Adding project ${proj}")
  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${git_protocol}://github.com/BRAINSia/BRAINSStandAlone.git"
    GIT_TAG "master"
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${proj}-build
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
      ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags} # Unused
      -DBUILD_TESTING:BOOL=ON
    # ITK
    -DUSE_SYSTEM_ITK:BOOL=ON
    -DITK_DIR:PATH=${ITK_DIR}
    -DUSE_SYSTEM_VTK:BOOL=ON
    -DVTK_DIR:PATH=${VTK_DIR}
    # SlicerExecutionModel
    -DUSE_SYSTEM_SEM:BOOL=ON
    -DSlicerExecutionModel_DIR:PATH=${SlicerExecutionModel_DIR}
    ## -- This could be some other variable to indicate a slicer build
      -DINTEGRATE_WITH_SLICER:BOOL=ON
      -DSlicer_SOURCE_DIR:PATH=${Slicer_SOURCE_DIR}
      -DSlicerBRAINSTools_LIBRARY_PROPERTIES:STRING=${Slicer_LIBRARY_PROPERTIES}
      -DSlicerBRAINSTools_INSTALL_BIN_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      -DSlicerBRAINSTools_INSTALL_LIB_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      #-DSlicerBRAINSTools_INSTALL_SHARE_DIR:PATH=${Slicer_INSTALL_ROOT}share/${SlicerBRAINSTools}
      -DSlicerBRAINSTools_INSTALL_NO_DEVELOPMENT:BOOL=${Slicer_INSTALL_NO_DEVELOPMENT}
      ## Which SlicerBRAINSTools packages to use
      -DUSE_BRAINSFit:BOOL=ON
      -DUSE_BRAINSABC:BOOL=ON
    INSTALL_COMMAND ""
    DEPENDS
      ${SlicerBRAINSTools_DEPENDENCIES}
    )
  set(SlicerBRAINSTools_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
else()
  # The project is provided using SlicerBRAINSTools_DIR, nevertheless since other project may depend on SlicerBRAINSTools,
  # let's add an 'empty' one
  SlicerMacroEmptyExternalProject(${proj} "${SlicerBRAINSTools_DEPENDENCIES}")
endif()

