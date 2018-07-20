# -------------------------------------------------------------------------
# Find and install python
# -------------------------------------------------------------------------
if(Slicer_USE_PYTHONQT)

  # Sanity checks
  foreach(varname IN ITEMS Slicer_SUPERBUILD_DIR PYTHON_STDLIB_SUBDIR)
    if("${${varname}}" STREQUAL "")
      message(FATAL_ERROR "${varname} CMake variable is expected to be set")
    endif()
  endforeach()

  set(PYTHON_DIR "${Slicer_SUPERBUILD_DIR}/python-install")
  if(NOT EXISTS "${PYTHON_DIR}/${PYTHON_STDLIB_SUBDIR}")
    message(FATAL_ERROR "Skipping generation of python install rules - Unexistant directory PYTHON_DIR:${PYTHON_DIR}/${PYTHON_STDLIB_SUBDIR}")
    return()
  endif()

  # Install libraries
  
  set(extra_exclude_pattern)
  if(UNIX)
    list(APPEND extra_exclude_pattern
      REGEX "distutils/command/wininst-.*" EXCLUDE
      )
  endif()

  install(
    DIRECTORY "${PYTHON_DIR}/${PYTHON_STDLIB_SUBDIR}/"
    DESTINATION "${Slicer_INSTALL_ROOT}lib/Python/${PYTHON_STDLIB_SUBDIR}/"
    COMPONENT Runtime
    USE_SOURCE_PERMISSIONS
    REGEX "lib2to3/" EXCLUDE
    REGEX "lib[-]old/" EXCLUDE
    REGEX "plat[-].*" EXCLUDE
    REGEX "/test/" EXCLUDE
    REGEX "wsgiref*" EXCLUDE
    ${extra_exclude_pattern}
    )
  # Install python library
  if(UNIX)
    if(NOT APPLE)
      slicerInstallLibrary(
        FILE ${PYTHON_LIBRARY}
        DESTINATION ${Slicer_INSTALL_ROOT}lib/Python/lib
        COMPONENT Runtime
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ)
    endif()
  elseif(WIN32)
    get_filename_component(PYTHON_LIB_BASE ${PYTHON_LIBRARY} NAME_WE)
    install(FILES "${PYTHON_LIBRARY_PATH}/${PYTHON_LIB_BASE}.dll"
      DESTINATION bin
      COMPONENT Runtime)
  endif()

  # Install interpreter
  get_filename_component(python_bin_dir ${PYTHON_EXECUTABLE} PATH)
  install(
    PROGRAMS ${python_bin_dir}/python${CMAKE_EXECUTABLE_SUFFIX}
    DESTINATION ${Slicer_INSTALL_BIN_DIR}
    RENAME python-real${CMAKE_EXECUTABLE_SUFFIX}
    COMPONENT Runtime
    )

  if(APPLE)
    # Fixes Slicer issue #4554
    set(dollar "$")
    install(CODE
      "set(app ${Slicer_INSTALL_BIN_DIR}/python-real)
       set(appfilepath \"${dollar}ENV{DESTDIR}${dollar}{CMAKE_INSTALL_PREFIX}/${dollar}{app}\")
       message(\"CPack: - Adding rpath to ${dollar}{app}\")
       execute_process(COMMAND install_name_tool -add_rpath @loader_path/..  ${dollar}{appfilepath})"
      COMPONENT Runtime
      )
  endif()

  # Install Slicer python launcher settings

  macro(_install_python_launcher executablename)
    # Install Slicer python launcher settings
    install(
      FILES ${python_bin_dir}/${executablename}LauncherSettingsToInstall.ini
      DESTINATION ${Slicer_INSTALL_BIN_DIR}
      RENAME ${executablename}LauncherSettings.ini
      COMPONENT Runtime
      )
    # Install Slicer python launcher
    set(_launcher CTKAppLauncher)
    if(Slicer_BUILD_WIN32_CONSOLE)
      set(_launcher CTKAppLauncherW)
    endif()
    install(
      PROGRAMS ${CTKAppLauncher_DIR}/bin/${_launcher}${CMAKE_EXECUTABLE_SUFFIX}
      DESTINATION ${Slicer_INSTALL_BIN_DIR}
      RENAME ${executablename}${CMAKE_EXECUTABLE_SUFFIX}
      COMPONENT Runtime
      )
  endmacro()

  _install_python_launcher(PythonSlicer)

  # SlicerPython executable is deprecated, see details in External_python.cmake
  _install_python_launcher(SlicerPython)

  # Install headers
  set(python_include_subdir /Include/)
  if(UNIX)
    set(python_include_subdir /include/python2.7/)
  endif()

  install(FILES "${PYTHON_DIR}${python_include_subdir}/pyconfig.h"
    DESTINATION ${Slicer_INSTALL_ROOT}lib/Python${python_include_subdir}
    COMPONENT Runtime
    )

endif()

