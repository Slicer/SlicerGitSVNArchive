
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED ChangeTrackerPy_SOURCE_DIR AND NOT EXISTS ${ChangeTrackerPy_SOURCE_DIR})
  message(FATAL_ERROR "ChangeTrackerPy_SOURCE_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set dependency list
set(ChangeTrackerPy_DEPENDENCIES "")

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(ChangeTrackerPy)
set(proj ChangeTrackerPy)

if(NOT DEFINED ChangeTrackerPy_SOURCE_DIR)
  #message(STATUS "${__indent}Adding project ${proj}")

  if(NOT DEFINED git_protocol)
    set(git_protocol "git")
  endif()

  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${git_protocol}://github.com/fedorov/ChangeTrackerPy.git"
    GIT_TAG "6459708494ebdf7ae8c25cf771397a40ba63c248"
    "${slicer_external_update}"
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${proj}-build
    CMAKE_GENERATOR ${gen}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS
      ${ChangeTrackerPy_DEPENDENCIES}
    )
  set(ChangeTrackerPy_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
else()
  # The project is provided using ChangeTrackerPy_DIR, nevertheless since other project may depend on EMSegment,
  # let's add an 'empty' one
  SlicerMacroEmptyExternalProject(${proj} "${ChangeTrackerPy_DEPENDENCIES}")
endif()
