
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Set dependency list
set(python_DEPENDENCIES CTKAPPLAUNCHER)
if(Slicer_USE_PYTHONQT_WITH_TCL)
  if(WIN32)
    list(APPEND python_DEPENDENCIES tcl)
  else()
    list(APPEND python_DEPENDENCIES tcl tk)
  endif()
endif()

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(python)
set(proj python)

#message(STATUS "${__indent}Adding project ${proj}")

get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

set(python_base ${CMAKE_CURRENT_BINARY_DIR}/${proj})
set(python_build ${CMAKE_CURRENT_BINARY_DIR}/${proj}-build)
#
# WARNING - If you consider updating the Python version, make sure the patch
#           step associated with both window and unix are still valid !
#
set(slicer_PYTHON_VERSION_MAJOR 2)
set(slicer_PYTHON_VERSION_MINOR 7)
set(slicer_PYTHON_VERSION_PATCH 3)
set(PYVER_SHORT ${slicer_PYTHON_VERSION_MAJOR}${slicer_PYTHON_VERSION_MINOR})
set(python_URL http://www.python.org/ftp/python/2.7.3/Python-2.7.3.tgz)
set(python_MD5 2cf641732ac23b18d139be077bd906cd)

if(WIN32)
# Since the solution file provided within "Python-2.6.6.tgz" is specific to VS2008, let's use
#  "Python-2.6.6-vc2010" where the solution file is converted for VS2010.
# This is required because there is command-line tool to convert the solution automatically.
  if(CMAKE_GENERATOR MATCHES "Visual*"  AND "${MSVC_VERSION}" VERSION_GREATER "1599")
    set(slicer_PYTHON_VERSION_MAJOR 2)
    set(slicer_PYTHON_VERSION_MINOR 6)
    set(slicer_PYTHON_VERSION_PATCH 6)
    set(PYVER_SHORT ${slicer_PYTHON_VERSION_MAJOR}${slicer_PYTHON_VERSION_MINOR})
    set(python_URL http://svn.slicer.org/Slicer3-lib-mirrors/trunk/Python-2.6.6-vc2010.tgz)
    set(python_MD5 120b65e3ab568d8861803be811707c79)
  endif()

  set(External_python_win_PROJECT_INCLUDED 1)
  include(${CMAKE_CURRENT_LIST_DIR}/External_python_win.cmake)
  set(External_python_win_PROJECT_INCLUDED 0)
else()
  set(External_python_unix_PROJECT_INCLUDED 1)
  include(${CMAKE_CURRENT_LIST_DIR}/External_python_unix.cmake)
  set(External_python_unix_PROJECT_INCLUDED 0)
endif()

#message(STATUS "MSVC_VERSION:${MSVC_VERSION}")
#message(STATUS "python_URL:${python_URL}")
#message(STATUS "CMAKE_GENERATOR:${CMAKE_GENERATOR}")

#message(STATUS "slicer_PYTHON_INCLUDE:${slicer_PYTHON_INCLUDE}")
#message(STATUS "slicer_PYTHON_LIBRARY:${slicer_PYTHON_LIBRARY}")
#message(STATUS "slicer_PYTHON_EXECUTABLE:${slicer_PYTHON_EXECUTABLE}")

