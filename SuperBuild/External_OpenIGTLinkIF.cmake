
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED OpenIGTLinkIF_SOURCE_DIR AND NOT EXISTS ${OpenIGTLinkIF_SOURCE_DIR})
  message(FATAL_ERROR "OpenIGTLinkIF_SOURCE_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set dependency list
set(OpenIGTLinkIF_DEPENDENCIES OpenIGTLink)

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(OpenIGTLinkIF)
set(proj OpenIGTLinkIF)

if(NOT DEFINED OpenIGTLinkIF_SOURCE_DIR)
  #message(STATUS "${__indent}Adding project ${proj}")
  ExternalProject_Add(${proj}
    SVN_REPOSITORY "http://svn.na-mic.org/NAMICSandBox/trunk/IGTLoadableModules/QtModules/OpenIGTLinkIF/"
    SVN_REVISION -r "7909"
    ${slicer_external_update}
    #GIT_REPOSITORY "${git_protocol}://github.com/Slicer/OpenIGTLinkIF.git"
    #GIT_TAG "8330b769cc8c607067134296d577b64ae7c92b87"
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${proj}-build
    CMAKE_GENERATOR ${gen}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS
      ${OpenIGTLinkIF_DEPENDENCIES}
    )
  set(OpenIGTLinkIF_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
else()
  # The project is provided using OpenIGTLinkIF_SOURCE_DIR, nevertheless since other project may depend on OpenIGTLinkIF,
  # let's add an 'empty' one
  SlicerMacroEmptyExternalProject(${proj} "${OpenIGTLinkIF_DEPENDENCIES}")
endif()
