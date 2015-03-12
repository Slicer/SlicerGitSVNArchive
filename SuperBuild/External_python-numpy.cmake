#============================================================================
#
#  Program: 3D Slicer
#
#  Copyright (c) Kitware, Inc.
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
#
#============================================================================

set(proj python-numpy)

#------------------------------------------------------------------------------
# Set dependency list
set(${proj}_DEPENDENCIES python python-setuptools OpenBLAS)
if(MSVC)
  list(APPEND ${proj}_DEPENDENCIES MinGW)
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  # XXX - Add a test checking if <proj> is available
endif()

if(NOT DEFINED ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  set(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} ${${CMAKE_PROJECT_NAME}_USE_SYSTEM_python})
endif()

if(NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  include(ExternalProjectForNonCMakeProject)

  # environment
  set(_env_script ${CMAKE_BINARY_DIR}/${proj}_Env.cmake)
  ExternalProject_Write_SetBuildEnv_Commands(${_env_script})
  ExternalProject_Write_SetPythonSetupEnv_Commands(${_env_script} APPEND)
  if(MSVC)
    file(APPEND ${_env_script}
"#------------------------------------------------------------------------------
# Added by '${CMAKE_CURRENT_LIST_FILE}'

set(ENV{MSYSTEM} \"MINGW32\") # See numpy/distutils/misc_util.py
set(ENV{PATH} \"${MinGW_DIR}/bin;\$ENV{PATH}\")
")
  endif()

  # configure step
  set(_configure_script ${CMAKE_BINARY_DIR}/${proj}_configure_step.cmake)
  file(WRITE ${_configure_script}
"include(\"${_env_script}\")
set(${proj}_WORKING_DIR \"${CMAKE_BINARY_DIR}/${proj}\")
file(WRITE \"${CMAKE_BINARY_DIR}/${proj}/site.cfg\" \"
[openblas]
libraries = openblas
library_dirs = ${OpenBLAS_DIR}/lib
include_dirs = ${OpenBLAS_DIR}/include
\")
ExternalProject_Execute(${proj} \"configure\" \"${PYTHON_EXECUTABLE}\" setup.py config)
")

  # build step
  set(_build_script ${CMAKE_BINARY_DIR}/${proj}_build_step.cmake)
  set(_build_options --fcompiler=gnu95)
  if(MSVC)
    list(APPEND _build_options --compiler=mingw32)
  endif()
  file(WRITE ${_build_script}
"include(\"${_env_script}\")
set(${proj}_WORKING_DIR \"${CMAKE_BINARY_DIR}/${proj}\")
ExternalProject_Execute(${proj} \"build\" \"${PYTHON_EXECUTABLE}\" setup.py build ${_build_options})
")

  # install step
  set(_install_script ${CMAKE_BINARY_DIR}/${proj}_install_step.cmake)
  file(WRITE ${_install_script}
"include(\"${_env_script}\")
set(${proj}_WORKING_DIR \"${CMAKE_BINARY_DIR}/${proj}\")
ExternalProject_Execute(${proj} \"install\" \"${PYTHON_EXECUTABLE}\" setup.py install)
")

  find_program(PATCH_EXECUTABLE
    NAMES patch
    PATH_SUFFIXES Git/bin
    DOC "git command line client"
    )
  mark_as_advanced(PATCH_EXECUTABLE)

  if(NOT PATCH_EXECUTABLE)
    message(FATAL_ERROR "PATCH_EXECUTABLE is required.")
  endif()

  set(_patch_step PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_CURRENT_LIST_DIR}/python-numpy-1.9.0-OpenBLAS.patch)
  if(MSVC)
    list(APPEND _patch_step
      COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_CURRENT_LIST_DIR}/python-numpy-1.9.0-OpenBLAS-MinGW.patch
      )
  elseif(UNIX)
    list(APPEND _patch_step
      COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_CURRENT_LIST_DIR}/python-numpy-1.9.0-OpenBLAS-Unix.patch
      )
  endif()

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    URL "http://slicer.kitware.com/midas3/download/item/160283/numpy-1.9.0.tar.gz"
    URL_MD5 "a93dfc447f3ef749b31447084839930b"
    SOURCE_DIR ${proj}
    BUILD_IN_SOURCE 1
    ${_patch_step}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${_configure_script}
    BUILD_COMMAND ${CMAKE_COMMAND} -P ${_build_script}
    INSTALL_COMMAND ${CMAKE_COMMAND} -P ${_install_script}
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  #-----------------------------------------------------------------------------
  if(MSVC)
    # XXX Moved this to External_python.cmake
    get_filename_component(PYTHON_DIR_PATH ${PYTHON_EXECUTABLE} PATH)
    set(PYTHON_LIBRARY_PATH ${PYTHON_DIR_PATH}/../lib)
    if(WIN32)
      set(PYTHON_LIBRARY_PATH ${PYTHON_DIR_PATH})
    endif()

    get_filename_component(PYTHON_LIB_BASE ${PYTHON_LIBRARY} NAME_WE)
    set(python_dll "${PYTHON_LIBRARY_PATH}/${PYTHON_LIB_BASE}.dll")
    set(python_def "${PYTHON_LIBRARY_PATH}/${PYTHON_LIB_BASE}.def")

    get_filename_component(PYTHON_IMPORT_LIBRARY_PATH ${PYTHON_LIBRARY} PATH)
    set(python_mingw_lib "${PYTHON_IMPORT_LIBRARY_PATH}/lib${PYTHON_LIB_BASE}.a")

    set(gendef "${MinGW_DIR}/bin/gendef.exe")
    set(dlltool "${MinGW_DIR}/bin/dlltool.exe")

    ExternalProject_Add_Step(${proj} python_create_static_lib
      # XXX - gendef output def file in the current directory
      COMMAND ${gendef} ${python_dll}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_LIB_BASE}.def ${python_def}
      COMMAND ${dlltool} -k --output-lib ${python_mingw_lib} --def ${python_def}
      DEPENDERS configure
      )
  endif()

  #-----------------------------------------------------------------------------
  # Since 'python-numpy-1.9.0-OpenBLAS-{MinGW,Unix}.patch' updates the numpy install rules
  # to copy 'libopenblas.{dll,so}' from "${OpenBLAS_DIR}/bin/" to "${CMAKE_BINARY_DIR}/${proj}/numpy/core/",
  # the following step copy the library where it is expected.
  set(_runtime_library_dir bin)
  set(_library_suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
  if(UNIX)
    set(_runtime_library_dir lib)
    if(NOT APPLE)
      set(_library_suffix ${_library_suffix}.0)
    endif()
  endif()
  ExternalProject_Add_Step(${proj} copy_libopenblas_to_numpy_core
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${OpenBLAS_DIR}/${_runtime_library_dir}/libopenblas${_library_suffix}
      ${CMAKE_BINARY_DIR}/${proj}/numpy/core/libopenblas${_library_suffix}
    DEPENDEES download
    DEPENDERS configure
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  set(_pythonhome ${CMAKE_BINARY_DIR}/python-install)
  set(pythonpath_subdir lib/python2.7)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(pythonpath_subdir Lib)
  endif()

  set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    ${_pythonhome}/${pythonpath_subdir}/site-packages/numpy/core
    ${_pythonhome}/${pythonpath_subdir}/site-packages/numpy/lib
    )
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    LABELS "LIBRARY_PATHS_LAUNCHER_BUILD"
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to install tree

  set(${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
    <APPLAUNCHER_DIR>/lib/Python/${pythonpath_subdir}/site-packages/numpy/core
    <APPLAUNCHER_DIR>/lib/Python/${pythonpath_subdir}/site-packages/numpy/lib
    )
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
    LABELS "LIBRARY_PATHS_LAUNCHER_INSTALLED"
    )

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()
