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


#-----------------------------------------------------------------------------
# Slicer version number.
#-----------------------------------------------------------------------------
# Releases define a tweak level
#set(Slicer_VERSION_TWEAK 1)
#set(Slicer_VERSION_RC 0)

set(CMAKE_MODULE_PATH
  ${Slicer_MODULE_PATH}
  ${CMAKE_MODULE_PATH})
include(${SlicerMacroExtractRepositoryInfo_PATH})
SlicerMacroExtractRepositoryInfo(VAR_PREFIX Slicer SOURCE_DIR ${Slicer_SOURCE_DIR}) # Used to configure vtkSlicerVersionConfigure.h.in
string(REGEX REPLACE ".*([0-9][0-9][0-9][0-9]\\-[0-9][0-9]\\-[0-9][0-9]).*" "\\1"
  Slicer_BUILDDATE "${Slicer_WC_LAST_CHANGED_DATE}")

if(NOT Slicer_FORCED_WC_REVISION STREQUAL "")
  set(Slicer_WC_REVISION "${Slicer_FORCED_WC_REVISION}")
endif()
if("${Slicer_VERSION_TWEAK}" STREQUAL "")
  set(_version_qualifier "-${Slicer_BUILDDATE}")
elseif("${Slicer_VERSION_TWEAK}" GREATER 0)
  set(_version_qualifier "-${Slicer_VERSION_TWEAK}")
endif()

# XXX This variable should not be set explicitly
set(Slicer_VERSION      "${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}")
set(Slicer_VERSION_FULL "${Slicer_VERSION}.${Slicer_VERSION_PATCH}")
if(Slicer_VERSION_RC)
  set(Slicer_VERSION_FULL "${Slicer_VERSION_FULL}-rc${Slicer_VERSION_RC}")
endif()
set(Slicer_VERSION_FULL "${Slicer_VERSION_FULL}${_version_qualifier}")

message(STATUS "Configuring ${Slicer_MAIN_PROJECT_APPLICATION_NAME} version [${Slicer_VERSION_FULL}]")
message(STATUS "Configuring ${Slicer_MAIN_PROJECT_APPLICATION_NAME} revision [${Slicer_WC_REVISION}]")

  configure_file(
    ${Slicer_SOURCE_DIR}/CMake/vtkSlicerVersionConfigure.h.in
    ${Slicer_BINARY_DIR}/vtkSlicerVersionConfigure.h
    )