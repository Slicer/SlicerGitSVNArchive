
# Make sure this file is included only once by creating globally unique varibles
# based on the name of this included file.
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

if(NOT WIN32)
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
set(extProjName tk) #The find_package known name
set(proj        tk) #This local name

#if(${USE_SYSTEM_${extProjName}})
#  unset(${extProjName}_DIR CACHE)
#endif()

# Sanity checks
if(DEFINED ${extProjName}_DIR AND NOT EXISTS ${${extProjName}_DIR})
  message(FATAL_ERROR "${extProjName}_DIR variable is defined but corresponds to non-existing directory (${${extProjName}_DIR})")
endif()

# Set dependency list
set(${proj}_DEPENDENCIES "tcl")

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(${proj})

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
  set(tk_SVN_REPOSITORY "http://svn.slicer.org/Slicer3-lib-mirrors/trunk/tcl/tk")
  set(tk_SVN_REVISION -r "114")
  set(tk_SOURCE_DIR "")
  set(tk_BINARY_DIR "")
  set(tk_BUILD_IN_SOURCE 0)
  set(tk_CONFIGURE_COMMAND "")
  set(tk_BUILD_COMMAND "")
  set(tk_INSTALL_COMMAND "")

  set(tk_SOURCE_DIR tcl/tk)
  set(tk_BUILD_IN_SOURCE 1)

  # configure, make and make install all need to be executed in tk/unix. External_Project
  # doesn't provide any way to set the working directory for each step so we do so by
  # configuring a script that has an execute_process command that has the correct working
  # directory
  configure_file(
    SuperBuild/tk_configure_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/tk_configure_step.cmake
    @ONLY)

  set(tk_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/tk_configure_step.cmake)

  configure_file(
    SuperBuild/tk_make_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/tk_make_step.cmake
    @ONLY)

  set(tk_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/tk_make_step.cmake)

  configure_file(
    SuperBuild/tk_install_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/tk_install_step.cmake
    @ONLY)

  set(tk_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/tk_install_step.cmake)

  ### --- End Project specific additions
  ExternalProject_Add(${proj}
    SVN_REPOSITORY ${tk_SVN_REPOSITORY}
    SVN_REVISION ${tk_SVN_REVISION}
    "${cmakeversion_external_disable_update}"
    SOURCE_DIR ${tk_SOURCE_DIR}
    BUILD_IN_SOURCE ${tk_BUILD_IN_SOURCE}
    CONFIGURE_COMMAND ${tk_CONFIGURE_COMMAND}
    BUILD_COMMAND ${tk_BUILD_COMMAND}
    INSTALL_COMMAND ${tk_INSTALL_COMMAND}
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  ExternalProject_Add_Step(${proj} Install_default.h
    COMMAND ${CMAKE_COMMAND} -E copy ${tcl_base}/tk/generic/default.h ${tcl_build}/include
    DEPENDEES install
    )

  ExternalProject_Add_Step(${proj} Install_tkUnixDefault.h
    COMMAND ${CMAKE_COMMAND} -E copy ${tcl_base}/tk/unix/tkUnixDefault.h ${tcl_build}/include
    DEPENDEES install
    )

  #-----------------------------------------------------------------------------
  # Since fixup_bundle expects the library to be writable, let's add an extra step
  # to make sure it's the case.
  if(APPLE)
    foreach(var tcl_build TCL_TK_VERSION_DOT)
      if(NOT DEFINED ${var})
        message(FATAL_ERROR "error: ${var} is not defined !")
      endif()
    endforeach()
    ExternalProject_Add_Step(${proj} tk_install_chmod_library
      COMMAND chmod u+xw ${tcl_build}/lib/libtk${TCL_TK_VERSION_DOT}.dylib
      DEPENDEES install
      )
  endif()

list(APPEND ${CMAKE_PROJECT_NAME}_SUPERBUILD_EP_VARS ${extProjName}_DIR:PATH)

ProjectDependancyPop(CACHED_extProjName extProjName)
ProjectDependancyPop(CACHED_proj proj)
endif()
