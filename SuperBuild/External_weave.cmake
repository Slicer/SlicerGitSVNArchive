
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
set(extProjName weave) #The find_package known name
set(proj        weave) #This local name

#if(${USE_SYSTEM_${extProjName}})
#  unset(${extProjName}_DIR CACHE)
#endif()

# Sanity checks
if(DEFINED ${extProjName}_DIR AND NOT EXISTS ${${extProjName}_DIR})
  message(FATAL_ERROR "${extProjName}_DIR variable is defined but corresponds to non-existing directory (${${extProjName}_DIR})")
endif()

# Set dependency list
set(${proj}_DEPENDENCIES python NUMPY)

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(${proj})

#message(STATUS "${__indent}Adding project ${proj}")

set(weave_binary "${CMAKE_CURRENT_BINARY_DIR}/weave/")

# to configure weave we run a cmake -P script
# the script will create a site.cfg file
# then run python setup.py config to verify setup
# configure_file(
#   SuperBuild/weave_configure_step.cmake.in
#   ${CMAKE_CURRENT_BINARY_DIR}/weave_configure_step.cmake @ONLY)
# to build weave we also run a cmake -P script.
# the script will set LD_LIBRARY_PATH so that
# python can run after it is built on linux
configure_file(
  SuperBuild/weave_make_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/weave_make_step.cmake @ONLY)

# create an external project to download weave,
# and configure and build it
ExternalProject_Add(weave
  # URL ${${PROJECT_NAME}_SOURCE_DIR}/Modules/Python/FilteredTractography/weave
  SVN_REPOSITORY http://svn.slicer.org/Slicer3-lib-mirrors/trunk/weave
  SVN_REVISION -r "154"
  "${cmakeversion_external_disable_update}"
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/weave
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/weave
  BUILD_COMMAND ${CMAKE_COMMAND}
    -P ${CMAKE_CURRENT_BINARY_DIR}/weave_make_step.cmake
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS
    ${${proj}_DEPENDENCIES}
  )

ProjectDependancyPop(CACHED_extProjName extProjName)
ProjectDependancyPop(CACHED_proj proj)
