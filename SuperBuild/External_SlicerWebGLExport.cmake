
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED SlicerWebGLExport_SOURCE_DIR AND NOT EXISTS ${SlicerWebGLExport_SOURCE_DIR})
  message(FATAL_ERROR "SlicerWebGLExport_SOURCE_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set dependency list
set(SlicerWebGLExport_DEPENDENCIES "")

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(SlicerWebGLExport)
set(proj SlicerWebGLExport)

if(NOT DEFINED SlicerWebGLExport_SOURCE_DIR)
  #message(STATUS "${__indent}Adding project ${proj}")

  if(NOT DEFINED git_protocol)
    set(git_protocol "git")
  endif()

  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${git_protocol}://github.com/xtk/SlicerWebGLExport.git"
    GIT_TAG "be54b2a30017971f938a9c2878db948fb3608cd9"
    ${slicer_external_update}
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${proj}-build
    CMAKE_GENERATOR ${gen}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS
      ${SlicerWebGLExport_DEPENDENCIES}
    )
  set(SlicerWebGLExport_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}/WebGLExport)
else()
  # The project is provided using SlicerWebGLExport_SOURCE_DIR, nevertheless since other project may depend on SlicerWebGLExport,
  # let's add an 'empty' one
  SlicerMacroEmptyExternalProject(${proj} "${SlicerWebGLExport_DEPENDENCIES}")
endif()
