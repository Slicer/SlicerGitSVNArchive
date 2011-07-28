#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#ifdef __BORLANDC__
#define ITK_LEAN_AND_MEAN
#endif


#include "vtkSmartPointer.h"
#include "vtkNRRDReader.h"
#include "vtkNRRDWriter.h"
#include "vtkMRMLNRRDStorageNode.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageWeightedSum.h"

#include "vtkImageSeedConnectivity.h"
#include "vtkImageConnectivity.h"
#include "vtkITKNewOtsuThresholdImageFilter.h"

#if ITK_VERSION_MAJOR >= 4
#include "itkFloatingPointExceptions.h"
#endif

#include "DiffusionWeightedMaskingCLP.h"

#define GRAD_0_TOL 1e-6

int main( int argc, char * argv[] )
  {

#if ITK_VERSION_MAJOR >= 4
  itk::FloatingPointExceptions::Disable();
#endif

  PARSE_ARGS;
    {
    vtkSmartPointer<vtkNRRDReader> reader =
      vtkSmartPointer<vtkNRRDReader>::New();
    reader->SetFileName(inputVolume.c_str());
    reader->Update();
    if ( reader->GetReadStatus() )
      {
      std::cerr << argv[0] << ": Error reading Diffusion file" << std::endl;
      return EXIT_FAILURE;
      }

    vtkSmartPointer<vtkDoubleArray> bValues =
      vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> grads =
      vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkMRMLNRRDStorageNode> helper =
      vtkSmartPointer<vtkMRMLNRRDStorageNode>::New();

    if ( !helper->ParseDiffusionInformation(reader,grads,bValues) )
      {
      std::cerr << argv[0] << ": Error parsing Diffusion information" << std::endl;
      return EXIT_FAILURE;
      }

    //Compute the mean baseline image
    vtkSmartPointer<vtkImageWeightedSum> imageWeightedSum = vtkSmartPointer<vtkImageWeightedSum>::New();
    imageWeightedSum->NormalizeByWeightOn();

    int b0_count = 0;
    for (int gradient_n=0; gradient_n<grads->GetNumberOfTuples(); gradient_n++)
      {
      double* gradient = grads->GetTuple3(gradient_n);
      if (abs(gradient[0]) + abs(gradient[1]) + abs(gradient[2]) < GRAD_0_TOL)
        {
          vtkSmartPointer<vtkImageExtractComponents> extractComponents = vtkSmartPointer<vtkImageExtractComponents>::New();
          extractComponents->SetInput(reader->GetOutput());
          extractComponents->SetComponents(gradient_n);
          extractComponents->Update();

          imageWeightedSum->AddInputConnection(extractComponents->GetOutputPort());
          imageWeightedSum->SetWeight(b0_count++, 1.);
        }
      }
    imageWeightedSum->Update();

    if (b0_count == 0)
      {
      std::cerr << argv[0] << ": Error parsing Diffusion information, no B0 images" << std::endl;
      return EXIT_FAILURE;       
      } 

    
    //compute DWI mask
    vtkSmartPointer<vtkITKNewOtsuThresholdImageFilter> otsu =
      vtkSmartPointer<vtkITKNewOtsuThresholdImageFilter>::New();
    otsu->SetInput(imageWeightedSum->GetOutput());
    otsu->SetOmega (1 + otsuOmegaThreshold);
    otsu->SetOutsideValue(1);
    otsu->SetInsideValue(0);
    otsu->Update();

    vtkSmartPointer<vtkImageData> mask = vtkSmartPointer<vtkImageData>::New();
    mask->DeepCopy(otsu->GetOutput());

    int *dims = mask->GetDimensions();
    int px = dims[0]/2;
    int py = dims[1]/2;
    int pz = dims[2]/2;

    vtkSmartPointer<vtkImageCast> cast =
      vtkSmartPointer<vtkImageCast>::New();
    cast->SetInput(mask);
    cast->SetOutputScalarTypeToUnsignedChar();
    cast->Update();

    vtkSmartPointer<vtkImageSeedConnectivity> con =
      vtkSmartPointer<vtkImageSeedConnectivity>::New();
    con->SetInput(cast->GetOutput());
    con->SetInputConnectValue(1);
    con->SetOutputConnectedValue(1);
    con->SetOutputUnconnectedValue(0);
    con->AddSeed(px, py, pz);
    con->Update();

    vtkSmartPointer<vtkImageCast> cast1 =
      vtkSmartPointer<vtkImageCast>::New();
    cast1->SetInput(con->GetOutput());
    cast1->SetOutputScalarTypeToShort();
    cast1->Update();


    vtkSmartPointer<vtkImageConnectivity> conn =
      vtkSmartPointer<vtkImageConnectivity>::New();

    if (removeIslands)  
      {
      conn->SetBackground(1);
      conn->SetMinForeground( -32768);
      conn->SetMaxForeground( 32767);
      conn->SetFunctionToRemoveIslands();
      conn->SetMinSize(10000);
      conn->SliceBySliceOn();
      conn->SetInput(cast1->GetOutput());
      conn->Update();
      }


    vtkSmartPointer<vtkImageCast> cast2 =
      vtkSmartPointer<vtkImageCast>::New();
    cast2->SetOutputScalarTypeToUnsignedChar();

    if (removeIslands)  
      {
      cast2->SetInput(conn->GetOutput());
      }
    else
      {
      cast2->SetInput(cast1->GetOutput());
      }

    vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix = reader->GetRasToIjkMatrix();
    ijkToRasMatrix->Invert();
    
    //Save baseline
    vtkSmartPointer<vtkNRRDWriter> writer =
      vtkSmartPointer<vtkNRRDWriter>::New();
    writer->SetInput(imageWeightedSum->GetOutput());
    writer->SetFileName( outputBaseline.c_str() );
    writer->UseCompressionOn();
    writer->SetIJKToRASMatrix( ijkToRasMatrix );
    writer->Write();

    //Save mask
    vtkSmartPointer<vtkNRRDWriter> writer2 =
      vtkSmartPointer<vtkNRRDWriter>::New();
    if (removeIslands)  
      {
      writer2->SetInput(conn->GetOutput());
      }
    else
      {
      writer2->SetInput(cast1->GetOutput());
      }
    writer2->SetFileName( thresholdMask.c_str() );
    writer2->UseCompressionOn();
    writer2->SetIJKToRASMatrix( ijkToRasMatrix );
    writer2->Write();

    }

    return EXIT_SUCCESS;
  }
