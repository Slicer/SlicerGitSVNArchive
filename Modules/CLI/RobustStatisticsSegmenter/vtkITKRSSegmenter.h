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


#include "vtkITKImageToImageFilterFF.h"
#include "SFLSRobustStatSegmentor3DLabelMap_single.h"

class VTK_ITK_EXPORT vtkITKRSSegmenter : public vtkObject
{
 public:
  static vtkITKRSSegmenter *New();
  vtkTypeRevisionMacro(vtkITKRSSegmenter, vtkObject);







  vtkSetMacro(IntensityHomogeneity, double);
  vtkGetMacro(IntensityHomogeneity, double);

  ///
  /// The mask image: used instead of brush if non NULL
  /// - image corresponds to the PaintRegion but is
  ///   in World coordinates.
  vtkSetObjectMacro(MaskImage, vtkImageData);
  vtkGetObjectMacro(MaskImage, vtkImageData);

  ///
  /// The reference image for threshold calculations
  vtkSetObjectMacro(BackgroundImage, vtkImageData);
  vtkGetObjectMacro(BackgroundImage, vtkImageData);

  ///
  /// Image data to be painted into
  vtkSetObjectMacro(WorkingImage, vtkImageData);
  vtkGetObjectMacro(WorkingImage, vtkImageData);







protected:
  typedef CSFLSRobustStatSegmentor3DLabelMap<Superclass::InputImageType> ImageFilterType;
  vtkITKRSSegmenter() : Superclass ( ImageFilterType::New() ){};
  ~vtkITKRSSegmenter() {};
  ImageFilterType* GetImageFilterPointer() { return dynamic_cast<ImageFilterType*> ( m_Filter.GetPointer() ); }


private:
  vtkITKRSSegmenter(const vtkITKRSSegmenter&);  /// Not implemented.
  void operator=(const vtkITKRSSegmenter&);  /// Not implemented.
};

#endif









#endif // __vtkITKRSSegmenter_h
