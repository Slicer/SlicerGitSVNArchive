
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
set(extProjName NUMPY) #The find_package known name
set(proj        NUMPY) #This local name

#if(${USE_SYSTEM_${extProjName}})
#  unset(${extProjName}_DIR CACHE)
#endif()

# Sanity checks
if(DEFINED ${extProjName}_DIR AND NOT EXISTS ${${extProjName}_DIR})
  message(FATAL_ERROR "${extProjName}_DIR variable is defined but corresponds to non-existing directory (${${extProjName}_DIR})")
endif()

# Set dependency list
set(${proj}_DEPENDENCIES python)

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(${proj})

message(STATUS "${__${proj}_superbuild_message} - Building without Fortran compiler - Non-optimized code will be built !")
#message(STATUS "${__indent}Adding project ${proj}")

set(numpy_URL http://svn.slicer.org/Slicer3-lib-mirrors/trunk/numpy-1.4.1.tar.gz)
set(numpy_MD5 5c7b5349dc3161763f7f366ceb96516b)

#------------------------------------------------------------------------------
set(${extProjName}_DIR "${CMAKE_BINARY_DIR}/${proj}")

configure_file(
  SuperBuild/${proj}_environment.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${proj}_environment.cmake @ONLY)

configure_file(
  SuperBuild/${proj}_configure_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${proj}_configure_step.cmake @ONLY)

configure_file(
  SuperBuild/NUMPY_make_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${proj}_make_step.cmake @ONLY)

configure_file(
  SuperBuild/${proj}_install_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${proj}_install_step.cmake @ONLY)

#------------------------------------------------------------------------------
ExternalProject_Add(${proj}
  URL ${numpy_URL}
  URL_MD5 ${numpy_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/NUMPY
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/NUMPY
  "${slicer_external_disable_update}"
  CONFIGURE_COMMAND ${CMAKE_COMMAND}
    -P ${CMAKE_CURRENT_BINARY_DIR}/${proj}_configure_step.cmake
  BUILD_COMMAND ${CMAKE_COMMAND}
    -P ${CMAKE_CURRENT_BINARY_DIR}/${proj}_make_step.cmake
  INSTALL_COMMAND ${CMAKE_COMMAND}
    -P ${CMAKE_CURRENT_BINARY_DIR}/${proj}_install_step.cmake
  PATCH_COMMAND ${CMAKE_COMMAND}
  -DNUMPY_SRC_DIR=${Slicer_BINARY_DIR}/NUMPY
    -P ${CMAKE_CURRENT_LIST_DIR}/${proj}_patch.cmake
  DEPENDS
    ${${proj}_DEPENDENCIES}
  )

ProjectDependancyPop(CACHED_extProjName extProjName)
ProjectDependancyPop(CACHED_proj proj)
