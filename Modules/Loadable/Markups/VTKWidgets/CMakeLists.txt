project(vtkSlicer${MODULE_NAME}ModuleVTKWidgets)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_VTKWIDGETS_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicer${MODULE_NAME}ModuleMRML_SOURCE_DIR}
  ${vtkSlicer${MODULE_NAME}ModuleMRML_BINARY_DIR}
  )

set(${KIT}_SRCS
  vtk${MODULE_NAME}GlyphSource2D.cxx
  vtk${MODULE_NAME}GlyphSource2D.h
  vtkSlicerAbstractRepresentation.cxx
  vtkSlicerAbstractRepresentation.h
  vtkSlicerAbstractRepresentation3D.cxx
  vtkSlicerAbstractRepresentation3D.h
  vtkSlicerAbstractRepresentation2D.cxx
  vtkSlicerAbstractRepresentation2D.h
  vtkSlicerAbstractWidget.cxx
  vtkSlicerAbstractWidget.h
  vtkSlicerAngleRepresentation2D.cxx
  vtkSlicerAngleRepresentation2D.h
  vtkSlicerAngleRepresentation3D.cxx
  vtkSlicerAngleRepresentation3D.h
  vtkSlicerAngleWidget.cxx
  vtkSlicerAngleWidget.h
  vtkSlicerCurveRepresentation2D.cxx
  vtkSlicerCurveRepresentation2D.h
  vtkSlicerCurveRepresentation3D.cxx
  vtkSlicerCurveRepresentation3D.h
  vtkSlicerCurveWidget.cxx
  vtkSlicerCurveWidget.h
  vtkSlicerLineRepresentation3D.cxx
  vtkSlicerLineRepresentation3D.h
  vtkSlicerLineRepresentation2D.cxx
  vtkSlicerLineRepresentation2D.h
  vtkSlicerLineWidget.cxx
  vtkSlicerLineWidget.h
  vtkSlicerPointsRepresentation3D.cxx
  vtkSlicerPointsRepresentation3D.h
  vtkSlicerPointsRepresentation2D.cxx
  vtkSlicerPointsRepresentation2D.h
  vtkSlicerPointsWidget.cxx
  vtkSlicerPointsWidget.h
  vtkBezierSlicerLineInterpolator.cxx
  vtkBezierSlicerLineInterpolator.h
  vtkLinearSlicerLineInterpolator.cxx
  vtkLinearSlicerLineInterpolator.h
  vtkSlicerLineInterpolator.cxx
  vtkSlicerLineInterpolator.h
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicer${MODULE_NAME}ModuleMRML
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
