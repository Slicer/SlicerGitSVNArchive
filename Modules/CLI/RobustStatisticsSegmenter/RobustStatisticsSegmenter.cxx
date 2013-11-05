#include "RobustStatisticsSegmenterCLP.h"

#include <iostream>

#include "SFLSRobustStatSegmentor3DLabelMap.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"

// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace
{

template <typename InputLabelImageType>
bool
preprocessInputLabelImage(const InputLabelImageType* inputLabelImage, typename InputLabelImageType::PixelType labelOfInterest, \
                          typename InputLabelImageType::Pointer& labelImageOnlyContainsTarget, \
                          typename InputLabelImageType::Pointer& rejectionLabelImage)
{
  labelImageOnlyContainsTarget = InputLabelImageType::New();
  labelImageOnlyContainsTarget->SetRegions(inputLabelImage->GetLargestPossibleRegion());
  labelImageOnlyContainsTarget->Allocate();
  labelImageOnlyContainsTarget->FillBuffer(0);
  labelImageOnlyContainsTarget->CopyInformation(inputLabelImage);

  rejectionLabelImage = InputLabelImageType::New();
  rejectionLabelImage->SetRegions(inputLabelImage->GetLargestPossibleRegion());
  rejectionLabelImage->Allocate();
  rejectionLabelImage->FillBuffer(0);
  rejectionLabelImage->CopyInformation(inputLabelImage);

  typedef itk::ImageRegionConstIterator<InputLabelImageType> ImageRegionConstIteratorType;
  ImageRegionConstIteratorType cIt(inputLabelImage, inputLabelImage->GetLargestPossibleRegion());
  cIt.GoToBegin();

  typedef itk::ImageRegionIterator<InputLabelImageType> ImageRegionIteratorType;
  ImageRegionIteratorType itL(labelImageOnlyContainsTarget, labelImageOnlyContainsTarget->GetLargestPossibleRegion());
  itL.GoToBegin();

  typedef itk::ImageRegionIterator<InputLabelImageType> ImageRegionIteratorType;
  ImageRegionIteratorType itR(rejectionLabelImage, rejectionLabelImage->GetLargestPossibleRegion());
  itR.GoToBegin();

  bool useRejectionLabel = false;

  for (; !cIt.IsAtEnd(); ++cIt, ++itL, ++itR)
    {
      typename InputLabelImageType::PixelType l = cIt.Get();
      if (l == labelOfInterest)
        {
          itL.Set(1);
        }
      else if (l != 0)
        {
          // other labels, not including 0,  are for rejection. 0 is for un-known.
          useRejectionLabel = true;
          itR.Set(1);
        }
    }

  return useRejectionLabel;
}


template <typename TPixel>
itk::Image<short, 3>::Pointer
getFinalMask(typename itk::Image<TPixel, 3>::Pointer img, unsigned char l, TPixel thod)
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

void
postProcessFinalMask(itk::Image<short, 3>* finalMask, unsigned char l, const itk::Image<short, 3>* inputLabelImage)
{
  typedef itk::Image<short, 3> MaskType;

  MaskType::SizeType size = finalMask->GetLargestPossibleRegion().GetSize();

  long nx = size[0];
  long ny = size[1];
  long nz = size[2];

  for( long ix = 0; ix < nx; ++ix )
    {
    for( long iy = 0; iy < ny; ++iy )
      {
      for( long iz = 0; iz < nz; ++iz )
        {
          MaskType::IndexType idx = {{ix, iy, iz}};
          MaskType::PixelType v = inputLabelImage->GetPixel(idx);

          if (v != l && v != 0)
            {
              finalMask->SetPixel(idx, 0);
            }
        }
      }
    }

  return;
}

} // end of anonymous namespace



int main(int argc, char* * argv)
{
  PARSE_ARGS;

  typedef short                                         PixelType;
  typedef CSFLSRobustStatSegmentor3DLabelMap<PixelType> SFLSRobustStatSegmentor3DLabelMap_c;

  // read input image
  typedef SFLSRobustStatSegmentor3DLabelMap_c::TImage Image_t;

  typedef itk::ImageFileReader<Image_t> ImageReaderType;
  ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(originalImageFileName.c_str() );
  Image_t::Pointer img;

  try
    {
    reader->Update();
    img = reader->GetOutput();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    raise(SIGABRT);
    }

  // read input label image
  typedef SFLSRobustStatSegmentor3DLabelMap_c::TLabelImage LabelImage_t;

  typedef itk::ImageFileReader<LabelImage_t> LabelImageReader_t;
  LabelImageReader_t::Pointer readerLabel = LabelImageReader_t::New();
  readerLabel->SetFileName(labelImageFileName.c_str() );
  LabelImage_t::Pointer labelImg;

  try
    {
    readerLabel->Update();
    labelImg = readerLabel->GetOutput();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    raise(SIGABRT);
    }

  // preprocess label map (labelImg, the naming is confusing.....)
  LabelImage_t::Pointer labelImageOnlyContainsTarget;
  LabelImage_t::Pointer rejectionLabelImage;

  bool useRejctionLabelImage = preprocessInputLabelImage<LabelImage_t>(labelImg, labelValue, labelImageOnlyContainsTarget, rejectionLabelImage);

  if ( useRejctionLabelImage )
    {
    std::cout << "Using rejection mask." << std::endl;
    }

  // do seg
  SFLSRobustStatSegmentor3DLabelMap_c seg;
  seg.setImage(img);

  seg.setNumIter(10000); // a large enough number, s.t. will not be stoped by this creteria.
  seg.setMaxVolume(expectedVolume);
  seg.setInputLabelImage(labelImageOnlyContainsTarget);

  seg.setMaxRunningTime(maxRunningTime);

  seg.setIntensityHomogeneity(intensityHomogeneity);
  seg.setCurvatureWeight(curvatureWeight / 1.5);

  if ( useRejctionLabelImage )
    {
    seg.setRejectionMask( rejectionLabelImage );
    }


  seg.doSegmenation();

  typedef itk::Image<short, 3> MaskImageType;

  MaskImageType::Pointer finalMask = getFinalMask<float>(seg.mp_phi, labelValue, levelSetThreshold);
  finalMask->CopyInformation(img);

  if ( useRejctionLabelImage )
    {
      postProcessFinalMask(finalMask, labelValue, labelImg);
    }

  typedef itk::ImageFileWriter<MaskImageType> WriterType;
  WriterType::Pointer outputWriter = WriterType::New();
  outputWriter->SetFileName(segmentedImageFileName.c_str() );
  outputWriter->SetInput(finalMask);
  outputWriter->Update();

  try
    {
    outputWriter->Update();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cout << "ExceptionObject caught !" << std::endl;
    std::cout << err << std::endl;
    raise(SIGABRT);
    }

  return EXIT_SUCCESS;
}
