
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Set dependency list
set(teem_DEPENDENCIES zlib VTK)

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(teem)
set(proj teem)

set(EXTERNAL_PROJECT_OPTIONAL_ARGS)

# Set CMake OSX variable to pass down the external project
if(APPLE)
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

if(NOT CMAKE_CONFIGURATION_TYPES)
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE})
endif()

set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG)
if(CTEST_USE_LAUNCHERS)
  set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG
    "-DCMAKE_PROJECT_Teem_INCLUDE:FILEPATH=${CMAKE_ROOT}/Modules/CTestUseLaunchers.cmake")
endif()

#message(STATUS "${__indent}Adding project ${proj}")

set(teem_PNG_LIBRARY_DIR ${VTK_DIR}/bin)
if(CMAKE_CONFIGURATION_TYPES)
  set(teem_PNG_LIBRARY_DIR ${teem_PNG_LIBRARY_DIR}/${CMAKE_CFG_INTDIR})
endif()
if(WIN32)
  set(teem_PNG_LIBRARY ${teem_PNG_LIBRARY_DIR}/vtkpng.lib)
elseif(APPLE)
  set(teem_PNG_LIBRARY ${teem_PNG_LIBRARY_DIR}/libvtkpng.dylib)
else()
  set(teem_PNG_LIBRARY ${teem_PNG_LIBRARY_DIR}/libvtkpng.so)
endif()

if(${CMAKE_VERSION} VERSION_GREATER "2.8.11.2")
  # Following CMake commit 2a7975398, the FindPNG.cmake module
  # supports detection of release and debug libraries. Specifying only
  # the release variable is enough to ensure the variable PNG_LIBRARY
  # is internally set if the project is built either in Debug or Release.
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DPNG_LIBRARY_RELEASE:FILEPATH=${teem_PNG_LIBRARY}
    )
else()
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_ARGS
    -DPNG_LIBRARY:FILEPATH=${teem_PNG_LIBRARY}
    )
endif()

set(teem_URL http://svn.slicer.org/Slicer3-lib-mirrors/trunk/teem-1.10.0-src.tar.gz)
set(teem_MD5 efe219575adc89f6470994154d86c05b)

ExternalProject_Add(${proj}
  URL ${teem_URL}
  URL_MD5 ${teem_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR teem
  BINARY_DIR teem-build
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    # Not needed -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
    -DBUILD_TESTING:BOOL=OFF
    -DBUILD_SHARED_LIBS:BOOL=ON
    ${CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG}
    -DTeem_USE_LIB_INSTALL_SUBDIR:BOOL=ON
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
    -DTeem_PTHREAD:BOOL=OFF
    -DTeem_BZIP2:BOOL=OFF
    -DTeem_ZLIB:BOOL=ON
    -DTeem_PNG:BOOL=ON
    -DZLIB_ROOT:PATH=${SLICER_ZLIB_ROOT}
    -DZLIB_INCLUDE_DIR:PATH=${SLICER_ZLIB_INCLUDE_DIR}
    -DZLIB_LIBRARY:FILEPATH=${SLICER_ZLIB_LIBRARY}
    -DTeem_VTK_MANGLE:BOOL=OFF ## NOT NEEDED FOR EXTERNAL ZLIB outside of vtk
    -DPNG_PNG_INCLUDE_DIR:PATH=${VTK_SOURCE_DIR}/Utilities/vtkpng
    -DTeem_PNG_DLLCONF_IPATH:PATH=${VTK_DIR}/Utilities
    ${EXTERNAL_PROJECT_OPTIONAL_ARGS}
  INSTALL_COMMAND ""
  DEPENDS
    ${teem_DEPENDENCIES}
  )

set(Teem_DIR ${CMAKE_BINARY_DIR}/teem-build)

