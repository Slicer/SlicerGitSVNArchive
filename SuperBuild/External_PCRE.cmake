
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
set(extProjName PCRE) #The find_package known name
set(proj        PCRE) #This local name

#if(${USE_SYSTEM_${extProjName}})
#  unset(${extProjName}_DIR CACHE)
#endif()

# Sanity checks
if(DEFINED ${extProjName}_DIR AND NOT EXISTS ${${extProjName}_DIR})
  message(FATAL_ERROR "${extProjName}_DIR variable is defined but corresponds to non-existing directory (${${extProjName}_DIR})")
endif()

# Set dependency list
set(${proj}_DEPENDENCIES "")

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(${proj})

#
#  PCRE (Perl Compatible Regular Expressions)
#

# follow the standard EP_PREFIX locations
set(pcre_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/PCRE-prefix/src/PCRE-build)
set(pcre_source_dir ${CMAKE_CURRENT_BINARY_DIR}/PCRE-prefix/src/PCRE)
set(pcre_install_dir ${CMAKE_CURRENT_BINARY_DIR}/PCRE)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild/pcre_configure_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/pcre_configure_step.cmake
  @ONLY)
set(pcre_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/pcre_configure_step.cmake)

ExternalProject_Add(${proj}
  URL http://downloads.sourceforge.net/project/pcre/pcre/8.12/pcre-8.12.tar.gz
  URL_MD5 fa69e4c5d8971544acd71d1f10d59193
  "${slicer_external_disable_update}"
  CONFIGURE_COMMAND ${pcre_CONFIGURE_COMMAND}
  )

ProjectDependancyPop(CACHED_extProjName extProjName)
ProjectDependancyPop(CACHED_proj proj)
