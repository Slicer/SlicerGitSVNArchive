# -------------------------------------------------------------------------
# Find and install Qt
# -------------------------------------------------------------------------
set(QT_INSTALL_LIB_DIR ${Slicer_INSTALL_LIB_DIR})

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
