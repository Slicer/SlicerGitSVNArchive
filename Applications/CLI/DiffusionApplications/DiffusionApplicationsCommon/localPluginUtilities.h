#ifndef __localPluginUtilities_h
#define __localPluginUtilities_h

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "localPluginFilterWatcher.h"
#include <vector>
#include <string>

namespace itk
{
// Description:
// Get the PixelType and ComponentType from fileName
void GetImageType(std::string fileName,
                  ImageIOBase::IOPixelType & pixelType,
                  ImageIOBase::IOComponentType & componentType)
{
  typedef itk::Image<unsigned char, 3> ImageType;
  itk::ImageFileReader<ImageType>::Pointer imageReader =
    itk::ImageFileReader<ImageType>::New();
  imageReader->SetFileName(fileName.c_str() );
  imageReader->UpdateOutputInformation();

  pixelType = imageReader->GetImageIO()->GetPixelType();
  componentType = imageReader->GetImageIO()->GetComponentType();
}

// Description:
// Get the PixelTypes and ComponentTypes from fileNames
void GetImageTypes(std::vector<std::string> fileNames,
                   std::vector<ImageIOBase::IOPixelType> & pixelTypes,
                   std::vector<ImageIOBase::IOComponentType> & componentTypes)
{
  pixelTypes.clear();
  componentTypes.clear();
  // For each file, find the pixel and component type
  for( std::vector<std::string>::size_type i = 0; i < fileNames.size(); i++ )
    {
    ImageIOBase::IOPixelType     pixelType;
    ImageIOBase::IOComponentType componentType;

    GetImageType(fileNames[i],
                 pixelType,
                 componentType);
    pixelTypes.push_back(pixelType);
    componentTypes.push_back(componentType);
    }
}

#include "itkContinuousIndex.h"
template <class T>
void AlignVolumeCenters(T *fixed, T *moving, typename T::PointType & origin)
{
  // compute the center of fixed
  typename T::PointType fixedCenter;
    {
    itk::ContinuousIndex<double, T::ImageDimension> centerIndex;
    typename T::SizeType size = fixed->GetLargestPossibleRegion().GetSize();
    for( unsigned int i = 0; i < T::ImageDimension; i++ )
      {
      centerIndex[i] = static_cast<double>( (size[i] - 1) / 2.0);
      }
    fixed->TransformContinuousIndexToPhysicalPoint(centerIndex, fixedCenter);
    }

  // compute the center of moving
  typename T::PointType movingCenter;
    {
    itk::ContinuousIndex<double, T::ImageDimension> centerIndex;
    typename T::SizeType size = moving->GetLargestPossibleRegion().GetSize();
    for( unsigned i = 0; i < T::ImageDimension; i++ )
      {
      centerIndex[i] = static_cast<double>( (size[i] - 1) / 2.0);
      }
    moving->TransformContinuousIndexToPhysicalPoint(centerIndex, movingCenter);
    }
  for( unsigned int j = 0; j < fixedCenter.Size(); j++ )
    {
    origin[j] = moving->GetOrigin()[j] - (movingCenter[j] - fixedCenter[j]);
    }
}

} // end namespace itk

#endif
