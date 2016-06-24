project(vtkSlicer${MODULE_NAME}ModuleMRMLDisplayableManager)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicerSegmentationsModuleMRML_INCLUDE_DIRS}
  )

set(DisplayableManager_SRCS
  vtkMRML${MODULE_NAME}DisplayableManager3D.cxx
  vtkMRML${MODULE_NAME}DisplayableManager2D.cxx
  )

set(VTK_USE_INSTANTIATOR_NEW 1)
if(${VTK_VERSION_MAJOR} GREATER 5)
  include(${VTK_CMAKE_DIR}/vtkMakeInstantiator.cmake)
endif()
VTK_MAKE_INSTANTIATOR3("${MODULE_NAME}Instantiator"
  DisplayableManagerInstantiator_SRCS
  "${DisplayableManager_SRCS}"
  "${${KIT}_EXPORT_DIRECTIVE}"
  ${CMAKE_CURRENT_BINARY_DIR}
  "${KIT}Export.h"
  )

set(${KIT}_SRCS
  ${DisplayableManagerInstantiator_SRCS}
  ${DisplayableManager_SRCS}
  )

set(${KIT}_VTK_LIBRARIES)
if(${VTK_VERSION_MAJOR} LESS 6)
  set(${KIT}_VTK_LIBRARIES vtkRendering)
endif()

set(${KIT}_TARGET_LIBRARIES
  ${MRML_LIBRARIES}
  ${${KIT}_VTK_LIBRARIES}
  vtkSlicerSegmentationsModuleMRML
  )

SET (${KIT}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${vtkSegmentationCore_INCLUDE_DIRS} ${Slicer_Base_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
