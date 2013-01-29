
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
set(extProjName Teem) #The find_package known name
set(proj        Teem) #This local name

#if(${USE_SYSTEM_${extProjName}})
#  unset(${extProjName}_DIR CACHE)
#endif()

# Sanity checks
if(DEFINED ${extProjName}_DIR AND NOT EXISTS ${${extProjName}_DIR})
  message(FATAL_ERROR "${extProjName}_DIR variable is defined but corresponds to non-existing directory (${${extProjName}_DIR})")
endif()

# Set dependency list
set(${proj}_DEPENDENCIES VTK)

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

  set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG)
  if(CTEST_USE_LAUNCHERS)
    set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG
      "-DCMAKE_PROJECT_Teem_INCLUDE:FILEPATH=${CMAKE_ROOT}/Modules/CTestUseLaunchers.cmake")
  endif()

  ### --- Project specific additions here

  if(WIN32)
    set(${proj}_ZLIB_LIBRARY ${VTK_DIR}/bin/${CMAKE_CFG_INTDIR}/vtkzlib.lib)
    set(${proj}_PNG_LIBRARY ${VTK_DIR}/bin/${CMAKE_CFG_INTDIR}/vtkpng.lib)
  elseif(APPLE)
    set(${proj}_ZLIB_LIBRARY ${VTK_DIR}/bin/libvtkzlib.dylib)
    set(${proj}_PNG_LIBRARY ${VTK_DIR}/bin/libvtkpng.dylib)
  else()
    set(${proj}_ZLIB_LIBRARY ${VTK_DIR}/bin/libvtkzlib.so)
    set(${proj}_PNG_LIBRARY ${VTK_DIR}/bin/libvtkpng.so)
  endif()

  set(${proj}_CMAKE_OPTIONS
    -DTeem_USE_LIB_INSTALL_SUBDIR:BOOL=ON
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
    -DTeem_PTHREAD:BOOL=OFF
    -DTeem_BZIP2:BOOL=OFF
    -DTeem_ZLIB:BOOL=ON
    -DTeem_PNG:BOOL=ON
    -DTeem_VTK_MANGLE:BOOL=ON
    -DTeem_VTK_TOOLKITS_IPATH:FILEPATH=${VTK_DIR}
    -DZLIB_INCLUDE_DIR:PATH=${VTK_SOURCE_DIR}/Utilities
    -DTeem_VTK_ZLIB_MANGLE_IPATH:PATH=${VTK_SOURCE_DIR}/Utilities/vtkzlib
    -DTeem_ZLIB_DLLCONF_IPATH:PATH=${VTK_DIR}/Utilities
    -DZLIB_LIBRARY:FILEPATH=${${proj}_ZLIB_LIBRARY}
    -DPNG_PNG_INCLUDE_DIR:PATH=${VTK_SOURCE_DIR}/Utilities/vtkpng
    -DTeem_PNG_DLLCONF_IPATH:PATH=${VTK_DIR}/Utilities
    -DPNG_LIBRARY:FILEPATH=${${proj}_PNG_LIBRARY}
  )
  ### --- End Project specific additions
  set(${proj}_URL http://svn.slicer.org/Slicer3-lib-mirrors/trunk/teem-1.10.0-src.tar.gz)
  set(${proj}_MD5 efe219575adc89f6470994154d86c05b)
  ExternalProject_Add(${proj}
    URL ${${proj}_URL}
    URL_MD5 ${${proj}_MD5}
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${proj}
    BINARY_DIR ${proj}-build
    INSTALL_COMMAND ""
    "${cmakeversion_external_update}"
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
  # Not needed -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DBUILD_TESTING:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=ON
      ${CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG}
      ${${proj}_CMAKE_OPTIONS}
    DEPENDS
      ${${proj}_DEPENDENCIES}
  )
  set(${proj}_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
else()
  if(${USE_SYSTEM_${extProjName}})
    find_package(${extProjName} REQUIRED)
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
