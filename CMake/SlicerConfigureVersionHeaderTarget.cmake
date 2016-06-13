################################################################################
#
#  Program: 3D Slicer
#
#  Copyright (c) Kitware Inc.
#
#  See COPYRIGHT.txt
#  or http://www.slicer.org/copyright/copyright.txt for details.
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
#  and was partially funded by NIH grant 3P41RR013218-12S1
#
################################################################################

#
# This module will add a target named 'SlicerConfigureVersionHeader'.
#
# It has been designed to be included in the build system of Slicer.
#
# The following variables are expected to be defined in the including scope:
#  GIT_EXECUTABLE
#  Slicer_BINARY_DIR
#  Slicer_CMAKE_DIR
#  Slicer_MAIN_PROJECT_APPLICATION_NAME
#  Slicer_SOURCE_DIR
#  Slicer_VERSION_MAJOR
#  Slicer_VERSION_MINOR
#  Slicer_VERSION_PATCH
#  Subversion_SVN_EXECUTABLE
#
# Optionally, these variable can also be set:
#  Slicer_FORCED_WC_REVISION (default "")
#  Slicer_VERSION_TWEAK
#  Slicer_VERSION_RC
#

# --------------------------------------------------------------------------
# Sanity checks
# --------------------------------------------------------------------------
set(expected_defined_vars
  GIT_EXECUTABLE
  Slicer_BINARY_DIR
  Slicer_CMAKE_DIR
  Slicer_MAIN_PROJECT_APPLICATION_NAME # Used by SlicerVersion.cmake
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

if(NOT DEFINED SLICER_CONFIGURE_VERSION_HEADER)
  set(SLICER_CONFIGURE_VERSION_HEADER 0)
endif()

# --------------------------------------------------------------------------
# Add SlicerConfigureVersionHeader target
# --------------------------------------------------------------------------
if(NOT SLICER_CONFIGURE_VERSION_HEADER)
  set(script_args)
  foreach(var IN LISTS expected_defined_vars)
    list(APPEND script_args "-D${var}:STRING=${${var}}")
  endforeach()
  if(NOT DEFINED Slicer_FORCED_WC_REVISION)
    set(Slicer_FORCED_WC_REVISION "")
  endif()
  add_custom_target(SlicerConfigureVersionHeader ALL
    COMMAND ${CMAKE_COMMAND}
      ${script_args}
      -DSlicer_FORCED_WC_REVISION:STRING=${Slicer_FORCED_WC_REVISION}
      -DSlicer_VERSION_TWEAK:STRING=${Slicer_VERSION_TWEAK}
      -DSlicer_VERSION_RC:STRING=${Slicer_VERSION_RC}
      -DSLICER_CONFIGURE_VERSION_HEADER=1
      -P ${CMAKE_CURRENT_LIST_FILE}
    COMMENT "Configuring vtkSlicerVersionConfigure.h"
    )
  return()
endif()

# --------------------------------------------------------------------------
# Configure header
# --------------------------------------------------------------------------

include(${Slicer_CMAKE_DIR}/SlicerVersion.cmake)

# Variables expected to be set by 'SlicerVersion' module.
set(expected_defined_vars
  Slicer_BUILDDATE
  Slicer_VERSION
  Slicer_VERSION_FULL
  Slicer_WC_REVISION
  Slicer_WC_URL
  )
foreach(var ${expected_defined_vars})
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is mandatory")
  endif()
endforeach()

configure_file(
  ${Slicer_SOURCE_DIR}/CMake/vtkSlicerVersionConfigure.h.in
  ${Slicer_BINARY_DIR}/vtkSlicerVersionConfigure.h
  )
