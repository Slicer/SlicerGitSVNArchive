
#include "itkFactoryRegistration.h"

// ITK includes
#include <itk/Modules/IO/ImageBase/include/itkImageFileReader.h>
#include <itk/Modules/IO/TransformBase/include/itkTransformFileReader.h>

// The following code is required to ensure that the
// mechanism allowing the ITK factory to be registered is not
// optimized out by the compiler.
void itk::itkFactoryRegistration(void)
{
  return;
}
