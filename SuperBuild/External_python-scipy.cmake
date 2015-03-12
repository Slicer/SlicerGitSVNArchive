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

set(proj python-scipy)

#------------------------------------------------------------------------------
# Set dependency list
set(${proj}_DEPENDENCIES python python-setuptools python-numpy OpenBLAS)
if(WIN32)
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

  # XXX These variables should be set in External_python.cmake
  set(_pythonhome ${CMAKE_BINARY_DIR}/python-install)
  set(_pythonpath_subdir lib/python2.7)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_pythonpath_subdir Lib)
  endif()

  set(_np_core_path ${_pythonhome}/${_pythonpath_subdir}/site-packages/numpy/core)
  set(_np_core_lib_path ${_pythonhome}/${_pythonpath_subdir}/site-packages/numpy/lib)

  # environment
  set(_env_script ${CMAKE_BINARY_DIR}/${proj}_Env.cmake)
  ExternalProject_Write_SetBuildEnv_Commands(${_env_script})
  ExternalProject_Write_SetPythonSetupEnv_Commands(${_env_script} APPEND)
  if(MSVC)
    file(APPEND ${_env_script}
"#------------------------------------------------------------------------------
# Added by '${CMAKE_CURRENT_LIST_FILE}'

set(ENV{MSYSTEM} \"MINGW32\") # See numpy/distutils/misc_util.py
set(ENV{PATH} \"${MinGW_DIR}/bin;${_np_core_path};${_np_core_lib_path};\$ENV{PATH}\")
")
  elseif(UNIX)
    file(APPEND ${_env_script}
"#------------------------------------------------------------------------------
# Added by '${CMAKE_CURRENT_LIST_FILE}'

set(ENV{LD_LIBRARY_PATH} \"${_np_core_path}:${_np_core_lib_path}:\$ENV{LD_LIBRARY_PATH}\")
")
  endif()

  # configure step
  set(_configure_script ${CMAKE_BINARY_DIR}/${proj}_configure_step.cmake)
  file(WRITE ${_configure_script}
"include(\"${_env_script}\")
set(${proj}_WORKING_DIR \"${CMAKE_BINARY_DIR}/${proj}\")
#file(WRITE \"${CMAKE_BINARY_DIR}/${proj}/site.cfg\" \"\")
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

  set(_patch_step PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_CURRENT_LIST_DIR}/python-scipy-0.14.0-OpenBLAS.patch)
  if(MSVC)
    list(APPEND _patch_step COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_CURRENT_LIST_DIR}/python-scipy-0.14.0-OpenBLAS-MinGW.patch)
  endif()

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    URL "http://slicer.kitware.com/midas3/download/item/160337/scipy-0.14.0.tar.gz"
    URL_MD5 "d7c7f4ccf8b07b08d6fe49d5cd51f85d"
    SOURCE_DIR ${proj}
    BUILD_IN_SOURCE 1
    ${_patch_step}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${_configure_script}
    BUILD_COMMAND ${CMAKE_COMMAND} -P ${_build_script}
    INSTALL_COMMAND ${CMAKE_COMMAND} -P ${_install_script}
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()
