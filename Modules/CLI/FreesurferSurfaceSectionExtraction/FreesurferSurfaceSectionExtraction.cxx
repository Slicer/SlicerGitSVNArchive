#include "FreesurferSurfaceSectionExtractionCLP.h"
#include "itk/Modules/Core/Common/include/itkImage.h"
#include "itk/Modules/IO/ImageBase/include/itkImageFileReader.h"
#include "itk/Modules/IO/ImageBase/include/itkImageFileWriter.h"
#include "itk/Modules/Filtering/Smoothing/include/itkDiscreteGaussianImageFilter.h"

int main(int argc, char * argv [])
{
  PARSE_ARGS;
  std::cout << "Hello World!" << std::endl;
  return EXIT_SUCCESS;
}
