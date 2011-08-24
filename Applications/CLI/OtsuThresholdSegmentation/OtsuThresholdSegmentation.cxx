/*=========================================================================
  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: OtsuThresholdImageFilter.cxx,v $
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "itkOtsuThresholdImageFilter.h"
#include "itkConnectedComponentImageFilter.h"
#include "itkRelabelComponentImageFilter.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkPluginFilterWatcher.h"

#include "OtsuThresholdSegmentationCLP.h"

// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace
{

} // end of anonymous namespace

int main( int argc, char * argv[] )
{
  PARSE_ARGS;

// Image Types
  typedef  long InputPixelType;
  typedef  long InternalPixelType;
  typedef  int  OutputPixelType;

  typedef itk::Image<InputPixelType,  3>   InputImageType;
  typedef itk::Image<InternalPixelType, 3> InternalImageType;
  typedef itk::Image<OutputPixelType, 3>   OutputImageType;

// Filter Types
  typedef itk::OtsuThresholdImageFilter<
    InputImageType, InputImageType>  OtsuFilterType;
  typedef itk::ConnectedComponentImageFilter<
    InputImageType, InternalImageType>  CCFilterType;
  typedef itk::RelabelComponentImageFilter<
    InternalImageType, OutputImageType> RelabelType;

// I/O Types
  typedef itk::ImageFileReader<InputImageType>  ReaderType;
  typedef itk::ImageFileWriter<OutputImageType> WriterType;

// Instances
  ReaderType::Pointer     reader = ReaderType::New();
  OtsuFilterType::Pointer OtsuFilter = OtsuFilterType::New();
  CCFilterType::Pointer   CCFilter = CCFilterType::New();
  RelabelType::Pointer    RelabelFilter = RelabelType::New();
  WriterType::Pointer     writer = WriterType::New();

// Watchers
  itk::PluginFilterWatcher OtsuWatcher(OtsuFilter, "Otsu Threshold Image Filter", CLPProcessInformation, 1.0 / 3.0, 0.0);
  itk::PluginFilterWatcher CCWatcher(CCFilter, "Connected Component Threshold Image Filter", CLPProcessInformation,
                                     1.0 / 3.0,
                                     1.0 / 3.0);
  itk::PluginFilterWatcher RelabelWatcher(RelabelFilter, "Relabel objects", CLPProcessInformation, 1.0 / 3.0, 2.0 / 3.0);

  reader->SetFileName(inputVolume.c_str() );

  OtsuFilter->SetInput( reader->GetOutput() );

  if( brightObjects )
    {
    OtsuFilter->SetOutsideValue( 255 );
    OtsuFilter->SetInsideValue(  0  );
    }
  else
    {
    OtsuFilter->SetOutsideValue( 0 );
    OtsuFilter->SetInsideValue(  255  );
    }
  OtsuFilter->SetNumberOfHistogramBins( numberOfBins );

  CCFilter->SetInput(OtsuFilter->GetOutput() );
  if( faceConnected )
    {
    CCFilter->FullyConnectedOff();
    }
  else
    {
    CCFilter->FullyConnectedOn();
    }
  RelabelFilter->SetInput(CCFilter->GetOutput() );
  RelabelFilter->SetMinimumObjectSize(minimumObjectSize);

  writer->SetInput( RelabelFilter->GetOutput() );
  writer->SetFileName( outputVolume.c_str() );
  writer->SetUseCompression(1);
  writer->Update();

  int threshold = OtsuFilter->GetThreshold();
  std::cout << "Threshold = " << threshold << std::endl;

  RelabelFilter->Print(std::cout);

  return EXIT_SUCCESS;
}
