/*=========================================================================

  Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   vtkITK
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Libs/vtkITK/vtkITKArchetypeDiffusionTensorImageReaderFile.h $
  Date:      $Date: 2007-01-19 13:21:56 -0500 (Fri, 19 Jan 2007) $
  Version:   $Revision: 2267 $

==========================================================================*/

#ifndef __vtkITKArchetypeDiffusionTensorImageReaderFile_h
#define __vtkITKArchetypeDiffusionTensorImageReaderFile_h

#include "vtkITKArchetypeImageSeriesReader.h"
#include <vtk/Common/Core/vtkVersion.h>

#include "itk/Modules/IO/ImageBase/include/itkImageFileReader.h"

class VTK_ITK_EXPORT vtkITKArchetypeDiffusionTensorImageReaderFile
  : public vtkITKArchetypeImageSeriesReader
{
 public:
  static vtkITKArchetypeDiffusionTensorImageReaderFile *New();
  vtkTypeMacro(vtkITKArchetypeDiffusionTensorImageReaderFile,vtkITKArchetypeImageSeriesReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

 protected:
  vtkITKArchetypeDiffusionTensorImageReaderFile();
  ~vtkITKArchetypeDiffusionTensorImageReaderFile();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  static void ReadProgressCallback(itk::ProcessObject* obj,const itk::ProgressEvent&, void* data);
  /// private:
};

#endif
