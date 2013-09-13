/*=========================================================================

  Program:   Diffusion Applications
  Language:  C++
  Module:    $HeadURL$
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Brigham and Women's Hospital (BWH) All Rights Reserved.

  See License.txt or http://www.slicer.org/copyright/copyright.txt for details.

==========================================================================*/

// DWIRicianLMMSEFilter includes
#include "DWIRicianLMMSEFilterCLP.h"
#include "itkLMMSEVectorImageFilter.h"

// CLI includes
#include <itkPluginUtilities.h>

// ITK includes
#include <itkCastImageFilter.h>
#include <itkImageFileWriter.h>
#include <itkMetaDataObject.h>
#include <itkNrrdImageIO.h>

#define DIMENSION 3

template <class PixelType>
int DoIt( int argc, char * argv[], PixelType )
{
  PARSE_ARGS;

  // do the typedefs

  typedef itk::VectorImage<PixelType, DIMENSION> DiffusionImageType;

  typedef itk::Image<PixelType, DIMENSION> ScalarImageType;

  typedef double                                       PixelTypeDouble;
  typedef itk::VectorImage<PixelTypeDouble, DIMENSION> DoubleDiffusionImageType;

  typedef itk::Image<PixelTypeDouble, DIMENSION> ScalarDoubleImageType;

  typedef itk::CovariantVector<double, DIMENSION> CovariantVectorType;

  std::vector<CovariantVectorType> diffusionDirections;

  typedef itk::ImageFileReader<DiffusionImageType> FileReaderType;
  typename FileReaderType::Pointer reader = FileReaderType::New();
  reader->SetFileName( inputVolume.c_str() );
  reader->Update();

  itk::MetaDataDictionary            imgMetaDictionary = reader->GetMetaDataDictionary();
  std::vector<std::string>           imgMetaKeys = imgMetaDictionary.GetKeys();
  std::vector<std::string>::iterator itKey = imgMetaKeys.begin();
  std::string                        metaString;

  std::cout << "Number of keys = " << imgMetaKeys.size() << std::endl;

  typedef itk::MetaDataDictionary DictionaryType;
  const DictionaryType & dictionary = reader->GetMetaDataDictionary();

  typedef itk::MetaDataObject<std::string> MetaDataStringType;

  DictionaryType::ConstIterator itr = dictionary.Begin();
  DictionaryType::ConstIterator end = dictionary.End();

  double dBValue = 1000;

  while( itr != end )
    {
    itk::MetaDataObjectBase::Pointer entry = itr->second;
    MetaDataStringType::Pointer      entryvalue =
      dynamic_cast<MetaDataStringType *>( entry.GetPointer() );

    if( entryvalue )
      {
      ::size_t pos = itr->first.find("DWMRI_gradient");

      if( pos != std::string::npos )
        {
        std::string tagkey = itr->first;
        std::string tagvalue = entryvalue->GetMetaDataObjectValue();

        double dx[DIMENSION];
        std::sscanf(tagvalue.c_str(), "%lf %lf %lf\n", &dx[0], &dx[1], &dx[2]);
        diffusionDirections.push_back( (CovariantVectorType)(dx) );

        }
      else
        {

        // try to find the b-value

        ::size_t posB = itr->first.find("DWMRI_b-value");

        if( posB != std::string::npos )
          {
          std::string tagvalue = entryvalue->GetMetaDataObjectValue();
          std::sscanf(tagvalue.c_str(), "%lf\n", &dBValue );
          }
        else
          {
          // std::cout << itr->first << " " << entryvalue->GetMetaDataObjectValue() << std::endl;
          }
        }
      }

    ++itr;
    }

  // find the first zero baseline image and use it for the noise estimation

  ::size_t iNrOfDWIs = diffusionDirections.size();
  ::size_t iFirstBaseline = std::string::npos;
  for( ::size_t iI = 0; iI < iNrOfDWIs; iI++ )
    {

    if( diffusionDirections[iI].GetNorm() == 0 )
      {
      iFirstBaseline = iI;
      std::cout << "First baseline found at index = " << iFirstBaseline << std::endl;
      break;
      }

    }

  if( iFirstBaseline == std::string::npos )
    {

    std::cout << "Did not find an explicit baseline image." << std::endl;
    std::cout << "Treating the first volume as the baseline volume." << std::endl;
    iFirstBaseline = 0;

    }

  typename ScalarImageType::SizeType indexRadiusE;
  typename ScalarImageType::SizeType indexRadiusF;

  indexRadiusF[0] = iRadiusFiltering[0]; // radius along x
  indexRadiusF[1] = iRadiusFiltering[1]; // radius along y
  indexRadiusF[2] = iRadiusFiltering[2]; // radius along z

  indexRadiusE[0] = iRadiusEstimation[0]; // radius along x
  indexRadiusE[1] = iRadiusEstimation[1]; // radius along y
  indexRadiusE[2] = iRadiusEstimation[2]; // radius along z

  // now filter the diffusion weighted images with this noise level
  // filter volume, by volume

  // filter the whole thing

  typedef itk::LMMSEVectorImageFilter<DiffusionImageType, DoubleDiffusionImageType> RicianLMMSEImageFilterType;
  typename RicianLMMSEImageFilterType::Pointer ricianFilter = RicianLMMSEImageFilterType::New();
  ricianFilter->SetInput( reader->GetOutput() );
  ricianFilter->SetIterations( iIterations );
  ricianFilter->SetRadiusFiltering( indexRadiusF );
  ricianFilter->SetRadiusEstimation( indexRadiusE );
  ricianFilter->SetMinimumNumberOfUsedVoxelsEstimation( iMinimumNumberOfUsedVoxelsE );
  ricianFilter->SetMinimumNumberOfUsedVoxelsFiltering( iMinimumNumberOfUsedVoxelsF );
  ricianFilter->SetHistogramResolutionFactor( dResFact );
  ricianFilter->SetMinimumNoiseSTD( dMinSTD );
  ricianFilter->SetMaximumNoiseSTD( dMaxSTD );
  if( bUseAbsoluteValue )
    {
    ricianFilter->SetUseAbsoluteValue(true);
    }
  else
    {
    ricianFilter->SetUseAbsoluteValue(false);
    }
  ricianFilter->SetFirstBaseline( iFirstBaseline );
  ricianFilter->SetChannels( reader->GetOutput()->GetVectorLength() );
  ricianFilter->Update();

  std::cout << "number of components per pixel" << ricianFilter->GetOutput()->GetNumberOfComponentsPerPixel()
            << std::endl;

  // now cast it back to diffusionimagetype

  typedef itk::CastImageFilter<DoubleDiffusionImageType, DiffusionImageType> CastImageFilterType;

  typename CastImageFilterType::Pointer castImageFilter = CastImageFilterType::New();
  castImageFilter->SetInput( ricianFilter->GetOutput() );
  castImageFilter->Update();

  // let's write it out

  typename itk::NrrdImageIO::Pointer io = itk::NrrdImageIO::New();

  itk::MetaDataDictionary metaDataDictionary;
  metaDataDictionary = reader->GetMetaDataDictionary();

  io->SetFileTypeToBinary();
  io->SetMetaDataDictionary( metaDataDictionary );

  typedef itk::ImageFileWriter<DiffusionImageType> WriterType;
  typename WriterType::Pointer nrrdWriter = WriterType::New();
  nrrdWriter->UseInputMetaDataDictionaryOff();
  nrrdWriter->SetInput( castImageFilter->GetOutput() );
  nrrdWriter->SetImageIO(io);
  nrrdWriter->SetFileName( outputVolume.c_str() );
  if (compressOutput)
    {
    nrrdWriter->UseCompressionOn();
    }
  else
    {
    nrrdWriter->UseCompressionOff();
    }

  try
    {
    nrrdWriter->Update();
    }
  catch( itk::ExceptionObject e )
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << e << std::endl;
    return EXIT_FAILURE;
    }

  std::cout << "success = " << EXIT_SUCCESS << std::endl;

  return EXIT_SUCCESS;

}

int main( int argc, char * argv[] )
{

  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     pixelType;
  itk::ImageIOBase::IOComponentType componentType;

  // try
  // {
  itk::GetImageType(inputVolume, pixelType, componentType);

  // This filter handles all types

  switch( componentType )
    {
#ifndef WIN32
    case itk::ImageIOBase::UCHAR:
      return DoIt( argc, argv, static_cast<unsigned char>(0) );
      break;
    case itk::ImageIOBase::CHAR:
      return DoIt( argc, argv, static_cast<char>(0) );
      break;
#endif
    case itk::ImageIOBase::USHORT:
      return DoIt( argc, argv, static_cast<unsigned short>(0) );
      break;
    case itk::ImageIOBase::SHORT:
      return DoIt( argc, argv, static_cast<short>(0) );
      break;
    case itk::ImageIOBase::UINT:
      return DoIt( argc, argv, static_cast<unsigned int>(0) );
      break;
    case itk::ImageIOBase::INT:
      return DoIt( argc, argv, static_cast<int>(0) );
      break;
#ifndef WIN32
    case itk::ImageIOBase::ULONG:
      return DoIt( argc, argv, static_cast<unsigned long>(0) );
      break;
    case itk::ImageIOBase::LONG:
      return DoIt( argc, argv, static_cast<long>(0) );
      break;
#endif
    case itk::ImageIOBase::FLOAT:
      std::cout << "FLOAT type not currently supported." << std::endl;
      break;
    case itk::ImageIOBase::DOUBLE:
      std::cout << "DOUBLE type not currently supported." << std::endl;
      break;
    case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
    default:
      std::cout << "unknown component type" << std::endl;
      break;
    }

  // }

  /*catch( itk::ExceptionObject &excep)
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
    }*/

  return EXIT_SUCCESS;
}
