
#
# This module will set the variables Slicer_VERSION and Slicer_VERSION_FULL.
#
# It will also set all variables describing the SCM associated
# with Slicer_SOURCE_DIR.
#
# It has been designed to be included in the build system of Slicer.
#
# The following variables are expected to be defined in the including scope:
#  GIT_EXECUTABLE
#  Slicer_CMAKE_DIR
#  Slicer_MAIN_PROJECT_APPLICATION_NAME
#  Slicer_RELEASE_TYPE
#  Slicer_SOURCE_DIR
#  Slicer_VERSION_MAJOR
#  Slicer_VERSION_MINOR
#  Slicer_VERSION_PATCH
#  Subversion_SVN_EXECUTABLE
#

# --------------------------------------------------------------------------
# Sanity checks
# --------------------------------------------------------------------------
set(expected_defined_vars
  GIT_EXECUTABLE
  Slicer_CMAKE_DIR
  Slicer_MAIN_PROJECT_APPLICATION_NAME
  Slicer_RELEASE_TYPE
  Slicer_SOURCE_DIR
  Slicer_VERSION_MAJOR
  Slicer_VERSION_MINOR
  Slicer_VERSION_PATCH
  Subversion_SVN_EXECUTABLE
  )
foreach(var ${expected_defined_vars})
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is mandatory")
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Update CMake module path
#-----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH
  ${Slicer_CMAKE_DIR}
  ${CMAKE_MODULE_PATH}
  )

include(SlicerMacroExtractRepositoryInfo)

#-----------------------------------------------------------------------------
# Slicer version number
#-----------------------------------------------------------------------------

SlicerMacroExtractRepositoryInfo(
  VAR_PREFIX Slicer
  SOURCE_DIR ${Slicer_SOURCE_DIR}
  )
string(REGEX REPLACE ".*([0-9][0-9][0-9][0-9]\\-[0-9][0-9]\\-[0-9][0-9]).*" "\\1"
  Slicer_BUILDDATE "${Slicer_WC_LAST_CHANGED_DATE}")

if(NOT Slicer_FORCED_WC_REVISION STREQUAL "")
  set(Slicer_WC_REVISION "${Slicer_FORCED_WC_REVISION}")
endif()

set(Slicer_VERSION      "${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}")
set(Slicer_VERSION_FULL "${Slicer_VERSION}.${Slicer_VERSION_PATCH}")

if(NOT "${Slicer_RELEASE_TYPE}" STREQUAL "Stable")
  set(Slicer_VERSION_FULL "${Slicer_VERSION_FULL}-${Slicer_BUILDDATE}")
endif()

message(STATUS "Configuring ${Slicer_MAIN_PROJECT_APPLICATION_NAME} version [${Slicer_VERSION_FULL}]")
message(STATUS "Configuring ${Slicer_MAIN_PROJECT_APPLICATION_NAME} revision [${Slicer_WC_REVISION}]")
message(STATUS "Configuring ${Slicer_MAIN_PROJECT_APPLICATION_NAME} release type [${Slicer_RELEASE_TYPE}]")
