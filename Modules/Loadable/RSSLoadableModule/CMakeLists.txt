cmake_minimum_required(VERSION 2.8.7)

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  set(EXTENSION_NAME RSSLoadableModule)
  set(EXTENSION_HOMEPAGE "http://www.slicer.org/slicerWiki/index.php/Documentation/4.1/Extensions/RSSLoadableModule")
  set(EXTENSION_CATEGORY "Examples")
  set(EXTENSION_CONTRIBUTORS "Jean-Christophe Fillion-Robin (Kitware)")
  set(EXTENSION_DESCRIPTION "This is an example of extension bundling a loadable module")
  set(EXTENSION_ICONURL "http://viewvc.slicer.org/viewvc.cgi/Slicer4/trunk/Extensions/Testing/RSSLoadableModule/Resources/Icons/RSSLoadableModule.png?revision=19437&view=co")
  set(EXTENSION_SCREENSHOTURLS "http://wiki.slicer.org/slicerWiki/images/4/42/Slicer-r19441-RSSLoadableModule-screenshot.png")
endif()

#-----------------------------------------------------------------------------
set(MODULE_NAME RSSLoadableModule)
set(MODULE_TITLE ${MODULE_NAME})

string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  find_package(Slicer REQUIRED)
  include(${Slicer_USE_FILE})
endif()

#-----------------------------------------------------------------------------
add_subdirectory(Logic)

#-----------------------------------------------------------------------------
set(MODULE_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_${MODULE_NAME_UPPER}_EXPORT")

# Current_{source,binary} and Slicer_{Libs,Base} already included
set(MODULE_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  )

set(MODULE_SRCS
  qSlicerRSSLoadableModuleModule.cxx
  qSlicerRSSLoadableModuleModule.h
  qSlicerRSSLoadableModuleModuleWidget.cxx
  qSlicerRSSLoadableModuleModuleWidget.h
  )

set(MODULE_MOC_SRCS
  qSlicerRSSLoadableModuleModule.h
  qSlicerRSSLoadableModuleModuleWidget.h
  )

set(MODULE_UI_SRCS
  Resources/UI/qSlicerRSSLoadableModuleModule.ui
  )

set(MODULE_TARGET_LIBRARIES
  vtkSlicerRSSLoadableModuleModuleLogic
  )

set(MODULE_RESOURCES
  Resources/qSlicerRSSLoadableModuleModule.qrc
  )

#-----------------------------------------------------------------------------
slicerMacroBuildQtModule(
  NAME ${MODULE_NAME}
  TITLE ${MODULE_TITLE}
  EXPORT_DIRECTIVE ${MODULE_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${MODULE_INCLUDE_DIRECTORIES}
  SRCS ${MODULE_SRCS}
  MOC_SRCS ${MODULE_MOC_SRCS}
  UI_SRCS ${MODULE_UI_SRCS}
  TARGET_LIBRARIES ${MODULE_TARGET_LIBRARIES}
  RESOURCES ${MODULE_RESOURCES}
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  include(${Slicer_EXTENSION_CPACK})
endif()
