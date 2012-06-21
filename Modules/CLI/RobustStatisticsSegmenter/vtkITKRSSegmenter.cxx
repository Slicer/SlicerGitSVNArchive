/*=========================================================================

  Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   vtkITK
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/Module/CLI/RobustStatisticsSegmenter/vtkITKRSSegmenter.cxx $
  Date:      $Date: 2006-12-21 07:21:52 -0500 (Thu, 21 Dec 2006) $
  Version:   $Revision: 1900 $

==========================================================================*/

#include "vtkITKRSSegmenter.h"
#include "vtkObjectFactory.h"

#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkImageData.h"

#include "SFLSRobustStatSegmentor3DLabelMap_single.h"

vtkCxxRevisionMacro(vtkITKRSSegmenter, "$Revision: 1900 $");
vtkStandardNewMacro(vtkITKRSSegmenter);

vtkITKRSSegmenter::vtkITKRSSegmenter()
{
    this->IntensityHomogeneity = 0.8;
    this->Smoothness = 0.3;
    this->ExpectedVolume = 10.0;
    this->MaxRunningTime = 10.0;
}

vtkITKRSSegmenter::~vtkITKRSSegmenter()
{
}


//template <class T>
//void vtkITKRSSegmenterExecute(vtkITKRSSegmenter *self, vtkImageData* input,
//                vtkImageData* vtkNotUsed(output),
//                T* inPtr, T* inSeedPtr, T* outPtr)

void vtkITKRSSegmenterExecute(vtkITKRSSegmenter *self, vtkImageData* input, vtkImageData* vtkNotUsed(output), short* inPtr, char* inSeedPtr, char* outPtr)
{

  int dims[3];
  input->GetDimensions(dims);
  double spacing[3];
  input->GetSpacing(spacing);

  // Wrap scalars into an ITK image
  // - mostly rely on defaults for spacing, origin etc for this filter
  typedef short ImagePixelType;
  typedef char LabelImagePixelType;

  typedef itk::Image<ImagePixelType, 3> ImageType;
  typename ImageType::Pointer inImage = ImageType::New();
  inImage->GetPixelContainer()->SetImportPointer(inPtr, dims[0]*dims[1]*dims[2], false);


  typename ImageType::RegionType region;
  typename ImageType::IndexType index;
  typename ImageType::SizeType size;
  index[0] = index[1] = index[2] = 0;
  size[0] = dims[0];
  size[1] = dims[1];
  size[2] = dims[2];
  region.SetIndex(index);
  region.SetSize(size);

  inImage->SetLargestPossibleRegion(region);
  inImage->SetBufferedRegion(region);
  inImage->SetSpacing(spacing);

  typedef itk::Image<LabelImagePixelType, 3> LabelImageType;
  typename LabelImageType::Pointer seedImage = LabelImageType::New();
  seedImage->GetPixelContainer()->SetImportPointer(inSeedPtr, dims[0]*dims[1]*dims[2], false);

  seedImage->SetLargestPossibleRegion(region);
  seedImage->SetBufferedRegion(region);
  seedImage->SetSpacing(spacing);


  // Calculate the distance transform
  typedef CSFLSRobustStatSegmentor3DLabelMap<PixelType> SFLSRobustStatSegmentor3DLabelMap_c;

  SFLSRobustStatSegmentor3DLabelMap_c seg;
  seg.setImage(inImage);

  seg.setNumIter(10000); // a large enough number, s.t. will not be stoped by this creteria.
  seg.setMaxVolume(ExpectedVolume);
  seg.setInputLabelImage(seedImage);

  seg.setMaxRunningTime(MaxRunningTime);

  seg.setIntensityHomogeneity(IntensityHomogeneity);
  seg.setCurvatureWeight(Smoothness / 1.5);

  seg.doSegmenation();

  LabelImagePixelType labelValue = 1;
  LabelImageType::Pointer finalMask = getFinalMask<float>(seg.mp_phi, labelValue, 2.0);
  finalMask->CopyInformation(img);


  // Copy to the output
  memcpy(outPtr, finalMask->GetBufferPointer(),
         finalMask->GetBufferedRegion().GetNumberOfPixels() * sizeof(LabelImagePixelType));

}


template <typename TPixel>
itk::Image<short, 3>::Pointer
vtkITKRSSegmenter::getFinalMask(typename itk::Image<TPixel, 3>::Pointer img, unsigned char l, TPixel thod)
{
  typedef itk::Image<short, 3> MaskType;

  MaskType::SizeType size = img->GetLargestPossibleRegion().GetSize();

  long nx = size[0];
  long ny = size[1];
  long nz = size[2];

  MaskType::Pointer   mask = MaskType::New();
  MaskType::IndexType start = {{0, 0, 0}};

  MaskType::RegionType region;
  region.SetSize( size );
  region.SetIndex( start );

  mask->SetRegions( region );

  mask->SetSpacing(img->GetSpacing() );
  mask->SetOrigin(img->GetOrigin() );

  mask->Allocate();
  mask->FillBuffer(0);
  for( long ix = 0; ix < nx; ++ix )
    {
    for( long iy = 0; iy < ny; ++iy )
      {
      for( long iz = 0; iz < nz; ++iz )
        {
        MaskType::IndexType idx = {{ix, iy, iz}};
        TPixel              v = img->GetPixel(idx);

        mask->SetPixel(idx, v <= thod ? l : 0);
        }
      }
    }

  return mask;
}



//
//
//
void vtkITKRSSegmenter::SimpleExecute(vtkImageData* input, vtkImageData* inputSeed, vtkImageData* outputLabel)
{
  vtkDebugMacro(<< "Executing RSS");
  pd=input->GetPointData();
  if (pd ==NULL)
    {
    vtkErrorMacro(<<"PointData is NULL");
    return;
    }
  vtkDataArray *inScalars=pd->GetScalars();
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for RSS");
    return;
    }


  //
  // cast the input image scalar type to short
  //
  vtkSmartPointer<vtkImageCast> castFilter = vtkSmartPointer<vtkImageCast>::New();
  castFilter->SetInputConnection(input);
  castFilter->SetOutputScalarTypeToShort();
  castFilter->Update();

  vtkSmartPointer<vtkImageCast> inputShort = castFilter->GetOutput();


  //
  // cast the input seed label image scalar type to char
  //
  vtkSmartPointer<vtkImageCast> castFilter1 = vtkSmartPointer<vtkImageCast>::New();
  castFilter1->SetInputConnection(inputSeed);
  castFilter1->SetOutputScalarTypeToChar();
  castFilter1->Update();

  vtkSmartPointer<vtkImageCast> inputSeedChar = castFilter1->GetOutput();

  //
  // Initialize and check input
  //
  vtkPointData *pd = inputShort->GetPointData();


  /// TODO: need to check the sizes of input and inputSeed match? (it is checked in the RSS code)


  if (inScalars->GetNumberOfComponents() == 1 )
    {

////////// These types are not defined in itk ////////////
#undef VTK_TYPE_USE_LONG_LONG
#undef VTK_TYPE_USE___INT64

//#define CALL  vtkITKRSSegmenterExecute(this, input, output, static_cast<VTK_TT *>(inPtr), static_cast<VTK_TT *>(inSeedPtr), static_cast<VTK_TT *>(outPtr));

    void* inPtr = inputShort->GetScalarPointer();
    void* inSeedPtr = inputSeedChar->GetScalarPointer();
    void* outPtr = output->GetScalarPointer();


    vtkITKRSSegmenterExecute(this, inputShort, output, static_cast<short *>(inPtr), static_cast<char *>(inSeedPtr), static_cast<char *>(outPtr));

//    switch (inScalars->GetDataType())
//      {
//      vtkTemplateMacroCase(VTK_DOUBLE, double, CALL);                           \
//      vtkTemplateMacroCase(VTK_FLOAT, float, CALL);                             \
//      vtkTemplateMacroCase(VTK_LONG, long, CALL);                               \
//      vtkTemplateMacroCase(VTK_UNSIGNED_LONG, unsigned long, CALL);             \
//      vtkTemplateMacroCase(VTK_INT, int, CALL);                                 \
//      vtkTemplateMacroCase(VTK_UNSIGNED_INT, unsigned int, CALL);               \
//      vtkTemplateMacroCase(VTK_SHORT, short, CALL);                             \
//      vtkTemplateMacroCase(VTK_UNSIGNED_SHORT, unsigned short, CALL);           \
//      vtkTemplateMacroCase(VTK_CHAR, char, CALL);                               \
//      vtkTemplateMacroCase(VTK_SIGNED_CHAR, signed char, CALL);                 \
//      vtkTemplateMacroCase(VTK_UNSIGNED_CHAR, unsigned char, CALL);
//      } //switch
    }
  else
    {
    vtkErrorMacro(<< "Can only calculate on scalar.");
    }
}

void vtkITKRSSegmenter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "BackgroundValue: " << BackgroundValue << std::endl;
  os << indent << "InsideIsPositive: " << InsideIsPositive << std::endl;
  os << indent << "UseImageSpacing: " << UseImageSpacing << std::endl;
  os << indent << "SquaredDistance: " << SquaredDistance << std::endl;
}



