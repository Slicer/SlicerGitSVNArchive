
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
set(extProjName LibArchive) #The find_package known name
set(proj        LibArchive) #This local name

#if(${USE_SYSTEM_${extProjName}})
#  unset(${extProjName}_DIR CACHE)
#endif()

# Sanity checks
if(DEFINED ${extProjName}_DIR AND NOT EXISTS ${${extProjName}_DIR})
  message(FATAL_ERROR "${extProjName}_DIR variable is defined but corresponds to non-existing directory (${${extProjName}_DIR})")
endif()

# Set dependency list
set(${proj}_DEPENDENCIES "")
if(WIN32)
  list(APPEND ${proj}_DEPENDENCIES zlib)
endif()

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(${proj})

if(NOT DEFINED ${extProjName}_DIR)
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
  #
  # NOTE: - a stable, recent release (3.0.4) of LibArchive is now checked out from git
  #         for all platforms.  For notes on cross-platform issues with earlier versions
  #         of LibArchive, see the repository for earlier revisions of this file.
  #
  set(ADDITIONAL_CMAKE_ARGS)
  if(WIN32)
    # CMake arguments specific to LibArchive >= 2.8.4
    list(APPEND ADDITIONAL_CMAKE_ARGS
      -DZLIB_INCLUDE_DIR:PATH=${zlib_DIR}/include
      -DZLIB_LIBRARY:FILEPATH=${zlib_DIR}/lib/zlib.lib
      -DZLIB_ROOT:PATH=${zlib_DIR}
      )
  endif()
  # CMake arguments specific to LibArchive >= 2.8.4
  list(APPEND ADDITIONAL_CMAKE_ARGS
    -DBUILD_TESTING:BOOL=OFF
    -DENABLE_OPENSSL:BOOL=OFF
    )
  if(NOT DEFINED git_protocol)
      set(git_protocol "git")
  endif()

  set(${proj}_CMAKE_OPTIONS
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DENABLE_ACL:BOOL=OFF
      -DENABLE_CPIO:BOOL=OFF
      -DENABLE_TAR:BOOL=OFF
      -DENABLE_TEST:BOOL=OFF
      -DENABLE_XATTR:BOOL=OFF
      ${ADDITIONAL_CMAKE_ARGS}
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )
  ### --- End Project specific additions
  set(${proj}_REPOSITORY "${git_protocol}://github.com/libarchive/libarchive.git")
  set(${proj}_GIT_TAG "v3.0.4")
  ExternalProject_Add(${proj}
    GIT_REPOSITORY ${${proj}_REPOSITORY}
    GIT_TAG ${${proj}_GIT_TAG}
    SOURCE_DIR ${proj}
    BINARY_DIR ${proj}-build
    INSTALL_DIR LibArchive-install
    "${cmakeversion_external_update}"
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
    # Not used -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    # Not used -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
      ${${proj}_CMAKE_OPTIONS}
    DEPENDS
      ${${proj}_DEPENDENCIES}
  )
  set(${extProjName}_DIR ${CMAKE_BINARY_DIR}/${proj}-install)
else()
  if(${USE_SYSTEM_${extProjName}})
    find_package(${extProjName} ${ITK_VERSION_MAJOR} REQUIRED)
    if(NOT ${extProjName}_DIR)
      message(FATAL_ERROR "To use the system ${extProjName}, set ${extProjName}_DIR")
    endif()
    message("USING the system ${extProjName}, set ${extProjName}_DIR=${${extProjName}_DIR}")
  endif()
  # The project is provided using ${extProjName}_DIR, nevertheless since other
  # project may depend on ${extProjName}, let's add an 'empty' one
  SlicerMacroEmptyExternalProject(${proj} "${${proj}_DEPENDENCIES}")
endif()

list(APPEND ${CMAKE_PROJECT_NAME}_SUPERBUILD_EP_VARS ${extProjName}_DIR:PATH)

ProjectDependancyPop(CACHED_extProjName extProjName)
ProjectDependancyPop(CACHED_proj proj)
