/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

/// \brief vtkOrientedGridTransform - arbitrarily oriented displacement field
/// deformation transformation.
///
/// This transforms extends vtkGridTransform to arbitrary grid orientation.
///

#ifndef __vtkOrientedGridTransform_h
#define __vtkOrientedGridTransform_h

#include "vtkAddon.h"

#include "vtkCommand.h"
#include "vtkGridTransform.h"

class VTK_ADDON_EXPORT vtkOrientedGridTransform : public vtkGridTransform
{
public:
  static vtkOrientedGridTransform *New();
  vtkTypeMacro(vtkOrientedGridTransform,vtkGridTransform);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Set/Get the b-spline grid axis directions.
  // This transform class will never modify the data.
  // Must be an orthogonal, normalized matrix.
  // The 4th column and 4th row are ignored.
  virtual void SetGridDirectionMatrix(vtkMatrix4x4*);
  vtkGetObjectMacro(GridDirectionMatrix,vtkMatrix4x4);

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform() override;

  /// List of custom events fired by the class.
  // ConvergenceFailureEvent is invoked when the gradient cannot be
  // inverted, probably due to a singular transform or numeric instability.
  enum Events
    {
    ConvergenceFailureEvent = vtkCommand::UserEvent + 1
    };

protected:
  vtkOrientedGridTransform();
  ~vtkOrientedGridTransform() override;

  // Description:
  // Update the displacement grid.
  void InternalUpdate() override;

  // Description:
  // Copy this transform from another of the same type.
  void InternalDeepCopy(vtkAbstractTransform *transform) override;

  // Avoid hiding overloads from base class... these will include the float
  // overloads that forward to the double overloads (hence no need to override
  // the float versions)
  using vtkGridTransform::ForwardTransformPoint;
  using vtkGridTransform::ForwardTransformDerivative;
  using vtkGridTransform::InverseTransformDerivative;

  // Description:
  // Internal functions for calculating the transformation.
  void ForwardTransformPoint(const double in[3], double out[3]) override;

  void ForwardTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]) override;

  void InverseTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]) override;

  // Description:
  // Grid axis direction vectors (i, j, k) in the output space
  vtkMatrix4x4* GridDirectionMatrix;

  // Description:
  // Transforms a point from the output coordinate system to the
  // grid index (IJK) coordinate system.
  vtkMatrix4x4* GridIndexToOutputTransformMatrixCached;
  vtkMatrix4x4* OutputToGridIndexTransformMatrixCached;

  // Description:
  // Avoid generating hundreds of warning messages for convergence problems
  // by keeping track of the MTime when the last warning was issued.
  vtkMTimeType LastWarningMTime;

private:
  vtkOrientedGridTransform(const vtkOrientedGridTransform&) = delete;
  void operator=(const vtkOrientedGridTransform&) = delete;
};

#endif
