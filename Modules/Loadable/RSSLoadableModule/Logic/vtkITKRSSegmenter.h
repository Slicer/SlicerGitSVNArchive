/*=========================================================================

  Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   vtkITK
  Module:    $HeadURL$
  Date:      $Date$
  Version:   $Revision$

==========================================================================*/

///  Wrapper class around CSFLSRobustStatSegmentor3DLabelMap
///

#ifndef __vtkITKRSSegmenter_h
#define __vtkITKRSSegmenter_h

#include "vtkITK.h"
#include "vtkSimpleImageToImageFilter.h"
#include "SFLSRobustStatSegmentor3DLabelMap_single.h"

class VTK_ITK_EXPORT vtkITKRSSegmenter : public vtkSimpleImageToImageFilter
{
 public:
  static vtkITKRSSegmenter *New();
  vtkTypeRevisionMacro(vtkITKRSSegmenter, vtkObject);

  /// If the target has homogeneous intensity
  vtkSetMacro(IntensityHomogeneity, double);
  vtkGetMacro(IntensityHomogeneity, double);

  /// A large number (max 1.0) indicates that the target boundary is smooth, small (min 0.0) for rough/jaggy boundary
  vtkSetMacro(Smoothness, double);
  vtkGetMacro(Smoothness, double);

  /// An upper limit of volume for the target object.
  vtkSetMacro(ExpectedVolume, double);
  vtkGetMacro(ExpectedVolume, double);

  /// Largest running time, over this, the algorithm is stopped and the result is output
  vtkSetMacro(MaxRunningTime, double);
  vtkGetMacro(MaxRunningTime, double);

//  ///
//  /// The seed image, a label image
//  vtkSetObjectMacro(SeedLabelImage, vtkImageData);
//  vtkGetObjectMacro(SeedLabelImage, vtkImageData);

//  ///
//  /// The image to be segmented
//  vtkSetObjectMacro(InputImage, vtkImageData);
//  vtkGetObjectMacro(InputImage, vtkImageData);


protected:
  vtkITKRSSegmenter();
  ~vtkITKRSSegmenter();

  virtual void SimpleExecute(vtkImageData* input, vtkImageData* inputSeed, vtkImageData* outputLabel);
  virtual void SimpleExecute(vtkImageData* input, vtkImageData* output) {} // TODO: This is simpply WRONG!!! The base class SimpleImageFilter only takes one input image. So it does not fit the RSS, which needs both grayscale image and seed label image.


  double IntensityHomogeneity;
  double Smoothness;
  double ExpectedVolume;
  double MaxRunningTime;

//  vtkImageData* InputImage;
//  vtkImageData* SeedLabelImage;


//  typedef CSFLSRobustStatSegmentor3DLabelMap<Superclass::InputImageType> ImageFilterType;
//  vtkITKRSSegmenter() : Superclass ( ImageFilterType::New() ){};
//  ~vtkITKRSSegmenter() {};
//  ImageFilterType* GetImageFilterPointer() { return dynamic_cast<ImageFilterType*> ( m_Filter.GetPointer() ); }


private:
  vtkITKRSSegmenter(const vtkITKRSSegmenter&);  /// Not implemented.
  void operator=(const vtkITKRSSegmenter&);  /// Not implemented.
};


#endif // __vtkITKRSSegmenter_h
