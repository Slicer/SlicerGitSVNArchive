#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#ifdef __BORLANDC__
#define ITK_LEAN_AND_MEAN
#endif




#include "vtkNRRDReader.h"
#include "vtkSeedTracts.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkMath.h"
#include "vtkImageCast.h"

#include "SeedingCLP.h"

int main( int argc, char * argv[] )
{

  PARSE_ARGS;
  try {
  vtkNRRDReader *reader = vtkNRRDReader::New();
  reader->SetFileName(InputVolume.c_str());
  reader->Update();
  
  if ( reader->GetOutput()->GetPointData()->GetTensors() == NULL )
    {
    std::cerr << argv[0] << ": No tensor data" << std::endl;
    return EXIT_FAILURE;
    }

  vtkNRRDReader *reader2 = vtkNRRDReader::New();
  reader2->SetFileName(InputROI.c_str());
  reader2->Update();
  
  /*
  vtkNRRDWriter *iwriter = vtkNRRDWriter::New();
  iwriter->SetInput(reader->GetOutput());
  iwriter->SetFileName("C:/Temp/helix.nhdr");
  iwriter->Write();
  iwriter->Delete();
  **/

  if ( reader2->GetOutput()->GetPointData()->GetScalars() == NULL )
    {
    std::cerr << argv[0] << ": No roi data" << std::endl;
    return EXIT_FAILURE;
    }
  
  vtkSeedTracts *seed = vtkSeedTracts::New();
  
  //1. Set Input
  seed->SetInputTensorField(reader->GetOutput());
  
  //2. Set Up matrices
  vtkMatrix4x4 *TensorRASToIJK = vtkMatrix4x4::New();
  TensorRASToIJK->DeepCopy(reader->GetRasToIjkMatrix());
  
  // VTK seeding is in ijk space with voxel scale included.
  // Calculate the matrix that goes from tensor "scaled IJK", 
  // the array with voxels that know their size (what vtk sees for tract seeding)
  // to our RAS.
  double sp[3];
  reader->GetOutput()->GetSpacing(sp);
  vtkTransform *trans = vtkTransform::New();
  trans->Identity();
  trans->PreMultiply();
  trans->SetMatrix(TensorRASToIJK);
  // Trans from IJK to RAS
  trans->Inverse();
  // Take into account spacing (remove from matrix) to compute Scaled IJK to RAS matrix
  trans->Scale(1/sp[0],1/sp[1],1/sp[2]);
  trans->Inverse();
  
  //Set Transformation to seeding class
  seed->SetWorldToTensorScaledIJK(trans);
  
  // Rotation part of matrix is only thing tensor is transformed by.
  // This is to transform output tensors into RAS space.
  // Tensors are output along the fibers.
  // This matrix is not used for calculating tractography.
  // The following should be replaced with finite strain method
  // rather than assuming rotation part of the matrix according to 
  // slicer convention.
  vtkMatrix4x4 *TensorRASToIJKRotation = vtkMatrix4x4::New();
  TensorRASToIJKRotation->DeepCopy(TensorRASToIJK);
  
  //Set Translation to zero
  for (int i=0;i<3;i++)
    {
    TensorRASToIJKRotation->SetElement(i,3,0);
    }
  //Remove scaling in rasToIjk to make a real rotation matrix
  double col[3];
  for (int jjj = 0; jjj < 3; jjj++) 
    {
    for (int iii = 0; iii < 3; iii++)
      {
      col[iii]=TensorRASToIJKRotation->GetElement(iii,jjj);
      }
    vtkMath::Normalize(col);
    for (int iii = 0; iii < 3; iii++)
      {
      TensorRASToIJKRotation->SetElement(iii,jjj,col[iii]);
     }  
  }
  TensorRASToIJKRotation->Invert();
  seed->SetTensorRotationMatrix(TensorRASToIJKRotation);  


  //vtkNRRDWriter *iwriter = vtkNRRDWriter::New();
  
  // 3. Set up ROI (not based on Cl mask), from input now

  // cast roi to short data type
  vtkImageCast *imageCast = vtkImageCast::New();
  imageCast->SetOutputScalarTypeToShort();
  imageCast->SetInput(reader2->GetOutput());
  imageCast->Update();

  //Create Cl mask
  /**
  iwriter->SetInput(imageCast->GetOutput());
  iwriter->SetFileName("C:/Temp/cast.nhdr");
  iwriter->Write();


  vtkDiffusionTensorMathematicsSimple *math = vtkDiffusionTensorMathematicsSimple::New();
  math->SetInput(0, reader->GetOutput());
  // math->SetInput(1, reader->GetOutput());
  math->SetScalarMask(imageCast->GetOutput());
  math->MaskWithScalarsOn();
  math->SetMaskLabelValue(ROIlabel);
  math->SetOperationToLinearMeasure();
  math->Update();
  
  iwriter->SetInput(math->GetOutput());
  iwriter->SetFileName("C:/Temp/math.nhdr");
  iwriter->Write();

  vtkImageThreshold *th = vtkImageThreshold::New();
  th->SetInput(math->GetOutput());
  th->ThresholdBetween(ClTh,1);
  th->SetInValue(1);
  th->SetOutValue(0);
  th->ReplaceInOn();
  th->ReplaceOutOn();
  th->SetOutputScalarTypeToShort();
  th->Update();

  iwriter->SetInput(th->GetOutput());
  iwriter->SetFileName("C:/Temp/th.nhdr");
  iwriter->Write();
  **/
  
  //PENDING: Do merging with input ROI
  
  seed->SetInputROI(imageCast->GetOutput());
  seed->SetInputROIValue(ROIlabel);
  seed->UseStartingThresholdOn();
  seed->SetStartingThreshold(ClTh);

  // Set up the matrix that will take points in ROI
  // to RAS space.  Code assumes this is world space
  // since  we have no access to external transforms.
  // This will only work if no transform is applied to 
  // ROI and tensor volumes.
  vtkMatrix4x4 *ROIRASToIJK = vtkMatrix4x4::New();
  ROIRASToIJK->DeepCopy(reader2->GetRasToIjkMatrix());
  vtkTransform *trans2 = vtkTransform::New();
  trans2->Identity();
  trans2->PreMultiply();
  // no longer assume this ROI is in tensor space
  //trans2->SetMatrix(TensorRASToIJK);
  trans2->SetMatrix(ROIRASToIJK);
  trans2->Inverse();
  seed->SetROIToWorld(trans2);
  
 
  //4. Set Tractography specific parameters
  
  if (WriteToFile) 
    {
    seed->SetFileDirectoryName(OutputDirectory.c_str());
    if (FilePrefix.length() > 0)
      {
      seed->SetFilePrefix(FilePrefix.c_str());
      }
    }
  
  if (UseIndexSpace) 
    {
    seed->SetIsotropicSeeding(0);
    }
  else
    {
    seed->SetIsotropicSeeding(1);
    }

  seed->SetIsotropicSeedingResolution(SeedSpacing);
  seed->SetMinimumPathLength(MinimumLength);
  seed->UseVtkHyperStreamlinePoints();
  vtkHyperStreamlineDTMRI *streamer=vtkHyperStreamlineDTMRI::New();
  seed->SetVtkHyperStreamlinePointsSettings(streamer);
  
  if (StoppingMode == std::string("LinearMeasurement") || StoppingMode == std::string("LinearMeasure"))
    {
     streamer->SetStoppingModeToLinearMeasure();
    }
  else if (StoppingMode == std::string("PlanarMeasurement") || StoppingMode == std::string("PlanarMeasure"))
    {  
    streamer->SetStoppingModeToPlanarMeasure();
    }
  else if (StoppingMode == std::string("FractionalAnisotropy"))
    {  
    streamer->SetStoppingModeToFractionalAnisotropy();
    }
  else
    {
    std::cerr<<"Mode "<<StoppingMode<<" is not supported"<<endl;
    return EXIT_FAILURE;
    }
      
  streamer->SetStoppingThreshold(StoppingValue);
  streamer->SetMaximumPropagationDistance(MaximumLength);
  streamer->SetRadiusOfCurvature(StoppingCurvature);
  streamer->SetIntegrationStepLength(IntegrationStepLength);

  // Temp fix to provide a scalar
  //seed->GetInputTensorField()->GetPointData()->SetScalars(math->GetOutput()->GetPointData()->GetScalars());

  //5. Run the thing
  seed->SeedStreamlinesInROI();
  
  //6. Extra5ct PolyData in RAS
  vtkPolyData *outFibers = vtkPolyData::New();
  
  //Save result
  vtkXMLPolyDataWriter *writer = vtkXMLPolyDataWriter::New();
  if (!WriteToFile) 
    {
    seed->TransformStreamlinesToRASAndAppendToPolyData(outFibers);
    writer->SetFileName(OutputFibers.c_str());
    //writer->SetFileTypeToBinary();
    writer->SetInput(outFibers);
    writer->Write();
    }
  // Delete everything: Still trying to figure out what is going on
  reader->Delete();
  outFibers->Delete();
  seed->Delete();
  TensorRASToIJK->Delete();
  ROIRASToIJK->Delete();
  TensorRASToIJKRotation->Delete();
  //math->Delete();
  //th->Delete();
  trans2->Delete();
  trans->Delete();
  streamer->Delete();
  reader2->Delete();
  writer->Delete();
  imageCast->Delete();
  }
  catch (...) 
    { 
    cout << "default exception"; 
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
