
#ifndef __vtkITKBSplineTransform_h
#define __vtkITKBSplineTransform_h

#include "vtkWarpTransform.h"
#include "vtkDoubleArray.h"

#include "itkBSplineDeformableTransform.h"
#include "itkAffineTransform.h"

#include "vtkITK.h"

class vtkITKBSplineTransformHelper;

/// \brief A BSpline transform.
//
/// See the ITK BSplineTransform documentation for details on the
/// underlying functionality and dependencies of this class.
class VTK_ITK_EXPORT vtkITKBSplineTransform : public vtkWarpTransform
{
public:
  typedef itk::AffineTransform<double,3> BulkTransformType;
  static vtkITKBSplineTransform *New();
  vtkTypeRevisionMacro( vtkITKBSplineTransform, vtkWarpTransform );
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  vtkAbstractTransform* MakeTransform();

  /// SetOrder MUST be called first before other set functions.
  void SetSplineOrder( unsigned int );

  unsigned int GetSplineOrder() const;

  /// The origin of the B-spline grid must be set at one grid position
  /// away from the origin of the desired output image.
  void SetGridOrigin( const double origin[3] );

  void GetGridOrigin(double *origin) const;

  /// The spacing between grid nodes.
  void SetGridSpacing( const double spacing[3] );

  void GetGridSpacing(double *spacing) const;

  /// Number of grid nodes in each dimension.
  //
  /// Note that there need to
  /// be extra nodes "outside" the valid input region.  The number of
  /// such extra nodes depends on the order of the BSpline; for a cubic
  /// (order 3) BSpline, there needs to be 3 extra nodes in each
  /// dimension.
  void SetGridSize( const unsigned int size[3] );

  void GetGridSize(unsigned int *size) const;

  /// See the documentation of SetParameters(double[]).
  void SetParameters( vtkDoubleArray& param );

  /// Set the BSpline parameters.
  //
  /// There should be this->GetNumberOfParameters() parameters in this
  /// vector.  The parameters can be thought of as the required
  /// displacement each node in each dimension.
  //
  /// Suppose the grid (set by SetGridSize) is LxNxM.  Then, the first
  /// LxNxM values represent the x-displacement, the next LxNxM values
  /// represent the y-displacement, and finally, the last LxNxM values
  /// represent the z-displacement.  For each space dimension, the
  /// LxNxM values are the values at the corresponding grid points,
  /// vectorized by traversing the grid in the x-dimension first, then
  /// the y-dimension, and finally the z-dimension.  That is, if
  /// param[17] is for grid node (1,2,3), then param[18] is for grid
  /// node (2,2,3).
  //
  /// The parameter values in \a param are copied, and hence can be
  /// released after this call.
  //
  void SetParameters( const double* param );

  void SetParameters( const float* param );

  /// The number of elements in the parameter vector.
  //
  /// See SetParameters(double[]).
  unsigned int GetNumberOfParameters() const;

  const double* GetParameters() const;


  /// Set the fixed parameters.
  //
  /// These are the grid spacing, the grid origin, etc.
  void SetFixedParameters( const double* param, unsigned N );

  /// These are the grid spacing, the grid origin, etc.
  void SetFixedParameters( const float* param, unsigned N );

  /// The number of fixed parameters.
  unsigned int GetNumberOfFixedParameters() const;

  /// Return a pointer to the fixed parameter array.
  //
  /// This is a pointer to internal data; the class still owns it.
  const double* GetFixedParameters() const;

  /// BulkTransform should be in the ITK coordinate system, which is LPS.
  void SetBulkTransform( const double linear[3][3], const double offset[3] );
  void GetBulkTransform( double linear[3][3], double offset[3] );
  BulkTransformType const* GetBulkTransform() const;

  /// Sets whether a LPS->RAS conversion should be done.
  //
  /// When the BSpline is created, it is assumed to be in an LPS
  /// coordinate system, as is typical for ITK BSplines.  If this
  /// switch is set to \c true, then this class will assume that the
  /// input and output points are in an RAS coordinate system, and will
  /// first convert them to LPS, call the ITK BSpline, and convert the
  /// result back to RAS.
  //
  /// By default, this switch is is FALSE.  Thus, by default, this
  /// class will behave exactly like the wrapped ITK BSpline.
  void SetSwitchCoordinateSystem( bool v );

  bool GetSwitchCoordinateSystem() const;

  itk::Transform<double,3,3>::Pointer GetITKTransform() const;

  /// copy underlying ITK transform
  void InternalDeepCopy(vtkAbstractTransform *abstractTransform);

protected:
  vtkITKBSplineTransform();
  virtual ~vtkITKBSplineTransform();

  void ForwardTransformPoint( const float in[3], float out[3] );
  void ForwardTransformPoint( const double in[3], double out[3] );

  void ForwardTransformDerivative( const float in[3], float out[3],
                                   float derivative[3][3] );
  void ForwardTransformDerivative( const double in[3], double out[3],
                                   double derivative[3][3] );

  void InverseTransformPoint( const float in[3], float out[3] );

  void InverseTransformPoint( const double in[3], double out[3] );

  void InverseTransformDerivative( const float in[3], float out[3],
                                   float derivative[3][3] );
  void InverseTransformDerivative( const double in[3], double out[3],
                                   double derivative[3][3] );

 public:
//private:
  vtkITKBSplineTransformHelper* Helper;
};

#endif
