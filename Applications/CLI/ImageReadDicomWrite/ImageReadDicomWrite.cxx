/*=========================================================================

This plugin is based on the Insight Toolkit example
ImageReadDicomWrite. It has been modified for GenerateCLP style
command line processing and additional features have been added.

=========================================================================*/
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkExtractImageFilter.h"
#include "itkShiftScaleImageFilter.h"
#include "itkGDCMImageIO.h"
#include "itkMetaDataObject.h"


#include "ImageReadDicomWriteCLP.h"


// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace {

} // end of anonymous namespace


int main( int argc, char* argv[] )
{
  PARSE_ARGS;


  typedef itk::Image<short,3>               Image3DType;
  typedef itk::Image<short,2>               Image2DType;
  typedef itk::ImageFileReader< Image3DType > ReaderType;
  typedef itk::ShiftScaleImageFilter< Image3DType, Image3DType > ShiftScaleType;
  typedef itk::ImageFileWriter< Image2DType > WriterType;
  typedef itk::GDCMImageIO                  ImageIOType;

  Image3DType::Pointer image;
  ReaderType::Pointer reader = ReaderType::New();

  try
    {
    reader->SetFileName(inputVolume.c_str());
    reader->Update();
    image = reader->GetOutput();
    }
  catch (itk::ExceptionObject &excp)
    {
    std::cerr << "Exception thrown while reading the image file: " << inputVolume << std::endl;
    std::cerr << excp << std::endl;

    return EXIT_FAILURE;
    }

  Image3DType::SpacingType spacing = image->GetSpacing();
  Image3DType::DirectionType oMatrix = image->GetDirection();

  // Shift scale the data if necessary based on the rescale slope and
  // rescale interscept prescribed.
  if (fabs(rescaleIntercept) > itk::NumericTraits<double>::epsilon()
      || fabs(rescaleSlope - 1.0) > itk::NumericTraits<double>::epsilon())
    {
    reader->ReleaseDataFlagOn();
    
    ShiftScaleType::Pointer shiftScale = ShiftScaleType::New();
    shiftScale->SetInput( reader->GetOutput() );
    shiftScale->SetShift( -rescaleIntercept );

    if (fabs(rescaleSlope) < itk::NumericTraits<double>::epsilon())
      {
      // too close to zero, ignore
      std::cerr << "Rescale slope too close to zero (" << rescaleSlope
                << "). Using the default value of 1.0" << std::endl;

      rescaleSlope = 1.0;
      }
    shiftScale->SetScale( 1.0/rescaleSlope );

    shiftScale->Update();
    image = shiftScale->GetOutput();
    }


  typedef itk::MetaDataDictionary   DictionaryType;
  unsigned int numberOfSlices = image->GetLargestPossibleRegion().GetSize()[2];

  ImageIOType::Pointer gdcmIO = ImageIOType::New();
  DictionaryType dictionary;

  // Progress
  std::cout << "<filter-start>"
            << std::endl;
  std::cout << "<filter-name>"
            << "ImageFileWriter"
            << "</filter-name>"
            << std::endl;
  std::cout << "<filter-comment>"
            << " \"" << "Creating DICOM"
            << " \""
            << std::endl;
  std::cout << "</filter-start>"
            << std::endl;
  std::cout << std::flush;
  float progress = 1.0 / (float) numberOfSlices;
  for (unsigned int i = 0; i < numberOfSlices; i++)
    {
    std::cout << "<filter-progress>"
              << (i + 1) * progress
              << "</filter-progress>" 
              << std::endl
              << std::flush;
    
    itksys_ios::ostringstream value;
    Image3DType::PointType origin;
    Image3DType::IndexType index;
    index.Fill(0);
    index[2] = i;
    image->TransformIndexToPhysicalPoint(index, origin);

    // Set all required DICOM fields
    value.str("");
    value << origin[0] << "\\" << origin[1] << "\\" << origin[2];
    itk::EncapsulateMetaData<std::string>(dictionary,"0020|0032", value.str());

    value.str("");
    value << i + 1;
    itk::EncapsulateMetaData<std::string>(dictionary,"0020|0013", value.str());

    itk::EncapsulateMetaData<std::string>(dictionary,"0008|0008",std::string("ORIGINAL\\PRIMARY\\AXIAL"));  // Image Type
    itk::EncapsulateMetaData<std::string>(dictionary,"0008|0016",std::string("1.2.840.10008.5.1.4.1.1.2")); // SOP Class UID
    itk::EncapsulateMetaData<std::string>(dictionary,"0010|0030",std::string("20060101")); // Patient's Birthdate
    itk::EncapsulateMetaData<std::string>(dictionary,"0010|0032",std::string("010100.000000") ); // Patient's Birth Time
    itk::EncapsulateMetaData<std::string>(dictionary,"0010|0040", std::string("M")); // Patient's Sex
    itk::EncapsulateMetaData<std::string>(dictionary,"0008|0020", std::string("20050101")); // Study Date
    itk::EncapsulateMetaData<std::string>(dictionary,"0008|0030", std::string("010100.000000")); // Study Time
    itk::EncapsulateMetaData<std::string>(dictionary,"0008|0050", std::string("1")); // Accession Number
    itk::EncapsulateMetaData<std::string>(dictionary,"0008|0090", std::string("Unknown")); // Referring Physician's Name
    itk::EncapsulateMetaData<std::string>(dictionary,"0018|5100", std::string("HFS")); // Patient Position
    itk::EncapsulateMetaData<std::string>(dictionary,"0020|1040", std::string("SN"));  // Position Reference Indicator
    //itk::EncapsulateMetaData<std::string>(dictionary,"0020|0037", std::string("1.000000\\0.000000\\0.000000\\0.000000\\1.000000\\0.000000")); // Image Orientation (Patient)
    value.str("");
    value << oMatrix[0][0] << "\\" << oMatrix[1][0] << "\\" << oMatrix[2][0] << "\\";
    value << oMatrix[0][1] << "\\" << oMatrix[1][1] << "\\" << oMatrix[2][1] ;
    itk::EncapsulateMetaData<std::string>(dictionary,"0020|0037", value.str()); // Image Orientation (Patient)
    value.str("");
    value << spacing[2];
    itk::EncapsulateMetaData<std::string>(dictionary,"0018|0050", value.str()); // Slice Thickness

    // Parameters from the command line
    if (patientName.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0010|0010", patientName);
      }
    if (patientID.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0010|0020", patientID);
      }
    if (patientComments.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0010|4000", patientComments);
      }
    if (studyID.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0020|0010", studyID);
      }
    if (studyDate.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0008|0020", studyDate);
      }
    if (studyComments.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0032|4000", studyComments);
      }
    if (studyDescription.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0008|1030", studyDescription);
      }
    if (modality.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0008|0060", modality);
      }
    if (manufacturer.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0008|0070", manufacturer);
      }
    if (model.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0008|1090", model);
      }
    if (seriesNumber.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0020|0011", seriesNumber);
      }
    if (seriesDescription.size() >0)
      {
      itk::EncapsulateMetaData<std::string>(dictionary, "0008|103e", seriesDescription);
      }


    // Always set the rescale interscept and rescale slope (even if
    // they are at their defaults of 0 and 1 respectively).
    //value.str("");
    //value << rescaleIntercept;
    //itk::EncapsulateMetaData<std::string>(dictionary, "0028|1052", value.str());
    //value.str("");
    //value << rescaleSlope;
    //itk::EncapsulateMetaData<std::string>(dictionary, "0028|1053", value.str());
    
    
    Image3DType::RegionType extractRegion;
    Image3DType::SizeType extractSize;
    Image3DType::IndexType extractIndex;
    extractSize = image->GetLargestPossibleRegion().GetSize();
      extractIndex.Fill(0);
      if (reverseImages)
        {
        extractIndex[2] = numberOfSlices - i - 1;
        }
      else
        {
        extractIndex[2] = i;
        }
      extractSize[2] = 0;
      extractRegion.SetSize(extractSize);
      extractRegion.SetIndex(extractIndex);

    typedef itk::ExtractImageFilter< Image3DType, Image2DType > ExtractType;
    ExtractType::Pointer extract = ExtractType::New();
#if  ITK_VERSION_MAJOR >=4
     extract->SetDirectionCollapseToGuess(); //ITKv3 compatible, but not recommended
#endif
      extract->SetInput(image );
      extract->SetExtractionRegion(extractRegion);
      extract->GetOutput()->SetMetaDataDictionary(dictionary);
      extract->Update();

    itk::ImageRegionIterator<Image2DType> it( extract->GetOutput(), extract->GetOutput()->GetLargestPossibleRegion() );
    Image2DType::PixelType minValue = itk::NumericTraits<Image2DType::PixelType>::max();
    Image2DType::PixelType maxValue = itk::NumericTraits<Image2DType::PixelType>::min();
    for (it.GoToBegin(); !it.IsAtEnd(); ++it)
    {
      Image2DType::PixelType p = it.Get();
      if (p > maxValue)
      {
        maxValue = p;
      }
      if (p < minValue)
      {
        minValue = p;
      }
    }
    Image2DType::PixelType windowCenter = (minValue+maxValue)/2;
    Image2DType::PixelType windowWidth = (maxValue-minValue);

    value.str("");
    value << windowCenter;
    itk::EncapsulateMetaData<std::string>(dictionary, "0028|1050", value.str());
    value.str("");
    value << windowWidth;
    itk::EncapsulateMetaData<std::string>(dictionary, "0028|1051", value.str());

    WriterType::Pointer writer = WriterType::New();
      char imageNumber[BUFSIZ];
#if WIN32
#define snprintf sprintf_s
#endif
      snprintf (imageNumber, BUFSIZ, dicomNumberFormat.c_str(), i+1);
      value.str("");
      value << dicomDirectory << "/" << dicomPrefix << imageNumber << ".dcm";
      writer->SetFileName(value.str().c_str());

      writer->SetInput(extract->GetOutput());
      writer->SetUseCompression(useCompression);
      try
        {
        writer->SetImageIO(gdcmIO);
        writer->Update();
        }
      catch( itk::ExceptionObject & excp )
        {
        std::cerr << "Exception thrown while writing the file " << std::endl;
        std::cerr << excp << std::endl;
        return EXIT_FAILURE;
        }
    }
  std::cout << "<filter-end>"
            << std::endl;
  std::cout << "<filter-name>"
            << "ImageFileWriter"
            << "</filter-name>"
            << std::endl;
  std::cout << "</filter-end>";
  std::cout << std::flush;


  return EXIT_SUCCESS;
}

