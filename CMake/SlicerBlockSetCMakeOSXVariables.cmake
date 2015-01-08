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
# SlicerBlockSetCMakeOSXVariables
#

#
# Adapted from Paraview/Superbuild/CMakeLists.txt
#

# Note: Change architecture *before* any enable_language() or project()
#       calls so that it's set properly to detect 64-bit-ness...
#
if(APPLE)

  # Waiting universal binaries are supported and tested, complain if
  # multiple architectures are specified.
  if(NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
    list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)
    if(arch_count GREATER 1)
      message(FATAL_ERROR "error: Only one value (i386 or x86_64) should be associated with CMAKE_OSX_ARCHITECTURES.")
    endif()
  endif()

  # See CMake/Modules/Platform/Darwin.cmake)
  #   8.x == Mac OSX 10.4 (Tiger)
  #   9.x == Mac OSX 10.5 (Leopard)
  #  10.x == Mac OSX 10.6 (Snow Leopard)
  #  11.x == Mac OSX 10.7 (Lion)
  #  12.x == Mac OSX 10.8 (Mountain Lion)
  #  13.x == Mac OSX 10.9 (Mavericks)
  #  14.x == Mac OSX 10.10 (Yosemite)
  set(OSX_SDK_104_NAME "Tiger")
  set(OSX_SDK_105_NAME "Leopard")
  set(OSX_SDK_106_NAME "Snow Leopard")
  set(OSX_SDK_107_NAME "Lion")
  set(OSX_SDK_108_NAME "Mountain Lion")
  set(OSX_SDK_109_NAME "Mavericks")
  set(OSX_SDK_1010_NAME "Yosemite")

  set(OSX_SDK_ROOTS
    /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
    /Developer/SDKs
    )
  # XXX Since
  #         (1) the default runtime associated with 10.9 is libc++ [1]
  #     and (2) qt support for 'macx-clang-libc++' is listed as 'unsupported' mkspecs (pre 2013-12-05 comment)
  #     and (3) Qt binaries are (as expected) build against 'libstdc++', we
  #     are removing 10.9 from the list of version to check.
  #     [1] http://stackoverflow.com/questions/19637164/c-linking-error-after-upgrading-to-mac-os-x-10-9-xcode-5-0-1/19637199#19637199

  ## NOTE: https://codereview.qt-project.org/#/c/70930/ QT 4.8.6 no longer has the restrictions
  ##       Binaries of QT 4.8.6 from homebrew are built with libc++
  ##       QT 4.8.6 can be built with libc++, and the stackoverflow comment is not valid

  set(OSX_SYSROOT_SEARCHED "")
  set(SDK_MAJOR_VERSIONS_TO_CHECK 10)
  set(SDK_MINOR_VERSIONS_TO_CHECK 10 9 8 7 6 5)
  foreach(SDK_MINOR_VERSION ${SDK_MINOR_VERSIONS_TO_CHECK})
    set(SDK_VERSION ${SDK_MAJOR_VERSIONS_TO_CHECK}.${SDK_MINOR_VERSION})
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET OR "${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "")
      foreach(SDK_ROOT ${OSX_SDK_ROOTS})
        set(TEST_OSX_SYSROOT "${SDK_ROOT}/MacOSX${SDK_VERSION}.sdk")
        list(APPEND OSX_SYSROOT_SEARCHED ${TEST_OSX_SYSROOT})
        if(EXISTS "${TEST_OSX_SYSROOT}")
          # Retrieve OSX target name
          string(REPLACE "." "" sdk_version_no_dot ${SDK_VERSION})
          set(OSX_NAME ${OSX_SDK_${sdk_version_no_dot}_NAME})
          set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Force build for 64-bit ${OSX_NAME}." FORCE)
          set(CMAKE_OSX_DEPLOYMENT_TARGET "${SDK_VERSION}" CACHE STRING "Force build for 64-bit ${OSX_NAME}." FORCE)
          set(CMAKE_OSX_SDK_MINOR_VERSION "${SDK_MINOR_VERSION}" CACHE STRING "Force build for 64-bit ${OSX_NAME}." FORCE)
          set(CMAKE_OSX_SYSROOT "${TEST_OSX_SYSROOT}" CACHE PATH "Force build for 64-bit ${OSX_NAME}." FORCE)
          message(STATUS "Setting OSX_ARCHITECTURES to '${CMAKE_OSX_ARCHITECTURES}' as none was specified.")
          message(STATUS "Setting OSX_DEPLOYMENT_TARGET to '${SDK_VERSION}' as none was specified.")
          message(STATUS "Setting OSX_SYSROOT to '${TEST_OSX_SYSROOT}' as none was specified.")
        endif()
      endforeach()
    endif()
  endforeach()

  if("${CMAKE_OSX_SYSROOT}" STREQUAL "")
    message(FATAL_ERROR "error: Required CMAKE_OSX_SYSROOT not found.  Searched in:\n${OSX_SYSROOT_SEARCHED}")
  else()
    if(NOT EXISTS "${CMAKE_OSX_SYSROOT}")
      message(FATAL_ERROR "error: CMAKE_OSX_SYSROOT='${CMAKE_OSX_SYSROOT}' does not exist")
    endif()
  endif()

  if(${CMAKE_OSX_SDK_MINOR_VERSION} GREATER 8 )
    message(WARNING "OSX Minor version is greater than 8, and it assumes
    default linkage against libc++ instead of libstd++.  It is the developers
    responsibility to ensure that all system tools (i.e. qt) are also built
    against libc++.  \"otool -L /usr/local/bin/qmake |fgrep libc++\"")
  endif()
endif()
