
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
set(extProjName VTK) #The find_package known name
set(proj        VTK) #This local name

#if(${USE_SYSTEM_${extProjName}})
#  unset(${extProjName}_DIR CACHE)
#endif()

# Sanity checks
if(DEFINED ${extProjName}_DIR AND NOT EXISTS ${${extProjName}_DIR})
  message(FATAL_ERROR "${extProjName}_DIR variable is defined but corresponds to non-existing directory (${${extProjName}_DIR})")
endif()

if(DEFINED ${extProjName}_SOURCE_DIR AND NOT EXISTS ${${extProjName}_SOURCE_DIR})
  message(FATAL_ERROR "${extProjName}_SOURCE_DIR variable is defined but corresponds to non-existing directory")
endif()

# Set dependency list
set(${proj}_DEPENDENCIES "")
if (${PROJECT_NAME}_USE_PYTHONQT)
  list(APPEND ${proj}_DEPENDENCIES python)
endif()

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(${proj})

if( ( NOT DEFINED ${extProjName}_DIR AND NOT ${USE_SYSTEM_${extProjName}} )
                                  OR NOT DEFINED ${extProjName}_SOURCE_DIR)
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
  set(VTK_WRAP_TCL OFF)
  set(VTK_WRAP_PYTHON OFF)

  if (${PROJECT_NAME}_USE_PYTHONQT)
    set(VTK_WRAP_PYTHON ON)
  endif()

  set(VTK_PYTHON_ARGS)
  if(${PROJECT_NAME}_USE_PYTHONQT)
    set(VTK_PYTHON_ARGS
      -DVTK_INSTALL_PYTHON_USING_CMAKE:BOOL=ON
      -DPYTHON_EXECUTABLE:PATH=${slicer_PYTHON_EXECUTABLE}
      -DPYTHON_INCLUDE_DIR:PATH=${slicer_PYTHON_INCLUDE}
      -DPYTHON_LIBRARY:FILEPATH=${slicer_PYTHON_LIBRARY}
      )
  endif()

  set(VTK_QT_ARGS)
  if(NOT APPLE)
    set(VTK_QT_ARGS
      #-DDESIRED_QT_VERSION:STRING=4 # Unused
      -DVTK_USE_GUISUPPORT:BOOL=ON
      -DVTK_USE_QVTK_QTOPENGL:BOOL=ON
      -DVTK_USE_QT:BOOL=ON
      -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
      )
  else()
    set(VTK_QT_ARGS
      -DVTK_USE_CARBON:BOOL=OFF
      -DVTK_USE_COCOA:BOOL=ON # Default to Cocoa, VTK/CMakeLists.txt will enable Carbon and disable cocoa if needed
      -DVTK_USE_X:BOOL=OFF
      #-DVTK_USE_RPATH:BOOL=ON # Unused
      #-DDESIRED_QT_VERSION:STRING=4 # Unused
      -DVTK_USE_GUISUPPORT:BOOL=ON
      -DVTK_USE_QVTK_QTOPENGL:BOOL=ON
      -DVTK_USE_QT:BOOL=ON
      -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
      )
  endif()

  # Disable Tk when Python wrapping is enabled
  if (${PROJECT_NAME}_USE_PYTHONQT)
    list(APPEND VTK_QT_ARGS -DVTK_USE_TK:BOOL=OFF)
  endif()

  set(slicer_TCL_LIB)
  set(slicer_TK_LIB)
  set(slicer_TCLSH)
  set(VTK_TCL_ARGS)
  if(VTK_WRAP_TCL)
    if(WIN32)
      set(slicer_TCL_LIB ${CMAKE_BINARY_DIR}/tcl-build/lib/tcl84.lib)
      set(slicer_TK_LIB ${CMAKE_BINARY_DIR}/tcl-build/lib/tk84.lib)
      set(slicer_TCLSH ${CMAKE_BINARY_DIR}/tcl-build/bin/tclsh.exe)
    elseif(APPLE)
      set(slicer_TCL_LIB ${CMAKE_BINARY_DIR}/tcl-build/lib/libtcl8.4.dylib)
      set(slicer_TK_LIB ${CMAKE_BINARY_DIR}/tcl-build/lib/libtk8.4.dylib)
      set(slicer_TCLSH ${CMAKE_BINARY_DIR}/tcl-build/bin/tclsh84)
    else()
      set(slicer_TCL_LIB ${CMAKE_BINARY_DIR}/tcl-build/lib/libtcl8.4.so)
      set(slicer_TK_LIB ${CMAKE_BINARY_DIR}/tcl-build/lib/libtk8.4.so)
      set(slicer_TCLSH ${CMAKE_BINARY_DIR}/tcl-build/bin/tclsh84)
    endif()
    set(VTK_TCL_ARGS
      -DTCL_INCLUDE_PATH:PATH=${CMAKE_BINARY_DIR}/tcl-build/include
      -DTK_INCLUDE_PATH:PATH=${CMAKE_BINARY_DIR}/tcl-build/include
      -DTCL_LIBRARY:FILEPATH=${slicer_TCL_LIB}
      -DTK_LIBRARY:FILEPATH=${slicer_TK_LIB}
      -DTCL_TCLSH:FILEPATH=${slicer_TCLSH}
      )
  endif()

  set(CUSTOM_BUILD_COMMAND)
  if(CMAKE_GENERATOR MATCHES ".*Makefiles.*")
    # Use ${MAKE} as build command to propagate parallel make option
    set(CUSTOM_BUILD_COMMAND BUILD_COMMAND "$(MAKE)")
    set(make_command_definition -DMAKE_COMMAND=$(MAKE) )
  else()
    set(make_command_definition -DMAKE_COMMAND=${MAKECOMMAND})
  endif()

  if(UNIX)
    configure_file(SuperBuild/VTK_build_step.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/VTK_build_step.cmake
      @ONLY)
    set(CUSTOM_BUILD_COMMAND BUILD_COMMAND ${CMAKE_COMMAND}
      ${make_command_definition}
      -P ${CMAKE_CURRENT_BINARY_DIR}/VTK_build_step.cmake)
  endif()


  set(${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY "github.com/Slicer/VTK.git" CACHE STRING "Repository from which to get VTK" FORCE)
  set(${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG "03ddda8cd503b957268fee138c7b57465e5842a4" CACHE STRING "VTK git tag to use" FORCE)

  mark_as_advanced(${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG)

  if(NOT DEFINED git_protocol)
      set(git_protocol "git")
  endif()

  set(${proj}_CMAKE_OPTIONS
      -DBUILD_TESTING:BOOL=OFF
      -DBUILD_EXAMPLES:BOOL=OFF
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DVTK_USE_PARALLEL:BOOL=ON
      -DVTK_DEBUG_LEAKS:BOOL=${${PROJECT_NAME}_USE_VTK_DEBUG_LEAKS}
      -DVTK_LEGACY_REMOVE:BOOL=ON
      -DVTK_WRAP_TCL:BOOL=${VTK_WRAP_TCL}
      #-DVTK_USE_RPATH:BOOL=ON # Unused
      ${VTK_TCL_ARGS}
      -DVTK_WRAP_PYTHON:BOOL=${VTK_WRAP_PYTHON}
      -DVTK_INSTALL_LIB_DIR:PATH=${${PROJECT_NAME}_INSTALL_LIB_DIR}
      ${VTK_PYTHON_ARGS}
      ${VTK_QT_ARGS}
      ${VTK_MAC_ARGS}
  )
  ### --- End Project specific additions
  set(${proj}_REPOSITORY "${git_protocol}://${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}" )
  set(${proj}_GIT_TAG ${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG} )
  ExternalProject_Add(${proj}
    GIT_REPOSITORY ${${proj}_REPOSITORY}
    GIT_TAG ${${proj}_GIT_TAG}
    SOURCE_DIR ${proj}
    BINARY_DIR ${proj}-build
    "${cmakeversion_external_update}"
    CMAKE_GENERATOR ${gen}
    ${CUSTOM_BUILD_COMMAND}
    CMAKE_ARGS
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
      ${${proj}_CMAKE_OPTIONS}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )
  set(${extProjName}_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
  set(${extProjName}_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
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
