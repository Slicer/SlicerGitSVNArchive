################################################################################
#
#  Program: 3D Slicer
#
#  Copyright (c) 2010 Kitware Inc.
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
#  This file was originally developed by
#   Dave Partyka and Jean-Christophe Fillion-Robin, Kitware Inc.
#  and was partially funded by NIH grant 3P41RR013218-12S1
#
################################################################################

#-----------------------------------------------------------------------------
# Git protocole option
#-----------------------------------------------------------------------------

option(Slicer_USE_GIT_PROTOCOL "If behind a firewall turn this off to use http instead." ON)

set(git_protocol "git")
if(NOT Slicer_USE_GIT_PROTOCOL)
  set(git_protocol "http")
endif()

#-----------------------------------------------------------------------------
# Qt - Let's check if a valid version of Qt is available
#-----------------------------------------------------------------------------
include(SlicerBlockFindQtAndCheckVersion)

#-----------------------------------------------------------------------------
# Enable and setup External project global properties
#-----------------------------------------------------------------------------

include(ExternalProject)
include(SlicerMacroCheckExternalProjectDependency)

set(ep_base        "${CMAKE_BINARY_DIR}")
#set(ep_install_dir "${ep_base}/Install")

set(ep_common_c_flags "${CMAKE_C_FLAGS_INIT} ${ADDITIONAL_C_FLAGS}")
set(ep_common_cxx_flags "${CMAKE_CXX_FLAGS_INIT} ${ADDITIONAL_CXX_FLAGS}")

set(ep_common_flags
  -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
  -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
  -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
  -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
  )

# Compute -G arg for configuring external projects with the same CMake generator:
if(CMAKE_EXTRA_GENERATOR)
  set(gen "${CMAKE_EXTRA_GENERATOR} - ${CMAKE_GENERATOR}")
else()
  set(gen "${CMAKE_GENERATOR}")
endif()

#------------------------------------------------------------------------------
# Slicer dependency list
#------------------------------------------------------------------------------
option(Slicer_BUILD_WITH_ITKv4 "Build Slicer against ITKv4" OFF)
mark_as_advanced(Slicer_BUILD_WITH_ITKv4)

if(Slicer_BUILD_WITH_ITKv4)
  set(ITK_EXTERNAL_NAME "ITKv4")
  set(ITK_LIBRARY_DIR_NAME "ITKv4")
else()
  set(ITK_EXTERNAL_NAME "Insight")
  set(ITK_LIBRARY_DIR_NAME "InsightToolkit")
endif()

set(Slicer_DEPENDENCIES LibArchive cmcurl OpenIGTLink teem VTK ${ITK_EXTERNAL_NAME} CTK qCDashAPI SlicerExecutionModel)
if(Slicer_USE_BatchMake)
  list(APPEND Slicer_DEPENDENCIES BatchMake)
endif()
if (Slicer_USE_CTKAPPLAUNCHER)
  list(APPEND Slicer_DEPENDENCIES CTKAPPLAUNCHER)
endif()
if(Slicer_USE_PYTHONQT)
  list(APPEND Slicer_DEPENDENCIES python)
  if(Slicer_USE_NUMPY)
    list(APPEND Slicer_DEPENDENCIES NUMPY)
    if(Slicer_USE_WEAVE)
      list(APPEND Slicer_DEPENDENCIES weave)
    endif()
    if(Slicer_USE_SCIPY)
      list(APPEND Slicer_DEPENDENCIES SciPy)
    endif()
  endif()
  if(Slicer_USE_PYTHONQT_WITH_TCL AND UNIX)
    list(APPEND Slicer_DEPENDENCIES incrTcl)
  endif()
endif()

SlicerMacroCheckExternalProjectDependency(Slicer)

#-----------------------------------------------------------------------------
# Dump Slicer external project dependencies
#------------------------------------------------------------------------------
#set(ep_dependency_graph "# External project dependencies")
#foreach(ep ${Slicer_DEPENDENCIES})
#  set(ep_dependency_graph "${ep_dependency_graph}\n${ep}:${${ep}_DEPENDENCIES}")
#endforeach()
#file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/SlicerDependencies.txt "${ep_dependency_graph}\n")
#message(STATUS "Generated ${CMAKE_CURRENT_BINARY_DIR}/SlicerDependencies.txt")

#-----------------------------------------------------------------------------
# Set superbuild boolean args
#

set(slicer_cmake_boolean_args
  DOCUMENTATION_TARGET_IN_ALL
  BUILD_TESTING
  BUILD_SHARED_LIBS
  WITH_COVERAGE
  WITH_MEMCHECK
  Slicer_BUILD_CLI
  Slicer_BUILD_QTLOADABLEMODULES
  Slicer_BUILD_QTSCRIPTEDMODULES
  Slicer_USE_PYTHONQT
  Slicer_USE_PYTHONQT_WITH_TCL
  Slicer_USE_CTKAPPLAUNCHER
  Slicer_USE_BatchMake
  Slicer_USE_MIDAS
  Slicer_USE_OpenIGTLink
  Slicer_WITH_LIBRARY_VERSION
  Slicer_USE_NUMPY
  #Slicer_USE_WEAVE
  )

set(slicer_superbuild_boolean_args)
foreach(slicer_cmake_arg ${slicer_cmake_boolean_args})
  list(APPEND slicer_superbuild_boolean_args -D${slicer_cmake_arg}:BOOL=${${slicer_cmake_arg}})
endforeach()

# message("CMake args:")
# foreach(arg ${slicer_superbuild_boolean_args})
#   message("  ${arg}")
# endforeach()

#-----------------------------------------------------------------------------
# Define list of additional options used to configure Slicer
#------------------------------------------------------------------------------
set(slicer_superbuild_extra_args)

if(DEFINED CTEST_CONFIGURATION_TYPE)
  list(APPEND slicer_superbuild_extra_args -DCTEST_CONFIGURATION_TYPE:STRING=${CTEST_CONFIGURATION_TYPE})
endif()

if(Slicer_BUILD_CLI)
  list(APPEND slicer_superbuild_extra_args -DBUILD_BRAINSTOOLS:BOOL=${Slicer_BUILD_BRAINSTOOLS})
endif()

if(Slicer_USE_PYTHONQT)
  list(APPEND slicer_superbuild_extra_args
    -DSlicer_USE_SYSTEM_PYTHON:BOOL=OFF
    -DPYTHON_EXECUTABLE:FILEPATH=${slicer_PYTHON_EXECUTABLE}
    -DPYTHON_INCLUDE_DIR:PATH=${slicer_PYTHON_INCLUDE}
    -DPYTHON_LIBRARY:FILEPATH=${slicer_PYTHON_LIBRARY}
    )
endif()

if(Slicer_USE_PYTHONQT_WITH_TCL)
  list(APPEND slicer_superbuild_extra_args
    -DSlicer_TCL_DIR:PATH=${tcl_build}
    -DTCL_TK_VERSION:STRING=${TCL_TK_VERSION}
    -DTCL_TK_VERSION_DOT:STRING=${TCL_TK_VERSION_DOT}
    )
  if(TARGET incrTcl)
    list(APPEND slicer_superbuild_extra_args
      -DINCR_TCL_VERSION:STRING=${INCR_TCL_VERSION}
      -DINCR_TCL_VERSION_DOT:STRING=${INCR_TCL_VERSION_DOT}
      )
  endif()
endif()

if(Slicer_USE_BatchMake)
  list(APPEND slicer_superbuild_extra_args -DBatchMake_DIR:PATH=${BatchMake_DIR})
endif()

if(Slicer_USE_OpenIGTLink)
  list(APPEND slicer_superbuild_extra_args -DOpenIGTLink_DIR:PATH=${OpenIGTLink_DIR})
endif()

if(Slicer_USE_CTKAPPLAUNCHER)
  list(APPEND slicer_superbuild_extra_args -DCTKAPPLAUNCHER_DIR:PATH=${CTKAPPLAUNCHER_DIR})
endif()

# Set CMake OSX variable to pass down the external project
set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
if(APPLE)
  list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

#------------------------------------------------------------------------------
# Configure and build Slicer
#------------------------------------------------------------------------------
set(proj Slicer)

ExternalProject_Add(${proj}
  DEPENDS ${Slicer_DEPENDENCIES}
  DOWNLOAD_COMMAND ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  BINARY_DIR Slicer-build
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    ${slicer_superbuild_boolean_args}
    ${slicer_superbuild_extra_args}
    ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
    -DADDITIONAL_C_FLAGS:STRING=${ADDITIONAL_C_FLAGS}
    -DADDITIONAL_CXX_FLAGS:STRING=${ADDITIONAL_CXX_FLAGS}
    -DSlicer_REQUIRED_C_FLAGS:STRING=${Slicer_REQUIRED_C_FLAGS}
    -DSlicer_REQUIRED_CXX_FLAGS:STRING=${Slicer_REQUIRED_CXX_FLAGS}
    -DWITH_COVERAGE:BOOL=${WITH_COVERAGE}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DSubversion_SVN_EXECUTABLE:FILEPATH=${Subversion_SVN_EXECUTABLE}
    -DGIT_EXECUTABLE:FILEPATH=${GIT_EXECUTABLE}
    -DSlicer_SUPERBUILD:BOOL=OFF
    -DSlicer_SUPERBUILD_DIR:PATH=${Slicer_BINARY_DIR}
    -DDOCUMENTATION_ARCHIVES_OUTPUT_DIRECTORY:PATH=${DOCUMENTATION_ARCHIVES_OUTPUT_DIRECTORY}
    -DDOXYGEN_EXECUTABLE:FILEPATH=${DOXYGEN_EXECUTABLE}
    # ITK
    -DITK_DIR:PATH=${ITK_DIR}
    # SlicerExecutionModel
    -DSlicerExecutionModel_DIR:PATH=${SlicerExecutionModel_DIR}
    # Teem
    -DTeem_DIR:PATH=${Teem_DIR}
    # VTK
    -DVTK_DIR:PATH=${VTK_DIR}
    -DVTK_DEBUG_LEAKS:BOOL=${Slicer_USE_VTK_DEBUG_LEAKS}
    # cmcurl
    -DSLICERLIBCURL_DIR:PATH=${SLICERLIBCURL_DIR}
    # LibArchive
    -DLibArchive_DIR:PATH=${LibArchive_DIR}
    # Qt
    -DQT_QMAKE_EXECUTABLE:PATH=${QT_QMAKE_EXECUTABLE}
    # CTK
    -DCTK_DIR:PATH=${CTK_DIR}
    # qCDashAPI
    -DqCDashAPI_DIR:PATH=${qCDashAPI_DIR}
  INSTALL_COMMAND ""
  )

#-----------------------------------------------------------------------------
# Slicer extensions
#-----------------------------------------------------------------------------
if(Slicer_BUILD_EXTENSIONS)
  set(Slicer_EXTENSION_DESCRIPTION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Extensions)
  set(Slicer_LOCAL_EXTENSIONS_DIR ${Slicer_EXTENSION_DESCRIPTION_DIR})
  set(Slicer_DIR ${CMAKE_CURRENT_BINARY_DIR}/Slicer-build)
  include(SlicerMacroExtractRepositoryInfo)
  SlicerMacroExtractRepositoryInfo(VAR_PREFIX Slicer)
  include(SlicerBlockBuildPackageAndUploadExtensions)
endif()
