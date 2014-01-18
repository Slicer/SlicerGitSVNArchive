
set(proj LibArchive)

# Set dependency list
set(${proj}_DEPENDENCIES "zlib")
if(WIN32)
  list(APPEND ${proj}_DEPENDENCIES zlib)
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} AND (WIN32 OR APPLE))
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  unset(LibArchive_DIR CACHE)
  find_package(LibArchive REQUIRED MODULE)
endif()

# Sanity checks
if(DEFINED LibArchive_DIR AND NOT EXISTS ${LibArchive_DIR})
  message(FATAL_ERROR "LibArchive_DIR variable is defined but corresponds to non-existing directory")
endif()

if(NOT DEFINED LibArchive_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  #
  # NOTE: - a stable, recent release (3.0.4) of LibArchive is now checked out from git
  #         for all platforms.  For notes on cross-platform issues with earlier versions
  #         of LibArchive, see the repository for earlier revisions of this file.

  set(EXTERNAL_PROJECT_OPTIONAL_ARGS)

  # CMake arguments specific to LibArchive >= 2.8.4
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DBUILD_TESTING:BOOL=OFF
    -DENABLE_OPENSSL:BOOL=OFF
    )

  if(NOT DEFINED git_protocol)
    set(git_protocol "git")
  endif()

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${git_protocol}://github.com/libarchive/libarchive.git"
    GIT_TAG "v3.0.4"
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${proj}-build
    INSTALL_DIR LibArchive-install
    CMAKE_CACHE_ARGS
    # Not used -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    # Not used -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DENABLE_ACL:BOOL=OFF
      -DENABLE_CPIO:BOOL=OFF
      -DENABLE_ICONV:BOOL=OFF
      -DENABLE_NETTLE:BOOL=OFF
      -DENABLE_TAR:BOOL=OFF
      -DENABLE_TEST:BOOL=OFF
      -DENABLE_XATTR:BOOL=OFF
      -DCMAKE_DISABLE_FIND_PACKAGE_BZip2:BOOL=ON
      -DCMAKE_DISABLE_FIND_PACKAGE_LibXml2:BOOL=ON
      -DCMAKE_DISABLE_FIND_PACKAGE_EXPAT:BOOL=ON
      -DCMAKE_DISABLE_FIND_PACKAGE_LZMA:BOOL=ON
      -DZLIB_ROOT:PATH=${ZLIB_ROOT}
      -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
      -DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY}
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      ${EXTERNAL_PROJECT_OPTIONAL_ARGS}
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  if(APPLE)
    ExternalProject_Add_Step(${proj} fix_rpath
      COMMAND install_name_tool -id ${CMAKE_BINARY_DIR}/${proj}-install/lib/libarchive.12.dylib ${CMAKE_BINARY_DIR}/${proj}-install/lib/libarchive.12.dylib
      DEPENDEES install
      )
  endif()

  set(LibArchive_DIR ${CMAKE_BINARY_DIR}/LibArchive-install)

  set(LibArchive_INCLUDE_DIR ${LibArchive_DIR}/include)
  if(WIN32)
    set(LibArchive_LIBRARY ${LibArchive_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}archive.lib)
  else()
    set(LibArchive_LIBRARY ${LibArchive_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}archive${CMAKE_SHARED_LIBRARY_SUFFIX})
  endif()

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(LibArchive_DIR:PATH)

mark_as_superbuild(
  VARS
    LibArchive_INCLUDE_DIR:PATH
    LibArchive_LIBRARY:FILEPATH
  LABELS "FIND_PACKAGE"
  )

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  ExternalProject_Message(${proj} "LibArchive_INCLUDE_DIR:${LibArchive_INCLUDE_DIR}")
  ExternalProject_Message(${proj} "LibArchive_LIBRARY:${LibArchive_LIBRARY}")
endif()
