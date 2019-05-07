// Copyright (c) 2010, Jerome Velut
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT OWNER ``AS IS'' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
// NO EVENT SHALL THE COPYRIGHT OWNER BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//! \class vtkFrenetSerretFrame
//! \brief Compute tangent and normal vectors to a polyline
//!
//! Given a polyline as input, this filter computes the Frenet-Serret frame
//! at each point. The output contains the tangent and normal vectors to the
//! curve. These vectors are appended, so that input array are not overwrited
//! \see vtkImagePathReslice for a use-case.
//!
//! \todo Comment this class. (cxx)
//! \todo [ENH] compute the whole chart (B=N^T) and put it in the
//! PointData as a tensor.
//!
//! \author Jerome Velut
//! \date 21 jan 2010

#ifndef vtkFrenetSerretFrame_h
#define vtkFrenetSerretFrame_h

// vtkAddon includes
#include "vtkAddon.h"

#include "vtkPolyDataAlgorithm.h"

class VTK_ADDON_EXPORT vtkFrenetSerretFrame : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkFrenetSerretFrame,vtkPolyDataAlgorithm);
  static vtkFrenetSerretFrame* New();

  //! Set ConsistentNormals to 1 if you want your frames to be 'smooth'.
  //! Note that in this case, the normal to the curve will not represent the
  //! acceleration, ie this is no more Frenet-Serret chart.
  vtkBooleanMacro( ConsistentNormals, int );
  vtkSetMacro( ConsistentNormals, int );
  vtkGetMacro( ConsistentNormals, int );

  //! If yes, computes the cross product between Tangent and Normal to get
  //! the binormal vector.
  vtkBooleanMacro( ComputeBinormal, int );
  vtkSetMacro( ComputeBinormal, int );
  vtkGetMacro( ComputeBinormal, int );

  //! Define the inclination of the consistent normals. Radian value.
  vtkSetMacro( ViewUp, double );
  vtkGetMacro( ViewUp, double );

  //! Rotate a vector around an axis
  //! \param [in] axis {Vector defining the axis to turn around.}
  //! \param [in] angle {Rotation angle in radian.}
  //! \param [out] vector {Vector to rotate. In place modification.}
  static void RotateVector( double* vector, const double* axis, double angle );


protected:
  vtkFrenetSerretFrame();
  ~vtkFrenetSerretFrame();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  virtual int FillInputPortInformation(int port, vtkInformation *info) override;

  //! Computes the derivative between 2 points (Next - Last).
  //! \param [in] pointIdNext {give first point Id}
  //! \param [in] pointIdLast {give second point Id}
  //! \param [out] tangent {fill a 3-array with the derivative value}
  //! \note If Next is [i+1], Last is [i-1], your are computing the
  //! centered tangent at [i].
  void ComputeTangentVectors( vtkIdType pointIdNext,
                              vtkIdType pointIdLast,
                              double* tangent );

  //! Computes the second derivative between 2 points (Next - Last).
  //! \param [in] nextTg {give a first derivative}
  //! \param [in] lastTg {give a first derivative}
  //! \param [out] normal {fill a 3-array with the second derivative value}
  void ComputeNormalVectors( double *tgNext,
                             double *tgLast,
                             double* normal );

  //! ConsistentNormal depends on the local tangent and the last computed
  //! normal. This is a projection of lastNormal on the plan defined
  //! by tangent.
  //! \param [in] tangent {give the tangent}
  //! \param [in] lastNormal {give the reference normal}
  //! \param [out] normal {fill a 3-array with the normal vector}
  void ComputeConsistentNormalVectors( double *tangent,
                                       double *lastNormal,
                                       double* normal );
private:
  vtkFrenetSerretFrame(const vtkFrenetSerretFrame&) = delete;
  void operator=(const vtkFrenetSerretFrame&) = delete;

  int ComputeBinormal; //!< If 1, a Binormal array is added to the output
  int ConsistentNormals; //!< Boolean. If 1, successive normals are computed
  //!< in smooth manner.
  //!< \see ComputeConsistentNormalVectors
  double ViewUp; //!< Define the inclination of the normal vectors in case of
  //!< ConsistentNormals is On
};

#endif //__VTKFRENETSERRETFRAME_H__
