
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

if(Slicer_USE_CTKAPPLAUNCHER)
  # Sanity checks
  if(DEFINED CTKAPPLAUNCHER_DIR AND NOT EXISTS ${CTKAPPLAUNCHER_DIR})
    message(FATAL_ERROR "CTKAPPLAUNCHER_DIR variable is defined but corresponds to non-existing directory")
  endif()

  # Set dependency list
  set(CTKAPPLAUNCHER_DEPENDENCIES "")

  # Include dependent projects if any
  SlicerMacroCheckExternalProjectDependency(CTKAPPLAUNCHER)
  set(proj CTKAPPLAUNCHER)

  if(NOT DEFINED CTKAPPLAUNCHER_DIR)
    SlicerMacroGetOperatingSystemArchitectureBitness(VAR_PREFIX CTKAPPLAUNCHER)
    set(launcher_version "0.1.9")
    # On windows, use i386 launcher unconditionally
    if("${CTKAPPLAUNCHER_OS}" STREQUAL "win")
      set(CTKAPPLAUNCHER_ARCHITECTURE "i386")
      set(md5 "f48d1dfbcf1581ec28a5fa78b8057d4f")
    elseif("${CTKAPPLAUNCHER_OS}" STREQUAL "linux")
      set(md5 "12203fba411a700b8733db03a75949a7")
    elseif("${CTKAPPLAUNCHER_OS}" STREQUAL "macosx")
      set(md5 "678637a341ff893c6adf8301cac98915")
    endif()
    #message(STATUS "${__indent}Adding project ${proj}")
    ExternalProject_Add(${proj}
      URL http://cloud.github.com/downloads/commontk/AppLauncher/CTKAppLauncher-${launcher_version}-${CTKAPPLAUNCHER_OS}-${CTKAPPLAUNCHER_ARCHITECTURE}.tar.gz
      URL_MD5 ${md5}
      SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
      "${slicer_external_update}"
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      DEPENDS
        ${CTKAPPLAUNCHER_DEPENDENCIES}
      )
    set(CTKAPPLAUNCHER_DIR ${CMAKE_BINARY_DIR}/${proj})
  else()
    # The project is provided using CTKAPPLAUNCHER_DIR, nevertheless since other
    # project may depend on CTKAPPLAUNCHER, let's add an 'empty' one
    SlicerMacroEmptyExternalProject(${proj} "${CTKAPPLAUNCHER_DEPENDENCIES}")
  endif()

endif()
