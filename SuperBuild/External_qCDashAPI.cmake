
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED qCDashAPI_DIR AND NOT EXISTS ${qCDashAPI_DIR})
  message(FATAL_ERROR "qCDashAPI_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set dependency list
set(qCDashAPI_DEPENDENCIES "")

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(qCDashAPI)
set(proj qCDashAPI)

if(NOT DEFINED qCDashAPI_DIR)
  #message(STATUS "${__indent}Adding project ${proj}")

  # Set CMake OSX variable to pass down the external project
  set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
  if(APPLE)
    list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
      -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
      -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
  endif()

  if(NOT DEFINED git_protocol)
    set(git_protocol "git")
  endif()

  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${git_protocol}://github.com/jcfr/qCDashAPI.git"
    GIT_TAG "9cd19663c1884b28ba4ad4153b290bf9da5500ab"
    ${slicer_external_update}
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${proj}-build
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
      ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      #-DCMAKE_C_FLAGS:STRING=${ep_common_c_flags} # Unused
      -DBUILD_TESTING:BOOL=OFF
      -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
    INSTALL_COMMAND ""
    DEPENDS
      ${qCDashAPI_DEPENDENCIES}
    )
  set(qCDashAPI_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
else()
  # The project is provided using qCDashAPI_DIR, nevertheless since other project may depend on qCDashAPI,
  # let's add an 'empty' one
  SlicerMacroEmptyExternalProject(${proj} "${qCDashAPI_DEPENDENCIES}")
endif()

