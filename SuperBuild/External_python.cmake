
# Make sure this file is included only once by creating globally unique varibles
# based on the name of this included file.
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

## External_${extProjName}.cmake files can be recurisvely included,
## and cmake variables are global, so when including sub projects it
## is important make the extProjName and proj variables
## appear to stay constant in one of these files.
## Store global variables before overwriting (then restore at end of this file.)
ProjectDependancyPush(CACHED_extProjName ${extProjName})
ProjectDependancyPush(CACHED_proj ${proj})

# Make sure that the ExtProjName/IntProjName variables are unique globally
# even if other External_${ExtProjName}.cmake files are sourced by
# SlicerMacroCheckExternalProjectDependency
set(extProjName python) #The find_package known name
set(proj        python) #This local name

#if(${USE_SYSTEM_${extProjName}})
#  unset(${extProjName}_DIR CACHE)
#endif()

# Sanity checks
if(DEFINED ${extProjName}_DIR AND NOT EXISTS ${${extProjName}_DIR})
  message(FATAL_ERROR "${extProjName}_DIR variable is defined but corresponds to non-existing directory (${${extProjName}_DIR})")
endif()

# Set dependency list
set(python_DEPENDENCIES CTKAPPLAUNCHER)
if(${PROJECT_NAME}_USE_PYTHONQT_WITH_TCL)
  if(WIN32)
    list(APPEND python_DEPENDENCIES tcl)
  else()
    list(APPEND python_DEPENDENCIES tcl tk)
  endif()
endif()

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(${proj})

if(NOT ( DEFINED "${extProjName}_DIR" OR ( DEFINED "${USE_SYSTEM_${extProjName}}" AND NOT "${USE_SYSTEM_${extProjName}}" ) ) )
#message(STATUS "${__indent}Adding project ${proj}")

# Set CMake OSX variable to pass down the external project
set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
if(APPLE)
  list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

### --- Project specific additions here

set(python_base ${CMAKE_CURRENT_BINARY_DIR}/${proj})
set(python_build ${CMAKE_CURRENT_BINARY_DIR}/${proj}-build)
#
# WARNING - If you consider updating the Python version, make sure the patch
#           step associated with both window and unix are still valid !
#
set(PYVER_SHORT 26)
set(python_URL http://svn.slicer.org/Slicer3-lib-mirrors/trunk/Python-2.6.6.tgz)
set(python_MD5 b2f209df270a33315e62c1ffac1937f0)
# Since the solution file provided within "Python-2.6.6.tgz" is specific to VS2008, let's use
#  "Python-2.6.6-vc2010" where the solution file is converted for VS2010.
# This is required because there is command-line tool to convert the solution automatically.
if(CMAKE_GENERATOR MATCHES "Visual*"  AND "${MSVC_VERSION}" VERSION_GREATER "1599")
  set(python_URL http://svn.slicer.org/Slicer3-lib-mirrors/trunk/Python-2.6.6-vc2010.tgz)
  set(python_MD5 120b65e3ab568d8861803be811707c79)
endif()

get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

if(WIN32)
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
else()
endif()

list(APPEND ${CMAKE_PROJECT_NAME}_SUPERBUILD_EP_VARS ${extProjName}_DIR:PATH)

ProjectDependancyPop(CACHED_extProjName extProjName)
ProjectDependancyPop(CACHED_proj proj)
