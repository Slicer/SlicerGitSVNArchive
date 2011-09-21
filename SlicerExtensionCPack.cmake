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

# -------------------------------------------------------------------------
# Sanity checks
# -------------------------------------------------------------------------
set(expected_nonempty_vars EXTENSION_NAME EXTENSION_DESCRIPTION Slicer_WC_REVISION Slicer_PLATFORM Slicer_ARCHITECTURE)
foreach(var ${expected_nonempty_vars})
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "error: ${var} is either NOT defined or empty.")
  endif()
endforeach()

set(expected_existing_vars EXTENSION_README_FILE EXTENSION_LICENSE_FILE)
foreach(var ${expected_existing_vars})
  if(NOT EXISTS ${${var}})
    message(FATAL_ERROR "error: ${var} points to an inexistent file: ${${var}}")
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Get working copy information
#-----------------------------------------------------------------------------
SlicerMacroExtractRepositoryInfo(VAR_PREFIX ${EXTENSION_NAME})

#-----------------------------------------------------------------------------
# Get today's date
#-----------------------------------------------------------------------------
include(SlicerFunctionToday)
TODAY(${EXTENSION_NAME}_BUILDDATE)

# -------------------------------------------------------------------------
# Package properties
# -------------------------------------------------------------------------

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${EXTENSION_DESCRIPTION}")

set(CPACK_MONOLITHIC_INSTALL ON)

set(CMAKE_PROJECT_NAME ${EXTENSION_NAME})
set(CPACK_PACKAGE_VENDOR "NA-MIC")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${EXTENSION_README_FILE}")
set(CPACK_RESOURCE_FILE_LICENSE "${EXTENSION_LICENSE_FILE}")
set(CPACK_PACKAGE_FILE_NAME "${Slicer_WC_REVISION}-${Slicer_PLATFORM}-${Slicer_ARCHITECTURE}-${EXTENSION_NAME}-${${EXTENSION_NAME}_WC_TYPE}${${EXTENSION_NAME}_WC_REVISION}-${${EXTENSION_NAME}_BUILDDATE}")
#set(CPACK_PACKAGE_VERSION_MAJOR "${Slicer_MAJOR_VERSION}")
#set(CPACK_PACKAGE_VERSION_MINOR "${Slicer_MINOR_VERSION}")
#set(CPACK_PACKAGE_VERSION_PATCH "${Slicer_PATCH_VERSION}")

#if(APPLE)
#  set(CPACK_PACKAGE_ICON "${Slicer_SOURCE_DIR}/Applications/SlicerQT/Resources/Slicer.icns")
#endif()

# Slicer does *NOT* require setting the windows path
set(CPACK_NSIS_MODIFY_PATH OFF)

# -------------------------------------------------------------------------
# Prepare s4ext file
# -------------------------------------------------------------------------
configure_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${EXTENSION_NAME}.s4ext
  ${CMAKE_CURRENT_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.s4ext
  COPYONLY
  )

# -------------------------------------------------------------------------
# Simulate generation of package named: ${CPACK_PACKAGE_FILE_NAME}.s4ext
# -------------------------------------------------------------------------
install(CODE "message(\"CPack: - package: ${CMAKE_CURRENT_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.s4ext generated.\")")

# -------------------------------------------------------------------------
# Disable source generator enabled by default
# -------------------------------------------------------------------------
set(CPACK_SOURCE_TBZ2 OFF CACHE BOOL "Enable to build TBZ2 source packages")
set(CPACK_SOURCE_TZ   OFF CACHE BOOL "Enable to build TZ source packages")

# -------------------------------------------------------------------------
# Enable generator
# -------------------------------------------------------------------------
set(CPACK_GENERATOR "TGZ")

include(CPack)

