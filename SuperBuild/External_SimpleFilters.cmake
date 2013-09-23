
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED SimpleFilters_SOURCE_DIR AND NOT EXISTS ${SimpleFilters_SOURCE_DIR})
  message(FATAL_ERROR "SimpleFilters_SOURCE_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set compile time dependency list
set(SimpleFilters_DEPENDENCIES "")

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(SimpleFilters)
set(proj SimpleFilters)

if(NOT DEFINED SimpleFilters_SOURCE_DIR)

  if(NOT DEFINED git_protocol)
    set(git_protocol "git")
  endif()

  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${git_protocol}://github.com/SimpleITK/SlicerSimpleFilters.git"
    GIT_TAG "fc1f06ce52aec9272f928f3d1e0fa59b1a1c8bd6"
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS
      ${SimpleFilters_DEPENDENCIES}
    )
  set(SimpleFilters_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
else()
  SlicerMacroEmptyExternalProject(${proj} "${SimpleFilters_DEPENDENCIES}")
endif()
