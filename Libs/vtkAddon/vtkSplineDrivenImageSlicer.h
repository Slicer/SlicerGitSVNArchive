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

//! \class vtkSplineDrivenImageSlicer
//! \brief Reslicing of a volume along a path
//!
//! Straightened Curved Planar Reformation (Stretched-CPR) builds a 2D image
//! from an input path and an input volume. Each point of the path is
//! considered as the center of a 2D vtkImageReslicer. Reslicers axes are set
//! orthogonal to the path. Reslicers output are appended on the z axis. Thus
//! the output of this filter is a volume with central XZ- and YZ-slices
//! corresponding to the Straightened-CPR.
//!
//! Input: vtkImageData (InputConnection) and vtkPolyData (PathConnection)
//! one polyline representing the path. Typically, the output of vtkSpline can
//! be used as path input.
//!
//! \see Kanitsar et al. "CPR - Curved Planar Reformation", Proc. IEEE  Visualization, 2002, 37-44
//! \author Jerome Velut
//! \date 6 february 2011

#ifndef vtkSplineDrivenImageSlicer_h
#define vtkSplineDrivenImageSlicer_h

#include"vtkImageAlgorithm.h"

class vtkFrenetSerretFrame;
class vtkImageReslice;

// vtkAddon includes
#include "vtkAddon.h"

class VTK_ADDON_EXPORT vtkSplineDrivenImageSlicer : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkSplineDrivenImageSlicer,vtkImageAlgorithm);
  static vtkSplineDrivenImageSlicer* New();

  //! Specify the path represented by a vtkPolyData wich contains PolyLines
  void SetPathConnection(int id, vtkAlgorithmOutput* algOutput);
  void SetPathConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetPathConnection(0, algOutput);
  };
  vtkAlgorithmOutput* GetPathConnection( )
  {return( this->GetInputConnection( 1, 0 ) );};

  vtkSetVector2Macro( SliceExtent, int );
  vtkGetVector2Macro( SliceExtent, int );

  vtkSetVector2Macro( SliceSpacing, double );
  vtkGetVector2Macro( SliceSpacing, double );

  vtkSetMacro( SliceThickness, double );
  vtkGetMacro( SliceThickness, double );

  vtkSetMacro( OffsetPoint, vtkIdType );
  vtkGetMacro( OffsetPoint, vtkIdType );

  vtkSetMacro( OffsetLine, vtkIdType );
  vtkGetMacro( OffsetLine, vtkIdType );

  vtkSetMacro( ProbeInput, vtkIdType );
  vtkGetMacro( ProbeInput, vtkIdType );
  vtkBooleanMacro( ProbeInput, vtkIdType );

  vtkSetMacro( Incidence, double );
  vtkGetMacro( Incidence, double );


protected:
  vtkSplineDrivenImageSlicer();
  ~vtkSplineDrivenImageSlicer();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;

  virtual int FillInputPortInformation(int port, vtkInformation *info) override;
  virtual int FillOutputPortInformation( int, vtkInformation*) override;
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**,
                                 vtkInformationVector*) override;
private:
  vtkSplineDrivenImageSlicer(const vtkSplineDrivenImageSlicer&) = delete;
  void operator=(const vtkSplineDrivenImageSlicer&) = delete;

  vtkFrenetSerretFrame* localFrenetFrames; //!< computes local tangent along path input
  vtkImageReslice* reslicer; //!< Reslicers array

  int     SliceExtent[2]; //!< Number of pixels nx, ny in the slice space around the center points
  double SliceSpacing[2]; //!< Pixel size sx, sy of the output slice
  double SliceThickness; //!< Slice thickness (useful for volumic reconstruction)
  double Incidence; //!< Rotation of the initial normal vector.

  vtkIdType OffsetPoint; //!< Id of the point where the reslicer proceed
  vtkIdType OffsetLine; //!< Id of the line cell where to get the reslice center
  vtkIdType ProbeInput; //!< If true, the output plane (2nd output probes the input image)
};

#endif //__vtkSplineDrivenImageSlicer_h__
