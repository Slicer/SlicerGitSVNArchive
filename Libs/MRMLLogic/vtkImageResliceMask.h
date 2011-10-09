/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImageResliceMask.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
///  vtkImageResliceMask - Reslices a volume along a new set of axes.
/// 
/// Changes: 6.2007:
/// vtkImageResliceMask is a modified version of vtkImageReslice 
/// that provides an additional output mask to distinguish the 
/// area outside the original dataset where no information is available
/// Original documentation:
/// vtkImageReslice is the swiss-army-knife of image geometry filters:  
/// It can permute, rotate, flip, scale, resample, deform, and pad image
/// data in any combination with reasonably high efficiency.  Simple
/// operations such as permutation, resampling and padding are done
/// with similar efficiently to the specialized vtkImagePermute,
/// vtkImageResample, and vtkImagePad filters.  There are a number of
/// tasks that vtkImageReslice is well suited for:
/// <p>1) Application of simple rotations, scales, and translations to
/// an image. It is often a good idea to use vtkImageChangeInformation
/// to center the image first, so that scales and rotations occur around
/// the center rather than around the lower-left corner of the image.
/// <p>2) Resampling of one data set to match the voxel sampling of 
/// a second data set via the SetInformationInput() method, e.g. for
/// the purpose of comparing two images or combining two images.
/// A transformation, either linear or nonlinear, can be applied 
/// at the same time via the SetResliceTransform method if the two
/// images are not in the same coordinate space.
/// <p>3) Extraction of slices from an image volume.  The most convenient
/// way to do this is to use SetResliceAxesDirectionCosines() to
/// specify the orientation of the slice.  The direction cosines give
/// the x, y, and z axes for the output volume.  The method 
/// SetOutputDimensionality(2) is used to specify that want to output a
/// slice rather than a volume.  The SetResliceAxesOrigin() command is
/// used to provide an (x,y,z) point that the slice will pass through.
/// You can use both the ResliceAxes and the ResliceTransform at the
/// same time, in order to extract slices from a volume that you have
/// applied a transformation to.
/// .SECTION Caveats
/// This filter is very inefficient if the output X dimension is 1.
/// .SECTION see also
/// vtkAbstractTransform vtkMatrix4x4


#ifndef __vtkImageResliceMask_h
#define __vtkImageResliceMask_h

#include "vtkMRMLLogicWin32Header.h"

// VTK includes
#include <vtkImageReslice.h> // for VTK_RESLICE_NEAREST, LINEAR, CUBIC
#include <vtkThreadedImageAlgorithm.h>
class vtkAbstractTransform;
class vtkImageData;
class vtkImageStencilData;
class vtkMatrix4x4;

class VTK_MRML_LOGIC_EXPORT vtkImageResliceMask : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageResliceMask *New();
  vtkTypeRevisionMacro(vtkImageResliceMask, vtkThreadedImageAlgorithm);

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  /// 
  /// This method is used to set up the axes for the output voxels.
  /// The output Spacing, Origin, and Extent specify the locations
  /// of the voxels within the coordinate system defined by the axes.
  /// The ResliceAxes are used most often to permute the data, e.g.
  /// to extract ZY or XZ slices of a volume as 2D XY images.
  /// <p>The first column of the matrix specifies the x-axis 
  /// vector (the fourth element must be set to zero), the second
  /// column specifies the y-axis, and the third column the
  /// z-axis.  The fourth column is the origin of the
  /// axes (the fourth element must be set to one).  
  /// <p>An alternative to SetResliceAxes() is to use 
  /// SetResliceAxesDirectionCosines() to set the directions of the
  /// axes and SetResliceAxesOrigin() to set the origin of the axes.
  virtual void SetResliceAxes(vtkMatrix4x4*);
  vtkGetObjectMacro(ResliceAxes, vtkMatrix4x4);

  /// 
  /// Specify the direction cosines for the ResliceAxes (i.e. the
  /// first three elements of each of the first three columns of 
  /// the ResliceAxes matrix).  This will modify the current
  /// ResliceAxes matrix, or create a new matrix if none exists.
  void SetResliceAxesDirectionCosines(double x0, double x1, double x2,
                                      double y0, double y1, double y2,
                                      double z0, double z1, double z2);
  void SetResliceAxesDirectionCosines(const double x[3],
                                      const double y[3],
                                      const double z[3]) {
    this->SetResliceAxesDirectionCosines(x[0], x[1], x[2],
                                         y[0], y[1], y[2],
                                         z[0], z[1], z[2]); };
  void SetResliceAxesDirectionCosines(const double xyz[9]) {
    this->SetResliceAxesDirectionCosines(xyz[0], xyz[1], xyz[2],
                                         xyz[3], xyz[4], xyz[5],
                                         xyz[6], xyz[7], xyz[8]); };
  void GetResliceAxesDirectionCosines(double x[3], double y[3], double z[3]);
  void GetResliceAxesDirectionCosines(double xyz[9]) {
    this->GetResliceAxesDirectionCosines(&xyz[0], &xyz[3], &xyz[6]); };
  double *GetResliceAxesDirectionCosines() {
    this->GetResliceAxesDirectionCosines(this->ResliceAxesDirectionCosines);
    return this->ResliceAxesDirectionCosines; };

  /// 
  /// Specify the origin for the ResliceAxes (i.e. the first three
  /// elements of the final column of the ResliceAxes matrix).
  /// This will modify the current ResliceAxes matrix, or create
  /// new matrix if none exists.
  void SetResliceAxesOrigin(double x, double y, double z);
  void SetResliceAxesOrigin(const double xyz[3]) {
    this->SetResliceAxesOrigin(xyz[0], xyz[1], xyz[2]); };
  void GetResliceAxesOrigin(double xyz[3]);
  double *GetResliceAxesOrigin() {
    this->GetResliceAxesOrigin(this->ResliceAxesOrigin);
    return this->ResliceAxesOrigin; };

  /// 
  /// Set a transform to be applied to the resampling grid that has
  /// been defined via the ResliceAxes and the output Origin, Spacing
  /// and Extent.  Note that applying a transform to the resampling
  /// grid (which lies in the output coordinate system) is
  /// equivalent to applying the inverse of that transform to
  /// the input volume.  Nonlinear transforms such as vtkGridTransform
  /// and vtkThinPlateSplineTransform can be used here.
  virtual void SetResliceTransform(vtkAbstractTransform*);
  vtkGetObjectMacro(ResliceTransform, vtkAbstractTransform);

  /// 
  /// Set a vtkImageData from which the default Spacing, Origin,
  /// and WholeExtent of the output will be copied.  The spacing,
  /// origin, and extent will be permuted according to the 
  /// ResliceAxes.  Any values set via SetOutputSpacing, 
  /// SetOutputOrigin, and SetOutputExtent will override these
  /// values.  By default, the Spacing, Origin, and WholeExtent
  /// of the Input are used.
  virtual void SetInformationInput(vtkImageData*);
  vtkGetObjectMacro(InformationInput, vtkImageData);

  /// 
  /// Specify whether to transform the spacing, origin and extent
  /// of the Input (or the InformationInput) according to the
  /// direction cosines and origin of the ResliceAxes before applying
  /// them as the default output spacing, origin and extent 
  /// (default: On).
  vtkSetMacro(TransformInputSampling, int);
  vtkBooleanMacro(TransformInputSampling, int);
  vtkGetMacro(TransformInputSampling, int);

  /// 
  /// Turn this on if you want to guarantee that the extent of the
  /// output will be large enough to ensure that none of the 
  /// data will be cropped (default: Off).
  vtkSetMacro(AutoCropOutput, int);
  vtkBooleanMacro(AutoCropOutput, int);
  vtkGetMacro(AutoCropOutput, int);

  /// 
  /// Turn on wrap-pad feature (default: Off).
  vtkSetMacro(Wrap, int);
  vtkGetMacro(Wrap, int);
  vtkBooleanMacro(Wrap, int);

  /// 
  /// Turn on mirror-pad feature (default: Off).
  /// This will override the wrap-pad. 
  vtkSetMacro(Mirror, int);
  vtkGetMacro(Mirror, int);
  vtkBooleanMacro(Mirror, int);

  /// 
  /// Extend the apparent input border by a half voxel (default: On).
  /// This changes how interpolation is handled at the borders of the
  /// input image: if the center of an output voxel is beyond the edge
  /// of the input image, but is within a half voxel width of the edge
  /// (using the input voxel width), then the value of the output voxel
  /// is calculated as if the input's edge voxels were duplicated past
  /// the edges of the input.
  /// This has no effect if Mirror or Wrap are on.
  vtkSetMacro(Border, int);
  vtkGetMacro(Border, int);
  vtkBooleanMacro(Border, int);

  /// 
  /// Set interpolation mode (default: nearest neighbor). 
  vtkSetMacro(InterpolationMode, int);
  vtkGetMacro(InterpolationMode, int);
  void SetInterpolationModeToNearestNeighbor() {
    this->SetInterpolationMode(VTK_RESLICE_NEAREST); };
  void SetInterpolationModeToLinear() {
    this->SetInterpolationMode(VTK_RESLICE_LINEAR); };
  void SetInterpolationModeToCubic() {
    this->SetInterpolationMode(VTK_RESLICE_CUBIC); };
  const char *GetInterpolationModeAsString();

  /// 
  /// Turn on and off optimizations (default on, they should only be
  /// turned off for testing purposes). 
  vtkSetMacro(Optimization, int);
  vtkGetMacro(Optimization, int);
  vtkBooleanMacro(Optimization, int);

  /// 
  /// Set the background color (for multi-component images).
  vtkSetVector4Macro(BackgroundColor, double);
  vtkGetVector4Macro(BackgroundColor, double);

  /// 
  /// Set background grey level (for single-component images).
  void SetBackgroundLevel(double v) { this->SetBackgroundColor(v,v,v,v); };
  double GetBackgroundLevel() { return this->GetBackgroundColor()[0]; };

  /// 
  /// Set the voxel spacing for the output data.  The default output
  /// spacing is the input spacing permuted through the ResliceAxes.
  vtkSetVector3Macro(OutputSpacing, double);
  vtkGetVector3Macro(OutputSpacing, double);
  void SetOutputSpacingToDefault() {
    this->SetOutputSpacing(VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX); };

  /// 
  /// Set the origin for the output data.  The default output origin
  /// is the input origin permuted through the ResliceAxes.
  vtkSetVector3Macro(OutputOrigin, double);
  vtkGetVector3Macro(OutputOrigin, double);
  void SetOutputOriginToDefault() {
    this->SetOutputOrigin(VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX); };

  /// 
  /// Set the extent for the output data.  The default output extent
  /// is the input extent permuted through the ResliceAxes.
  vtkSetVector6Macro(OutputExtent, int);
  vtkGetVector6Macro(OutputExtent, int);
  void SetOutputExtentToDefault() {
    this->SetOutputExtent(VTK_INT_MIN, VTK_INT_MAX,
                          VTK_INT_MIN, VTK_INT_MAX,
                          VTK_INT_MIN, VTK_INT_MAX); };

  /// 
  /// Force the dimensionality of the output to either 1, 2,
  /// 3 or 0 (default: 3).  If the dimensionality is 2D, then
  /// the Z extent of the output is forced to (0,0) and the Z
  /// origin of the output is forced to 0.0 (i.e. the output
  /// extent is confined to the xy plane).  If the dimensionality
  /// is 1D, the output extent is confined to the x axis.  
  /// For 0D, the output extent consists of a single voxel at 
  /// (0,0,0).
  vtkSetMacro(OutputDimensionality, int);
  vtkGetMacro(OutputDimensionality, int);

  /// 
  /// When determining the modified time of the filter, 
  /// this check the modified time of the transform and matrix.
  unsigned long int GetMTime();

  /// 
  /// Convenient methods for switching between nearest-neighbor and linear
  /// interpolation.  
  /// InterpolateOn() is equivalent to SetInterpolationModeToLinear() and
  /// InterpolateOff() is equivalent to SetInterpolationModeToNearestNeighbor().
  /// You should not use these methods if you use the SetInterpolationMode
  /// methods.
  void SetInterpolate(int t) {
    if (t && !this->GetInterpolate()) {
      this->SetInterpolationModeToLinear(); }
    else if (!t && this->GetInterpolate()) {
      this->SetInterpolationModeToNearestNeighbor(); } };
  void InterpolateOn() {
    this->SetInterpolate(1); };
  void InterpolateOff() {
    this->SetInterpolate(0); };
  int GetInterpolate() {
    return (this->GetInterpolationMode() != VTK_RESLICE_NEAREST); };

  /// 
  /// Use a stencil to limit the calculations to a specific region of
  /// the output.  Portions of the output that are 'outside' the stencil
  /// will be cleared to the background color.  
  void SetStencil(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  vtkImageData *GetBackgroundMask();
protected:
  vtkImageResliceMask();
  ~vtkImageResliceMask();

  vtkMatrix4x4 *ResliceAxes;
  double ResliceAxesDirectionCosines[9];
  double ResliceAxesOrigin[3];
  vtkAbstractTransform *ResliceTransform;
  vtkImageData *InformationInput;
  int Wrap;
  int Mirror;
  int Border;
  int InterpolationMode;
  int Optimization;
  double BackgroundColor[4];
  double OutputOrigin[3];
  double OutputSpacing[3];
  int OutputExtent[6];
  int OutputDimensionality;
  int TransformInputSampling;
  int AutoCropOutput;
  int HitInputExtent;

  vtkMatrix4x4 *IndexMatrix;
  vtkAbstractTransform *OptimizedTransform;

  //vtkImageData *BackgroundMask;

  void GetAutoCroppedOutputBounds(vtkInformation *inInfo, double bounds[6]);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  virtual void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData, int ext[6], int id);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkMatrix4x4 *GetIndexMatrix(vtkInformation *inInfo,
                               vtkInformation *outInfo);
  vtkAbstractTransform *GetOptimizedTransform() { 
    return this->OptimizedTransform; };

private:
  vtkImageResliceMask(const vtkImageResliceMask&);  /// Not implemented.
  void operator=(const vtkImageResliceMask&);  /// Not implemented.
};

//----------------------------------------------------------------------------
inline const char *vtkImageResliceMask::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      return "NearestNeighbor";
    case VTK_RESLICE_LINEAR:
      return "Linear";
    case VTK_RESLICE_CUBIC:
      return "Cubic";
    default:
      return "";
    }
}  
//void SetBackgroundMask(bool *&BackgroundMaskPositionPtr, const int NumberOfPixels, const bool flag);
#endif





