/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerPointsRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSlicerPointsRepresentation2D
 * @brief   Default representation for the points widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerPointsWidget. It works in conjunction with the
 * vtkPointPlacer. See vtkSlicerPointsWidget for details.
 * @sa
 * vtkSlicerAbstractRepresentation2D vtkSlicerPointsWidget vtkPointPlacer
*/

#ifndef vtkSlicerPointsRepresentation2D_h
#define vtkSlicerPointsRepresentation2D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation2D.h"

class vtkAppendPolyData;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerPointsRepresentation2D : public vtkSlicerAbstractRepresentation2D
{
public:
  /// Instantiate this class.
  static vtkSlicerPointsRepresentation2D *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerPointsRepresentation2D,vtkSlicerAbstractRepresentation2D);

  /// These are the methods that the widget and its representation use to
  /// communicate with each other.
  void Highlight(int highlight) VTK_OVERRIDE;

  /// Get the points in this widget as a vtkPolyData.
  vtkPolyData *GetWidgetRepresentationAsPolyData() VTK_OVERRIDE;

  /// Return the bounds of the representation
  double *GetBounds() VTK_OVERRIDE;

protected:
  vtkSlicerPointsRepresentation2D();
  ~vtkSlicerPointsRepresentation2D() VTK_OVERRIDE;

  virtual void BuildLines() VTK_OVERRIDE;

  vtkAppendPolyData *appendActors;

private:
  vtkSlicerPointsRepresentation2D(const vtkSlicerPointsRepresentation2D&) = delete;
  void operator=(const vtkSlicerPointsRepresentation2D&) = delete;
};

#endif
