# -------------------------------------------------------------------------
# Find and install Qt
# -------------------------------------------------------------------------
set(QT_INSTALL_LIB_DIR ${Slicer_INSTALL_LIB_DIR})

if(Slicer_REQUIRED_QT_VERSION VERSION_LESS "5")

  foreach(qtlib ${Slicer_REQUIRED_QT_MODULES})
    if(QT_${qtlib}_LIBRARY_RELEASE)
      if(APPLE)
        install(DIRECTORY "${QT_${qtlib}_LIBRARY_RELEASE}"
          DESTINATION ${QT_INSTALL_LIB_DIR} COMPONENT Runtime)
      elseif(UNIX)
        # Install .so and versioned .so.x.y
        get_filename_component(QT_LIB_DIR_tmp ${QT_${qtlib}_LIBRARY_RELEASE} PATH)
        get_filename_component(QT_LIB_NAME_tmp ${QT_${qtlib}_LIBRARY_RELEASE} NAME)
        install(DIRECTORY ${QT_LIB_DIR_tmp}/
          DESTINATION ${QT_INSTALL_LIB_DIR} COMPONENT Runtime
          FILES_MATCHING PATTERN "${QT_LIB_NAME_tmp}*"
          PATTERN "${QT_LIB_NAME_tmp}*.debug" EXCLUDE)
      elseif(WIN32)
        get_filename_component(QT_DLL_PATH_tmp ${QT_QMAKE_EXECUTABLE} PATH)
        if(EXISTS "${QT_DLL_PATH_tmp}/${qtlib}4.dll")
          install(FILES ${QT_DLL_PATH_tmp}/${qtlib}4.dll
            DESTINATION bin COMPONENT Runtime)
        endif()
      endif()
    endif()
  endforeach()

else()

  list(APPEND QT_LIBRARIES
    "Qt5::Gui"
    )

  # WebEngine Dependencies
  if("Qt5::WebEngine" IN_LIST QT_LIBRARIES)
    find_package(Qt5 REQUIRED COMPONENTS
      Positioning
      Qml
      Quick
      QuickWidgets
      )
    list(APPEND QT_LIBRARIES
      "Qt5::Positioning"
      "Qt5::Qml"
      "Qt5::Quick"
      "Qt5::QuickWidgets"
      "Qt5::WebEngineCore"
      )
  endif()

  if(UNIX)

    # Get root directory
    get_property(_filepath TARGET "Qt5::Core" PROPERTY LOCATION_RELEASE)
    get_filename_component(_dir ${_filepath} PATH)
    set(qt_root_dir "${_dir}/..")

    find_package(Qt5 REQUIRED COMPONENTS
      DBus
      X11Extras
      )
    list(APPEND QT_LIBRARIES
      "Qt5::DBus"
      "Qt5::X11Extras"
      )

    # XcbQpa
    slicerInstallLibrary(FILE ${qt_root_dir}/lib/libQt5XcbQpa.so
      DESTINATION ${QT_INSTALL_LIB_DIR} COMPONENT Runtime
      )

    # ICU libraries
    foreach(iculib IN ITEMS data i18n io le lx test tu uc)
      slicerInstallLibrary(FILE ${qt_root_dir}/lib/libicu${iculib}.so
        DESTINATION ${QT_INSTALL_LIB_DIR} COMPONENT Runtime
        )
    endforeach()

    # WebEngine Dependencies
    if("Qt5::WebEngine" IN_LIST QT_LIBRARIES)
      install(PROGRAMS ${qt_root_dir}/libexec/QtWebEngineProcess
        DESTINATION ${Slicer_INSTALL_ROOT}/libexec COMPONENT Runtime
        )
    endif()

    set(_qt_version "${Qt5_VERSION_MAJOR}.${Qt5_VERSION_MINOR}.${Qt5_VERSION_PATCH}")

    foreach(target ${QT_LIBRARIES})
      get_target_property(type ${target} TYPE)
      if(NOT type STREQUAL "SHARED_LIBRARY")
        continue()
      endif()
      get_property(location TARGET ${target} PROPERTY LOCATION_RELEASE)
      if(UNIX)
        # Install .so and versioned .so.x.y
        get_filename_component(QT_LIB_DIR_tmp ${location} PATH)
        get_filename_component(QT_LIB_NAME_tmp ${location} NAME)
        string(REPLACE ".${_qt_version}" "" QT_LIB_NAME_tmp ${QT_LIB_NAME_tmp})
        install(DIRECTORY ${QT_LIB_DIR_tmp}/
          DESTINATION ${QT_INSTALL_LIB_DIR} COMPONENT Runtime
          FILES_MATCHING PATTERN "${QT_LIB_NAME_tmp}*"
          PATTERN "${QT_LIB_NAME_tmp}*.debug" EXCLUDE)
      endif()
    endforeach()

    # Install resources directory
    set(resources_dir "${qt_root_dir}/resources")
    install(DIRECTORY ${resources_dir}
      DESTINATION ${Slicer_INSTALL_ROOT} COMPONENT Runtime
      )

    # Install webengine translations
    set(translations_dir "${qt_root_dir}/translations/qtwebengine_locales")
    install(DIRECTORY ${translations_dir}
      DESTINATION ${Slicer_INSTALL_ROOT}/share/QtTranslations/ COMPONENT Runtime
      )

    # Configure and install qt.conf
    set(qt_conf_contents "[Paths]\nPrefix = ..\nPlugins = ${Slicer_INSTALL_QtPlugins_DIR}\nTranslations = share/QtTranslations")
    install(
      CODE "file(WRITE \"\${CMAKE_INSTALL_PREFIX}/bin/qt.conf\" \"${qt_conf_contents}\")"
      COMPONENT Runtime
      )

  elseif(WIN32)
    # Get location of windeployqt.exe based on uic.exe location
    get_target_property(uic_location Qt5::uic IMPORTED_LOCATION)
    get_filename_component(_dir ${uic_location} DIRECTORY)
    set(windeployqt "${_dir}/windeployqt.exe")
    if(NOT EXISTS ${windeployqt})
      message(FATAL_ERROR "Failed to locate windeployqt executable: [${windeployqt}]")
    endif()
    set(_args "--release --no-compiler-runtime --no-translations --no-plugins")

    foreach(target IN LISTS QT_LIBRARIES)
      get_target_property(type ${target} TYPE)
      if(NOT type STREQUAL "SHARED_LIBRARY")
        continue()
      endif()
      string(REPLACE "Qt5::" "" module ${target})
      string(TOLOWER ${module} module_lc)
      set(_args "${_args} -${module_lc}")
    endforeach()

    set(executable "${Slicer_MAIN_PROJECT_APPLICATION_NAME}App-real.exe")

    install(
      CODE "
        set(ENV{PATH} \"${QT_BINARY_DIR};\$ENV{PATH}\")
        execute_process(COMMAND \"${windeployqt}\" ${_args} \"\${CMAKE_INSTALL_PREFIX}/bin/${executable}\")
      "
      COMPONENT Runtime
      )
  endif()
endif()
