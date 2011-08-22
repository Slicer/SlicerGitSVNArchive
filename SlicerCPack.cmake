if(Slicer_USE_PYTHONQT)
  # Python install rules are common to both 'bundled' and 'regular' package
  include(${Slicer_CMAKE_DIR}/SlicerBlockInstallPython.cmake)
endif()
if(Slicer_USE_PYTHONQT_WITH_TCL)
  # Tcl install rules are common to both 'bundled' and 'regular' package
  include(${Slicer_CMAKE_DIR}/SlicerBlockInstallTcl.cmake)
endif()

include(${Slicer_CMAKE_DIR}/SlicerBlockInstallQtDatabaseDriverPlugins.cmake)

if(NOT APPLE)
  include(${Slicer_CMAKE_DIR}/SlicerBlockInstallQt.cmake)
  include(${Slicer_CMAKE_DIR}/SlicerBlockInstallPythonQt.cmake)
  include(${Slicer_CMAKE_DIR}/SlicerBlockInstallLibArchive.cmake)
  include(InstallRequiredSystemLibraries)
  include(${Slicer_CMAKE_DIR}/SlicerBlockInstallCMakeProjects.cmake)
else()
  # Note: Since CPACK_INSTALL_CMAKE_PROJECTS isn't defined, CPack will default to the current project
  #       and slicer install rules will be considered.
  if(Slicer_USE_PYTHONQT)
    include(${Slicer_CMAKE_DIR}/SlicerBlockInstallExternalPythonModules.cmake)
  endif()
  include(${Slicer_CMAKE_DIR}/SlicerBlockInstallQtImageFormatsPlugins.cmake)

  # Generate qt.conf
  file(WRITE ${Slicer_BINARY_DIR}/Utilities/LastConfigureStep/qt.conf-to-install
"[Paths]
  Plugins = ${Slicer_QtPlugins_DIR}")
  # .. and install
  install(FILES ${Slicer_BINARY_DIR}/Utilities/LastConfigureStep/qt.conf-to-install
          DESTINATION ${Slicer_INSTALL_ROOT}Resources
          COMPONENT Runtime
          RENAME qt.conf)

  set(executable_path @executable_path)
  set(slicer_complete_bundle_directory ${Slicer_BINARY_DIR}/Utilities/LastConfigureStep/SlicerCompleteBundles)
  configure_file(
    "${Slicer_SOURCE_DIR}/Utilities/LastConfigureStep/SlicerCompleteBundles.cmake.in"
    "${slicer_complete_bundle_directory}/SlicerCompleteBundles.cmake"
    @ONLY)
  # HACK - For a given directory, "install(SCRIPT ...)" rule will be evaluated first,
  #        let's make sure the following install rule is evaluated within its own directory.
  #        Otherwise, the associated script will be executed before any other relevant install rules.
  file(WRITE ${slicer_complete_bundle_directory}/CMakeLists.txt
    "install(SCRIPT \"${slicer_complete_bundle_directory}/SlicerCompleteBundles.cmake\")")
  add_subdirectory(${slicer_complete_bundle_directory} ${slicer_complete_bundle_directory}-binary)
endif()

# -------------------------------------------------------------------------
# Package properties
# -------------------------------------------------------------------------
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Medical Visualization and Processing Environment for Research")

set(CPACK_MONOLITHIC_INSTALL ON)

set(CPACK_PACKAGE_VENDOR "NA-MIC")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${Slicer_SOURCE_DIR}/README.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${Slicer_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${Slicer_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${Slicer_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${Slicer_PATCH_VERSION}")
set(CPACK_SYSTEM_NAME "${Slicer_PLATFORM}-${Slicer_ARCHITECTURE}")

if(APPLE)
  set(CPACK_PACKAGE_ICON "${Slicer_SOURCE_DIR}/Applications/SlicerQT/Resources/Slicer.icns")
endif()

# Slicer does *NOT* require setting the windows path
set(CPACK_NSIS_MODIFY_PATH OFF)

set(APPLICATION_NAME "Slicer")
set(EXECUTABLE_NAME "Slicer")
set(CPACK_PACKAGE_EXECUTABLES "..\\\\${EXECUTABLE_NAME}" "${APPLICATION_NAME}")

# -------------------------------------------------------------------------
# File extensions
# -------------------------------------------------------------------------
set(FILE_EXTENSIONS .mrml .xcat)

if(FILE_EXTENSIONS)

  # For NSIS (Win32) now, we will add MacOSX support later (get back to Wes)

  if(WIN32 AND NOT UNIX)
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS)
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS)
    foreach(ext ${FILE_EXTENSIONS})
      string(LENGTH "${ext}" len)
      math(EXPR len_m1 "${len} - 1")
      string(SUBSTRING "${ext}" 1 ${len_m1} ext_wo_dot)
      set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
            WriteRegStr HKCR \\\"${APPLICATION_NAME}\\\" \\\"\\\" \\\"${APPLICATION_NAME} supported file\\\"
            WriteRegStr HKCR \\\"${APPLICATION_NAME}\\\\shell\\\\open\\\\command\\\" \\\"\\\" \\\"$\\\\\\\"$INSTDIR\\\\${EXECUTABLE_NAME}.exe$\\\\\\\" $\\\\\\\"%1$\\\\\\\"\\\"
            WriteRegStr HKCR \\\"${ext}\\\" \\\"\\\" \\\"${APPLICATION_NAME}\\\"
            WriteRegStr HKCR \\\"${ext}\\\" \\\"Content Type\\\" \\\"application/x-${ext_wo_dot}\\\"
          ")
      set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
            DeleteRegKey HKCR \\\" ${APPLICATION_NAME}\\\"
            DeleteRegKey HKCR \\\"${ext}\\\"
          ")
    endforeach()
  endif()
endif()

# -------------------------------------------------------------------------
# Disable source generator enabled by default
# -------------------------------------------------------------------------
set(CPACK_SOURCE_TBZ2 OFF CACHE BOOL "Enable to build TBZ2 source packages" FORCE)
set(CPACK_SOURCE_TGZ  OFF CACHE BOOL "Enable to build TGZ source packages" FORCE)
set(CPACK_SOURCE_TZ   OFF CACHE BOOL "Enable to build TZ source packages" FORCE)

# -------------------------------------------------------------------------
# Enable generator
# -------------------------------------------------------------------------
if(UNIX)
  set(CPACK_GENERATOR "TGZ")
  if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
  endif()
elseif(WIN32)
  set(CPACK_GENERATOR "NSIS")
endif()

include(CPack)

