/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearSlicerLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLinearSlicerLineInterpolator
 * @brief   Interpolates supplied nodes with line segments
 *
 * The line interpolator interpolates supplied nodes (see InterpolateLine)
 * with line segments. The finess of the curve may be controlled using
 * SetMaximumCurveError and SetMaximumNumberOfLineSegments.
 *
 * @sa
 * vtkSlicerLineInterpolator
*/

#ifndef vtkLinearSlicerLineInterpolator_h
#define vtkLinearSlicerLineInterpolator_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerLineInterpolator.h"

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkLinearSlicerLineInterpolator
                          : public vtkSlicerLineInterpolator
{
public:
  /// Instantiate this class.
  static vtkLinearSlicerLineInterpolator *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkLinearSlicerLineInterpolator,vtkSlicerLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Interpolate the line between two nodes.
  int InterpolateLine( vtkSlicerAbstractRepresentation *rep,
                       int idx1, int idx2 ) VTK_OVERRIDE;

protected:
  vtkLinearSlicerLineInterpolator();
  ~vtkLinearSlicerLineInterpolator() VTK_OVERRIDE;

private:
  vtkLinearSlicerLineInterpolator(const vtkLinearSlicerLineInterpolator&) = delete;
  void operator=(const vtkLinearSlicerLineInterpolator&) = delete;
};

#endif
