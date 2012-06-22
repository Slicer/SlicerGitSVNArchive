
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED LibArchive_DIR AND NOT EXISTS ${LibArchive_DIR})
  message(FATAL_ERROR "LibArchive_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set dependency list
set(LibArchive_DEPENDENCIES "")
if(WIN32)
  list(APPEND LibArchive_DEPENDENCIES zlib)
endif()

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(LibArchive)
set(proj LibArchive)

if(NOT DEFINED LibArchive_DIR)
  #message(STATUS "${__indent}Adding project ${proj}")
  #
  # NOTE: - a stable, recent release (3.0.4) of LibArchive is now checked out from git 
  #         for all platforms.  For notes on cross-platform issues with earlier versions 
  #         of LibArchive, see the repository for earlier revisions of this file.
  #
  set(ADDITIONAL_CMAKE_ARGS)
  if(WIN32)
    # CMake arguments specific to LibArchive >= 2.8.4
    list(APPEND ADDITIONAL_CMAKE_ARGS
      -DBUILD_TESTING:BOOL=OFF
      -DENABLE_OPENSSL:BOOL=OFF
      -DZLIB_INCLUDE_DIR:PATH=${zlib_DIR}/include
      -DZLIB_LIBRARY:FILEPATH=${zlib_DIR}/lib/zlib.lib
      -DZLIB_ROOT:PATH=${zlib_DIR}
      )
  endif()

  # Set CMake OSX variable to pass down the external project
  set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
  if(APPLE)
    list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
      -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
      -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
  endif()

  if(NOT DEFINED git_protocol)
    set(git_protocol "git")
  endif()

  ExternalProject_Add(${proj}
    GIT_REPOSITORY "${git_protocol}://github.com/libarchive/libarchive.git"
    GIT_TAG "v3.0.4"
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${proj}-build
    UPDATE_COMMAND ""
    INSTALL_DIR LibArchive-install
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
    # Not used -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    # Not used -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DENABLE_ACL:BOOL=OFF
      -DENABLE_CPIO:BOOL=OFF
      -DENABLE_TAR:BOOL=OFF
      -DENABLE_TEST:BOOL=OFF
      -DENABLE_XATTR:BOOL=OFF
      ${ADDITIONAL_CMAKE_ARGS}
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    DEPENDS
      ${LibArchive_DEPENDENCIES}
    )
  set(LibArchive_DIR ${CMAKE_BINARY_DIR}/LibArchive-install)
else()
  # The project is provided using LibArchive_DIR, nevertheless since other project may depend on LibArchive,
  # let's add an 'empty' one
  SlicerMacroEmptyExternalProject(${proj} "${LibArchive_DEPENDENCIES}")
endif()

