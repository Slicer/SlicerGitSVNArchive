/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkSeedTracts.cxx,v $
  Date:      $Date: 2007/02/23 22:13:46 $
  Version:   $Revision: 1.25 $

=========================================================================auto=*/

// vtkTeem includes
#include "vtkSeedTracts.h"

// VTK includes
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCellArray.h>
#include <vtkCommand.h>
#include <vtkInformation.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyDataWriter.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTimerLog.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVersion.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSeedTracts);

//----------------------------------------------------------------------------
vtkSeedTracts::vtkSeedTracts()
{
  // matrices
  // Initialize these to identity, so if the user doesn't set them it's okay.
  this->ROIToWorld = vtkTransform::New();
  this->ROI2ToWorld = vtkTransform::New();
  this->WorldToTensorScaledIJK = vtkTransform::New();
  this->TensorRotationMatrix = vtkMatrix4x4::New();

  // The user must set these for the class to function.
  this->InputTensorFieldConnection = NULL;

  // The user may need to set these, depending on class usage
  this->InputROIConnection = NULL;
  this->InputROIValue = -1;
  this->InputMultipleROIValues = NULL;
  this->InputROIConnection2 = NULL;
  this->IsotropicSeeding = 0;
  this->IsotropicSeedingResolution = 2;
  this->RandomGrid = 0;

  // if the user doesn't set these they will be ignored
  this->VtkHyperStreamlineSettings=NULL;
  this->VtkHyperStreamlinePointsSettings=NULL;
  this->VtkPreciseHyperStreamlinePointsSettings=NULL;
  this->VtkHyperStreamlineTeemSettings=NULL;

  // default to vtkHyperStreamline class creation
  this->UseVtkHyperStreamline();

  // collections
  this->Streamlines = vtkCollection::New();


  // Streamline parameters for all streamlines
  this->IntegrationDirection = VTK_INTEGRATE_BOTH_DIRECTIONS;

  this->MinimumPathLength = 15;
  this->FileDirectoryName = NULL;
  this->FilePrefix = NULL;
  this->UseStartingThreshold = 0;
  this->StartingThreshold = 0;
}

//----------------------------------------------------------------------------
vtkSeedTracts::~vtkSeedTracts()
{
  // matrices
  this->ROIToWorld->Delete();
  this->ROI2ToWorld->Delete();
  this->WorldToTensorScaledIJK->Delete();
  this->TensorRotationMatrix->Delete();

  // volumes
  if (this->InputTensorFieldConnection) this->InputTensorFieldConnection->Delete();
  if (this->InputROIConnection) this->InputROIConnection->Delete();
  if (this->InputROIConnection2) this->InputROIConnection2->Delete();

  // settings
  if (this->VtkHyperStreamlineSettings)
    this->VtkHyperStreamlineSettings->Delete();
  if (this->VtkHyperStreamlinePointsSettings)
    this->VtkHyperStreamlinePointsSettings->Delete();
  if (this->VtkPreciseHyperStreamlinePointsSettings)
    this->VtkPreciseHyperStreamlinePointsSettings->Delete();
  if (this->VtkHyperStreamlineTeemSettings)
    this->VtkHyperStreamlineTeemSettings->Delete();

  // collection
  if (this->Streamlines)
    {
    this->DeleteAllStreamlines();
    this->Streamlines->Delete();
    }
  if (FileDirectoryName)
    {
    delete [] FileDirectoryName;
    }
  if (FilePrefix)
    {
    delete [] FilePrefix;
    }
}


// Here we create the type of streamline class requested by the user.
// Elsewhere in this class, all are treated as vtkHyperStreamline *.
// We copy settings from the example object that the user has access
// to.
// (It would be nicer if we required the hyperstreamline classes to
// implement a copy function.)
//----------------------------------------------------------------------------
vtkHyperStreamline * vtkSeedTracts::CreateHyperStreamline()
{
  vtkHyperStreamline *currHS;
  vtkHyperStreamlineDTMRI *currHSP;
  vtkPreciseHyperStreamlinePoints *currPHSP;
  vtkHyperStreamlineTeem *currHST;

  vtkDebugMacro(<< "in create HyperStreamline, type " << this->TypeOfHyperStreamline);


  switch (this->TypeOfHyperStreamline)
    {
    case USE_VTK_HYPERSTREAMLINE:
      if (this->VtkHyperStreamlineSettings)
        {
          currHS=vtkHyperStreamline::New();
          return(currHS);
        }
      else
        {
          return(vtkHyperStreamline::New());
        }
      break;
    case USE_VTK_HYPERSTREAMLINE_TEEM:
      if (this->VtkHyperStreamlineTeemSettings)
        {
          // create object
          std::cout << "Creatng HST" << endl;
          currHST=vtkHyperStreamlineTeem::New();

          std::cout << "settings for HST" << endl;

          this->UpdateHyperStreamlineTeemSettings(currHST);


          std::cout << "returning HST" << endl;

          return((vtkHyperStreamline *)currHST);
        }
      else
        {
          return((vtkHyperStreamline *) vtkHyperStreamlineTeem::New());
        }
      break;
    case USE_VTK_HYPERSTREAMLINE_POINTS:
      if (this->VtkHyperStreamlinePointsSettings)
        {
          // create object
          currHSP=vtkHyperStreamlineDTMRI::New();

          this->UpdateHyperStreamlinePointsSettings(currHSP);

          return((vtkHyperStreamline *)currHSP);
        }
      else
        {
          return((vtkHyperStreamline *) vtkHyperStreamlineDTMRI::New());

        }


      break;
    case USE_VTK_PRECISE_HYPERSTREAMLINE_POINTS:
      if (this->VtkPreciseHyperStreamlinePointsSettings)
        {

          // create object
          currPHSP=vtkPreciseHyperStreamlinePoints::New();

          // Now copy user's settings into this object:
          // Method
          currPHSP->
            SetMethod(this->VtkPreciseHyperStreamlinePointsSettings->
                      GetMethod());
          // Terminal FA
          currPHSP->
            SetTerminalFractionalAnisotropy(this->VtkPreciseHyperStreamlinePointsSettings->GetTerminalFractionalAnisotropy());
          // MaximumPropagationDistance
          currPHSP->
            SetMaximumPropagationDistance(this->VtkPreciseHyperStreamlinePointsSettings->GetMaximumPropagationDistance());
          // MinimumPropagationDistance
          currPHSP->
            SetMinimumPropagationDistance(this->VtkPreciseHyperStreamlinePointsSettings->GetMinimumPropagationDistance());
          // TerminalEigenvalue
          currPHSP->
            SetTerminalEigenvalue(this->VtkPreciseHyperStreamlinePointsSettings->GetTerminalEigenvalue());
          // IntegrationStepLength
          currPHSP->
            SetIntegrationStepLength(this->VtkPreciseHyperStreamlinePointsSettings->GetIntegrationStepLength());
          // StepLength
          currPHSP->
            SetStepLength(this->VtkPreciseHyperStreamlinePointsSettings->GetStepLength());
          // Radius
          currPHSP->
            SetRadius(this->VtkPreciseHyperStreamlinePointsSettings->GetRadius());
          // NumberOfSides
          currPHSP->
            SetNumberOfSides(this->VtkPreciseHyperStreamlinePointsSettings->GetNumberOfSides());

          // Eigenvector to integrate
          currPHSP->SetIntegrationEigenvector(this->VtkPreciseHyperStreamlinePointsSettings->GetIntegrationEigenvector());

          // IntegrationDirection (set in this class, default both ways)
          currPHSP->SetIntegrationDirection(this->IntegrationDirection);

          // MaxStep
          currPHSP->
            SetMaxStep(this->VtkPreciseHyperStreamlinePointsSettings->GetMaxStep());
          // MinStep
          currPHSP->
            SetMinStep(this->VtkPreciseHyperStreamlinePointsSettings->GetMinStep());
          // MaxError
          currPHSP->
            SetMaxError(this->VtkPreciseHyperStreamlinePointsSettings->GetMaxError());
          // MaxAngle
          currPHSP->
            SetMaxAngle(this->VtkPreciseHyperStreamlinePointsSettings->GetMaxAngle());
          // LengthOfMaxAngle
          currPHSP->
            SetLengthOfMaxAngle(this->VtkPreciseHyperStreamlinePointsSettings->GetLengthOfMaxAngle());

          return((vtkHyperStreamline *) currPHSP);
          //
        }
      else
        {
          return((vtkHyperStreamline *) vtkPreciseHyperStreamlinePoints::New());
        }
      break;
    }
  return (NULL);
}

// Loop through all of the hyperstreamline objects and set their
// parameters according to the current vtkHyperStreamline*Settings object
// which the user can modify.
//----------------------------------------------------------------------------
void vtkSeedTracts::UpdateAllHyperStreamlineSettings()
{
  vtkObject *currStreamline;
  vtkHyperStreamlineDTMRI *currHSP;
  vtkHyperStreamlineTeem *currHST;

  // traverse streamline collection
  this->Streamlines->InitTraversal();

  currStreamline= (vtkObject *)this->Streamlines->GetNextItemAsObject();

  while(currStreamline)
    {
      vtkDebugMacro( << currStreamline->GetClassName() );
      if (strcmp(currStreamline->GetClassName(),"vtkHyperStreamlineDTMRI") == 0)
        {
          vtkDebugMacro( << " match" );
          currHSP = (vtkHyperStreamlineDTMRI *) currStreamline;
          this->UpdateHyperStreamlinePointsSettings(currHSP);
          currHSP->Update();
        }
      if (strcmp(currStreamline->GetClassName(),"vtkHyperStreamlineTeem") == 0)
        {
          vtkDebugMacro( << " match" );
          currHST = (vtkHyperStreamlineTeem *) currStreamline;
          this->UpdateHyperStreamlineTeemSettings(currHST);
          currHST->Update();
        }

      currStreamline= (vtkObject *)this->Streamlines->GetNextItemAsObject();
    }
}

// Update settings of one hyper streamline
//----------------------------------------------------------------------------
void vtkSeedTracts::UpdateHyperStreamlinePointsSettings( vtkHyperStreamlineDTMRI *currHSP)
{

  // Copy user's settings into this object:

  // MaximumPropagationDistance
  currHSP->SetMaximumPropagationDistance(this->VtkHyperStreamlinePointsSettings->GetMaximumPropagationDistance());
  // IntegrationStepLength
  currHSP->SetIntegrationStepLength(this->VtkHyperStreamlinePointsSettings->GetIntegrationStepLength());
  // RadiusOfCurvature
  currHSP->SetRadiusOfCurvature(this->VtkHyperStreamlinePointsSettings->GetRadiusOfCurvature());

  // Stopping threshold
  currHSP->SetStoppingThreshold(this->VtkHyperStreamlinePointsSettings->GetStoppingThreshold());

  // Stopping Mode
  currHSP->SetStoppingMode(this->VtkHyperStreamlinePointsSettings->GetStoppingMode());


  // Eigenvector to integrate
  currHSP->SetIntegrationEigenvector(this->VtkHyperStreamlinePointsSettings->GetIntegrationEigenvector());

  // IntegrationDirection (set in this class, default both ways)
  currHSP->SetIntegrationDirection(this->IntegrationDirection);

  currHSP->SetIntegrationStepLength(this->VtkHyperStreamlinePointsSettings->GetIntegrationStepLength());

}

// Update settings of one hyper streamline:
// This is where teem hyperstreamlines have their settings updated
// from the user interface.
//----------------------------------------------------------------------------
void vtkSeedTracts::UpdateHyperStreamlineTeemSettings( vtkHyperStreamlineTeem *currHST)
{

  std::cout << "in settings  function HST" << endl;

  // Potentially this should update the tendFiberContext class for the given volume,
  // instead of updating all streamlines.

  // Copy user's settings into this object:

  // MaximumPropagationDistance
  currHST->SetMaximumPropagationDistance(this->VtkHyperStreamlineTeemSettings->GetMaximumPropagationDistance());
  // IntegrationStepLength
  currHST->SetIntegrationStepLength(this->VtkHyperStreamlineTeemSettings->GetIntegrationStepLength());
  // RadiusOfCurvature
  currHST->SetRadiusOfCurvature(this->VtkHyperStreamlineTeemSettings->GetRadiusOfCurvature());

  // Stopping threshold
  currHST->SetStoppingThreshold(this->VtkHyperStreamlineTeemSettings->GetStoppingThreshold());

  // Stopping Mode
  currHST->SetStoppingMode(this->VtkHyperStreamlineTeemSettings->GetStoppingMode());


  // Eigenvector to integrate
  currHST->SetIntegrationEigenvector(this->VtkHyperStreamlineTeemSettings->GetIntegrationEigenvector());

  // IntegrationDirection (set in this class, default both ways)
  currHST->SetIntegrationDirection(this->IntegrationDirection);

  currHST->SetIntegrationStepLength(this->VtkHyperStreamlineTeemSettings->GetIntegrationStepLength());

  std::cout << "DONE in settings  function HST" << endl;
}


// Test whether the given point is in bounds (inside the input data)
//----------------------------------------------------------------------------
int vtkSeedTracts::PointWithinTensorData(double *point, double *pointw)
{
  double *bounds;
  int inbounds;

  vtkAlgorithm* producer = this->InputTensorFieldConnection->GetProducer();
  vtkImageData* inputTensorField = vtkImageData::SafeDownCast(producer->GetOutputDataObject(0));
  producer->Update();
  bounds = inputTensorField->GetBounds();
  vtkDebugMacro("Bounds " << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " << bounds[3] << " " << bounds[4] << " " << bounds[5]);

  inbounds=1;
  if (point[0] < bounds[0]) inbounds = 0;
  if (point[0] > bounds[1]) inbounds = 0;
  if (point[1] < bounds[2]) inbounds = 0;
  if (point[1] > bounds[3]) inbounds = 0;
  if (point[2] < bounds[4]) inbounds = 0;
  if (point[2] > bounds[5]) inbounds = 0;

  if (inbounds ==0)
    {
      std::cout << "point " << pointw[0] << " " << pointw[1] << " " << pointw[2] << " outside of tensor dataset" << endl;
    }

  return(inbounds);
}

//----------------------------------------------------------------------------
void vtkSeedTracts::SeedStreamlineFromPoint(double x,
                                            double y,
                                            double z)

{
  double pointw[3], point[3];
  vtkHyperStreamlineDTMRI *newStreamline;

  // test we have input
  if (this->InputTensorFieldConnection == NULL)
    {
      vtkErrorMacro("No tensor data input.");
      return;
    }

  pointw[0]=x;
  pointw[1]=y;
  pointw[2]=z;

  vtkDebugMacro("Starting streamline from point " << pointw[0] << " " << pointw[1] << " " << pointw[2]);

  // Transform from world coords to scaled ijk of the input tensors
  this->WorldToTensorScaledIJK->TransformPoint(pointw,point);

  vtkDebugMacro("Starting streamline from point " << point[0] << " " << point[1] << " " << point[2]);

  // make sure it is within the bounds of the tensor dataset
  if (!this->PointWithinTensorData(point,pointw))
    {
      vtkErrorMacro("Point " << x << ", " << y << ", " << z << " outside of tensor dataset.");
      return;
    }


  newStreamline=(vtkHyperStreamlineDTMRI *)this->CreateHyperStreamline();

  // Set its input information.
  newStreamline->SetInputConnection(this->InputTensorFieldConnection);
  newStreamline->SetStartPosition(point[0],point[1],point[2]);
  //newStreamline->DebugOn();

  // Ask it to output tensors and to only do one trajectory per start point
  newStreamline->OutputTensorsOn();
  newStreamline->OneTrajectoryPerSeedPointOn();

  // Force it to execute
  newStreamline->Update();

  double length = (newStreamline->GetOutput()->GetNumberOfPoints() - 1) *
                        newStreamline->GetIntegrationStepLength();

  if (length > this->MinimumPathLength)
    {
    this->Streamlines->AddItem((vtkObject *)newStreamline);
    }
  else
    {
    // Free allocated memory
    newStreamline->Delete();
    }
  //newStreamline->Delete();
}

// Seed in an ROI using a continous grid with the resolution given by
//this->IsotropicSeedingResolution.
//----------------------------------------------------------------------------
void vtkSeedTracts::SeedStreamlinesInROI()
{
  double idxX, idxY, idxZ;
  double maxX, maxY, maxZ;
  double gridIncX, gridIncY, gridIncZ;
  int inExt[6];
  double point[3], point2[3];

  short *inPtr;
  vtkHyperStreamlineDTMRI *newStreamline;
  int idx;


  // test we have input
  if (this->InputROIConnection == NULL)
    {
      vtkErrorMacro("No ROI input.");
      return;
    }
  if (this->InputTensorFieldConnection == NULL)
    {
      vtkErrorMacro("No tensor data input.");
      return;
    }
  // check ROI's value of interest
  if (this->InputROIValue <= 0)
    {
      vtkErrorMacro("Input ROI value has not been set or is 0. (value is "  << this->InputROIValue << ".");
      return;
    }
  // make sure it is short type
  // TODO

  vtkNew<vtkTransform> transform;
  transform->SetMatrix(this->WorldToTensorScaledIJK->GetMatrix());
  transform->Inverse();

  vtkNew<vtkTransformPolyDataFilter> transformer;
  transformer->SetTransform(transform.GetPointer());

  vtkNew<vtkPolyDataWriter> writer;

  // make sure we are creating objects with points
  this->UseVtkHyperStreamlinePoints();
 
  double spacing[3];

  vtkImageData* inputTensorField = vtkImageData::SafeDownCast(this->InputTensorFieldConnection->GetProducer()->GetOutputDataObject(0));
  inputTensorField->GetSpacing(spacing);

  // currently this filter is not multithreaded, though in the future
  // it could be (especially if it inherits from an image filter class)
  {
    this->InputROIConnection->GetProducer()->Update();
    vtkInformation *inInfo = this->InputROIConnection->GetProducer()->GetOutputInformation(0);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  }

  // find the region to loop over
  maxX = inExt[1] - inExt[0];
  maxY = inExt[3] - inExt[2];
  maxZ = inExt[5] - inExt[4];

  //cout << "Dims: " << maxX << " " << maxY << " " << maxZ << endl;
  //cout << "Incr: " << inIncX << " " << inIncY << " " << inIncZ << endl;

  // If we are iterating over a non-voxel (isotropic) grid, change the increments
  // to reflect this.  So we want to iterate in voxel (IJK) space still, but with
  // increments corresponding to the desired seed resolution.  The points are
  // then converted to world space and to tensor IJK for seeding.  So for example if
  // we want to seed at 2mm resolution, and in the x direction the voxel size is 0.85
  // mm, then we want the increment of 2/0.85 = 2.35 voxel units in the x direction.
  if (this->IsotropicSeeding)
    {
      gridIncX = this->IsotropicSeedingResolution/spacing[0];
      gridIncY = this->IsotropicSeedingResolution/spacing[1];
      gridIncZ = this->IsotropicSeedingResolution/spacing[2];
    }
  else
    {
      gridIncX = 1;
      gridIncY = 1;
      gridIncZ = 1;
    }

  double tensor[3][3];
  double *m[3], w[3], *v[3];
  double m0[3], m1[3], m2[3];
  double v0[3], v1[3], v2[3];
  m[0] = m0; m[1] = m1; m[2] = m2;
  v[0] = v0; v[1] = v1; v[2] = v2;

  // filename index
  idx=0;

  int gridIncXYZ = gridIncX*gridIncY*gridIncZ;
  int numLabelVoxels = 0;
  if (gridIncXYZ)
    {
    numLabelVoxels = maxX*maxY*maxZ/gridIncXYZ;
    }
  int labelVoxelCount = 0;
  int progressCount = 0;
  int progressCountMax = 100;
  double progress;

  vtkImageData* inputROI = vtkImageData::SafeDownCast(this->InputROIConnection->GetProducer()->GetOutputDataObject(0));

  for (idxZ = 0; idxZ <= maxZ; idxZ+=gridIncZ)
    {
      // just output (fractional or integer) current slice number
      std::cout << idxZ << " / " << maxZ << endl;

      //for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      for (idxY = 0; idxY <= maxY; idxY+=gridIncY)
        {

          for (idxX = 0; idxX <= maxX; idxX+=gridIncX)
            {
              labelVoxelCount++;
              // get the pointer to the nearest voxel at this location
              int pt[3];
              pt[0]= (int) floor(idxX + 0.5);
              pt[1]= (int) floor(idxY + 0.5);
              pt[2]= (int) floor(idxZ + 0.5);
              inPtr = (short *) inputROI->GetScalarPointer(pt);

              // If the point is equal to the ROI value then seed here.
              if (*inPtr == this->InputROIValue)
                {
                  vtkDebugMacro( << "start streamline at: " << idxX << " " <<
                                 idxY << " " << idxZ);

                  // First transform to world space.
                  point[0]=idxX;
                  point[1]=idxY;
                  point[2]=idxZ;
                  this->ROIToWorld->TransformPoint(point,point2);

                  // jitter about seed point if requested
                  // (now we are in a mm space, not voxels)
                  if (this->RandomGrid)
                    {
                      //Call random twice to avoid init problems
                      double rand=vtkMath::Random();

                      int ridx;
                      for (ridx = 0; ridx < 3; ridx++)
                        {
                          if (this->IsotropicSeeding)
                            {
                              // rand was from [0 .. 1], now from +/- half grid spacing
                              rand = vtkMath::Random( - this->IsotropicSeedingResolution / 2.0, this->IsotropicSeedingResolution / 2.0 );
                            }
                          else
                            {
                              // use half of the x voxel dimension
                              rand = vtkMath::Random( - spacing[0] / 2.0 , spacing[0] / 2.0 );
                            }

                          // add the random offset
                          point2[ridx] = point2[ridx] + rand;
                        }
                    }

                  // Now transform to scaled ijk of the input tensors
                  this->WorldToTensorScaledIJK->TransformPoint(point2,point);

                  // make sure it is within the bounds of the tensor dataset
                  if (this->PointWithinTensorData(point,point2))
                    {
                    if (this->UseStartingThreshold)
                      {
                      // Check the tensor threshold
                      int  ijk[3];
                      double  pcoords[3];
                      inputTensorField->ComputeStructuredCoordinates  ( point, ijk, pcoords);
                      vtkIdType tensorId = inputTensorField->ComputePointId  (ijk );

                      vtkDataArray *inTensors = inputTensorField->GetPointData()->GetTensors();
                      inTensors->GetTuple(tensorId,(double *)tensor);
                      for (int j=0; j<3; j++)
                        {
                        for (int i=0; i<3; i++)
                          {
                          // transpose
                          m[i][j] = tensor[j][i];
                          }
                        }
                      // compute eigensystem
                      //vtkMath::Jacobi(m, w, v);
                      vtkDiffusionTensorMathematics::TeemEigenSolver(m,w,v);
                      double cl = vtkDiffusionTensorMathematics::LinearMeasure(w);
                      if (cl < this->StartingThreshold)
                        {
                        continue;
                        }
                      } // end if (UseStartingThreshold)

                      // Report progress
                      if (numLabelVoxels && progressCount == progressCountMax)
                        {
                        progressCount = 0;
                        progress = (labelVoxelCount+0.0)/numLabelVoxels;
                        this->InvokeEvent(vtkCommand::ProgressEvent, (void *)&progress);
                        }
                      else
                        {
                        progressCount++;
                        }
                      // Now create a streamline
                      newStreamline=(vtkHyperStreamlineDTMRI *)
                        this->CreateHyperStreamline();

                      // Set its input information.
                      newStreamline->SetInputConnection(this->InputTensorFieldConnection);
                      newStreamline->SetStartPosition(point[0],point[1],point[2]);
                      //newStreamline->DebugOn();

                      // Ask it to output tensors and to only do one trajectory per start point
                      newStreamline->OutputTensorsOn();
                      newStreamline->OneTrajectoryPerSeedPointOn();

                      // Force it to execute
                      newStreamline->Update();

                      // See if we like it enough to add to the collection
                      // This relies on the fact that the step length is in units of
                      // length (unlike fractions of a cell in vtkHyperStreamline).
                      double length =
                        (newStreamline->GetOutput()->GetNumberOfPoints() - 1) *
                        newStreamline->GetIntegrationStepLength();

                      if (length > this->MinimumPathLength)
                        {
                        if (this->FileDirectoryName)
                          // write streamline to disk
                          {
                          if (this->FilePrefix == NULL)
                            {
                            this->SetFilePrefix("line");
                            }
                          // transform model
                          transformer->SetInputConnection(newStreamline->GetOutputPort());

                          // Save the model to disk
                          writer->SetInputConnection(transformer->GetOutputPort());
                          writer->SetFileType(2);

                          std::stringstream fileNameStr;
                          fileNameStr << FileDirectoryName << "/" << FilePrefix << '_' << idx << ".vtk";
                          writer->SetFileName(fileNameStr.str().c_str());
                          writer->Write();
                          newStreamline->Delete();
                          }
                        else
                          {
                          // keep the streamline in memory
                          this->Streamlines->AddItem((vtkObject *) newStreamline);
                          }
                        idx++;
                        }
                      else
                        {
                        newStreamline->Delete();
                        }
                    }
                }

            }

        }

    }

}




void vtkSeedTracts::SeedStreamlinesInROIWithMultipleValues()
{

  int numROIs;
  int initialROIValue = this->InputROIValue;

  if (this->InputMultipleROIValues == NULL)
    {
      vtkErrorMacro(<<"No values to seed from. SetInputMultipleROIValues before trying.");
      return;
    }

  numROIs=this->InputMultipleROIValues->GetNumberOfTuples();

  // test we have input
  if (this->InputROIConnection == NULL)
    {
      vtkErrorMacro("No ROI input.");
      return;
    }
  if (this->InputTensorFieldConnection == NULL)
    {
      vtkErrorMacro("No tensor data input.");
      return;
    }

  for (int i=0 ; i<numROIs ; i++)
    {
      this->InputROIValue = this->InputMultipleROIValues->GetValue(i);
      // check ROI's value of interest
      if (this->InputROIValue <= 0)
        {
          vtkErrorMacro("Input ROI value has not been set or is 0. (value is "  << this->InputROIValue << ". Trying next value");
          break;
        }
      this->SeedStreamlinesInROI();
    }

  //Restore InputROIValue variable
  this->InputROIValue = initialROIValue;
}

void vtkSeedTracts::TransformStreamlinesToRASAndAppendToPolyData(vtkPolyData *outFibers)
  {

  if (outFibers == NULL)
    {
    vtkErrorMacro("PolyData objects has not been allocated");
    return;
    }

  // Create transformation matrix to place actors in scene
  // This is used to transform the models before writing them to disk
  vtkNew<vtkTransform> transform;
  transform->SetMatrix(this->WorldToTensorScaledIJK->GetMatrix());
  transform->Inverse();

  vtkNew<vtkTransformPolyDataFilter> transformer;
  transformer->SetTransform(transform.GetPointer());

  vtkHyperStreamline *streamline;
  int npts = 0;
  int ncells = 0;
  //Loop through the collection and gather total number of points
  for (int i=0; i<this->Streamlines->GetNumberOfItems(); i++)
    {
    streamline = static_cast<vtkHyperStreamline*> (this->Streamlines->GetItemAsObject(i));
    npts += streamline->GetOutput()->GetNumberOfPoints();
    ncells += streamline->GetOutput()->GetNumberOfLines();
    }
  if (npts == 0 || ncells == 0)
    {
    return;
    }

  //Preallocate PolyData elements
  vtkNew<vtkPoints> points;
  outFibers->SetPoints(points.GetPointer());
  //outFibers->GetPoints()->SetNumberOfPoints(npts);

  vtkNew<vtkCellArray> outFibersCellArray;
  outFibers->SetLines(outFibersCellArray.GetPointer());
  outFibersCellArray->SetNumberOfCells(this->Streamlines->GetNumberOfItems());

  vtkIdTypeArray *cellArray = outFibersCellArray->GetData();
  cellArray->SetNumberOfTuples(npts+ncells);

  vtkNew<vtkFloatArray> newTensors;
  newTensors->SetNumberOfComponents(9);
  newTensors->Allocate(9*npts);
  outFibers->GetPointData()->SetTensors(newTensors.GetPointer());

  int ptId=0;
  int cellId=0;
  int ptOffset = 0;
  vtkIdType cellIndex;
  for (int i=0; i<this->Streamlines->GetNumberOfItems(); i++)
    {
    streamline = static_cast<vtkHyperStreamline*> (this->Streamlines->GetItemAsObject(i));
    transformer->SetInputConnection(streamline->GetOutputPort());
    transformer->Update();

    // Fill points and cells
    for (int k=0; k<transformer->GetOutput()->GetNumberOfPoints(); k++)
      {
      outFibers->GetPoints()->InsertNextPoint(transformer->GetOutput()->GetPoint(k));
      ptId++;
      }
    vtkIdTypeArray *cellArrayTransf = transformer->GetOutput()->GetLines()->GetData();
    cellIndex = cellArrayTransf->GetNumberOfTuples()-1;
    cellArray->SetTupleValue(cellId, &cellIndex);
    cellId++;
    for (int k=1; k<cellArrayTransf->GetNumberOfTuples(); k++)
      {
      //cellArray->InsertNextValue(ptOffset+cellArrayTransf->GetValue(k));
      cellIndex = ptOffset+cellArrayTransf->GetValue(k);
      cellArray->SetTupleValue(cellId, &cellIndex);
      cellId++;
      }
    ptOffset += transformer->GetOutput()->GetNumberOfPoints();

    // transform any tensors as well (rotate them)
    // this should be a vtk class but leave that for slicer3/vtk5
    // Here we rotate the tensors into the same (world) coordinate system.
    // -------------------------------------------------
    vtkDebugMacro("Rotating tensors");
    int numPts = transformer->GetOutput()->GetNumberOfPoints();

    vtkDebugMacro("Rotating tensors: init");
    double (*matrix)[4] = this->TensorRotationMatrix->Element;
    double tensor[9];
    double tensor3x3[3][3];
    double temp3x3[3][3];
    double matrix3x3[3][3];
    double matrixTranspose3x3[3][3];
    for (int row = 0; row < 3; row++)
      {
      for (int col = 0; col < 3; col++)
        {
          matrix3x3[row][col] = matrix[row][col];
          matrixTranspose3x3[row][col] = matrix[col][row];
        }
      }

     vtkDebugMacro("Rotating tensors: get tensors from probe");
     vtkDataArray *oldTensors = transformer->GetOutput()->GetPointData()->GetTensors();
     vtkDebugMacro("Rotating tensors: rotate");
     for (vtkIdType ii = 0; ii < numPts; ii++)
       {
         oldTensors->GetTuple(ii,tensor);
         int idx = 0;
         for (int row = 0; row < 3; row++)
           {
             for (int col = 0; col < 3; col++)
               {
                 tensor3x3[row][col] = tensor[idx];
                 idx++;
               }
           }
         // rotate by our matrix
         // R T R'
         vtkMath::Multiply3x3(matrix3x3,tensor3x3,temp3x3);
         vtkMath::Multiply3x3(temp3x3,matrixTranspose3x3,tensor3x3);
         idx =0;
         for (int row = 0; row < 3; row++)
           {
             for (int col = 0; col < 3; col++)
                {
                  tensor[idx] = tensor3x3[row][col];
                  idx++;
                }
            }
          newTensors->InsertNextTuple(tensor);
        }
    // End of tensor rotation code.
    // -------------------------------------------------
    }

  // Remove the scalars if any, we don't need
  // to save anything but the tensors
  outFibers->GetPointData()->SetScalars(NULL);
}


// seed in each voxel in the ROI, only keep paths that intersect the
// second ROI
//----------------------------------------------------------------------------
void vtkSeedTracts::SeedStreamlinesFromROIIntersectWithROI2()
{

  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  int inExt[6];
  double point[3], point2[3];

  //unsigned long target;
  short *inPtr;
  vtkHyperStreamlineDTMRI *newStreamline;

  // time
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();

  // test we have input
  if (this->InputROIConnection == NULL)
    {
      vtkErrorMacro("No ROI input.");
      return;
    }
  // make sure it is short type
  // TODO

  if (this->InputTensorFieldConnection == NULL)
    {
      vtkErrorMacro("No tensor data input.");
      return;
    }
  // test we have input
  if (this->InputROIConnection2 == NULL)
    {
      vtkErrorMacro("No ROI input.");
      return;
    }
  // make sure it is short type
  // TODO

  vtkNew<vtkTransform> transform;
  transform->SetMatrix(this->WorldToTensorScaledIJK->GetMatrix());
  transform->Inverse();

  vtkNew<vtkTransformPolyDataFilter> transformer;
  transformer->SetTransform(transform.GetPointer());

  vtkNew<vtkPolyDataWriter> writer;

  // Create transformation matrices to go backwards from streamline points to ROI space
  // This is used to access ROI2.
  vtkNew<vtkTransform> WorldToROI2;
  WorldToROI2->SetMatrix(this->ROI2ToWorld->GetMatrix());
  WorldToROI2->Inverse();
  vtkNew<vtkTransform> TensorScaledIJKToWorld;
  TensorScaledIJKToWorld->SetMatrix(this->WorldToTensorScaledIJK->GetMatrix());
  TensorScaledIJKToWorld->Inverse();

  // currently this filter is not multithreaded, though in the future
  // it could be (especially if it inherits from an image filter class)
  vtkInformation *inInfo = this->InputROIConnection->GetProducer()->GetOutputInformation(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  vtkImageData* inputROI = vtkImageData::SafeDownCast(this->InputROIConnection->GetProducer()->GetOutputDataObject(0));
  inputROI->GetContinuousIncrements(inExt, inIncX, inIncY, inIncZ);
  vtkImageData* inputROI2 = vtkImageData::SafeDownCast(this->InputROIConnection2->GetProducer()->GetOutputDataObject(0));

  // find the region to loop over
  maxX = inExt[1] - inExt[0];
  maxY = inExt[3] - inExt[2];
  maxZ = inExt[5] - inExt[4];

  //cout << "Dims: " << maxX << " " << maxY << " " << maxZ << endl;
  //cout << "Incr: " << inIncX << " " << inIncY << " " << inIncZ << endl;

  // for progress notification
  //target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  //target++;

  // start point in input integer field
  inPtr = (short *) inputROI->GetScalarPointerForExtent(inExt);

  // testing for seeding at a certain resolution.
  int increment = 1;

  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
      //for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      //for (idxY = 0; idxY <= maxY; idxY++)
      for (idxY = 0; idxY <= maxY; idxY += increment)
        {
          for (idxX = 0; idxX <= maxX; idxX += increment)
            {
              // if it is in the ROI/mask
              if (*inPtr == this->InputROIValue)
                {

                  // seed there and update
                  vtkDebugMacro( << "start streamline at: " << idxX << " " <<
                                 idxY << " " << idxZ);

                  // First transform to world space.
                  point[0]=idxX;
                  point[1]=idxY;
                  point[2]=idxZ;
                  this->ROIToWorld->TransformPoint(point,point2);
                  // Now transform to scaled ijk of the input tensors
                  this->WorldToTensorScaledIJK->TransformPoint(point2,point);

                  // make sure it is within the bounds of the tensor dataset
                  if (this->PointWithinTensorData(point,point2))
                    {
                      // Now create a streamline.
                      newStreamline=(vtkHyperStreamlineDTMRI *) this->CreateHyperStreamline();

                      // Set its input information.
                      newStreamline->SetInputConnection(this->InputTensorFieldConnection);
                      newStreamline->SetStartPosition(point[0],point[1],point[2]);

                      // Force it to update to access the path points
                      newStreamline->Update();

                      // for each point on the path, test
                      // the nearest voxel for path/ROI intersection.
                      vtkPoints * hs0
                        = newStreamline->GetOutput()->GetCell(0)->GetPoints();

                      int numPts=hs0->GetNumberOfPoints();
                      int ptidx=0;
                      int pt[3];
                      int intersects = 0;
                      while (ptidx < numPts)
                        {
                          hs0->GetPoint(ptidx,point);
                          // First transform to world space.
                          TensorScaledIJKToWorld->TransformPoint(point,point2);
                          // Now transform to ROI2 IJK space
                          WorldToROI2->TransformPoint(point2,point);
                          // Find that voxel number
                          pt[0]= (int) floor(point[0]+0.5);
                          pt[1]= (int) floor(point[1]+0.5);
                          pt[2]= (int) floor(point[2]+0.5);
                          short *tmp = (short *) inputROI2->GetScalarPointer(pt);
                          if (tmp != NULL)
                            {
                              if (*tmp == this->InputROI2Value) {
                                intersects = 1;
                              }
                            }
                          ptidx++;
                        }

                      vtkPoints * hs1
                        = newStreamline->GetOutput()->GetCell(1)->GetPoints();
                      numPts=hs1->GetNumberOfPoints();
                      // Skip the first point in the second line since it
                      // is a duplicate of the initial point.
                      ptidx=1;
                      while (ptidx < numPts)
                        {
                          hs1->GetPoint(ptidx,point);
                          // First transform to world space.
                          TensorScaledIJKToWorld->TransformPoint(point,point2);
                          // Now transform to ROI IJK space
                          WorldToROI2->TransformPoint(point2,point);
                          // Find that voxel number
                          pt[0]= (int) floor(point[0]+0.5);
                          pt[1]= (int) floor(point[1]+0.5);
                          pt[2]= (int) floor(point[2]+0.5);
                          short *tmp = (short *) inputROI2->GetScalarPointer(pt);
                          if (tmp != NULL)
                            {
                              if (*tmp == this->InputROI2Value) {
                                intersects = 1;
                              }
                            }
                          ptidx++;
                        }

                      // if it intersects with some ROI, then
                      // display it, otherwise delete it.
                      if (intersects)
                        {
                        if (this->FileDirectoryName)
                          {
                          if (this->FilePrefix == NULL)
                            {
                            this->SetFilePrefix("line");
                            }
                          // transform model
                          transformer->SetInputConnection(newStreamline->GetOutputPort());
                          
                          // Save the model to disk
                          writer->SetInputConnection(transformer->GetOutputPort());
                          writer->SetFileType(2);

                          std::stringstream fileNameStr;
                          fileNameStr << FileDirectoryName << "/" << FilePrefix << '_' << this->Streamlines->GetNumberOfItems()-1 << ".vtk";
                          writer->SetFileName(fileNameStr.str().c_str());
                          writer->Write();
                          }
                        else
                          {
                          this->Streamlines->AddItem((vtkObject *)newStreamline);
                          newStreamline->Delete();
                          }
                        }
                      else
                        {
                          newStreamline->Delete();
                        }

                    } // end if inside tensor field

                } // end if in ROI

              //inPtr++;
              inPtr += increment;

              inPtr += inIncX;
            }
          //inPtr += inIncY;
          inPtr += inIncY*increment;
        }
      inPtr += inIncZ;
    }

  timer->StopTimer();
  std::cout << "Tractography in ROI time: " << timer->GetElapsedTime() << endl;
}

//----------------------------------------------------------------------------
void vtkSeedTracts::DeleteAllStreamlines()
{
  int numStreamlines, i;

  i=0;
  numStreamlines = this->Streamlines->GetNumberOfItems();
  while (i < numStreamlines)
    {
      vtkDebugMacro( << "Deleting streamline " << i);
      // always delete the first streamline from the collections
      // (they change size as we do this, shrinking away)
      this->DeleteStreamline(0);
      i++;
    }

}

// Delete one streamline and all of its associated objects.
// Here we delete the actual vtkHyperStreamline subclass object.
// We call helper class DeleteStreamline functions in order
// to get rid of (for example) graphics display objects.
//----------------------------------------------------------------------------
void vtkSeedTracts::DeleteStreamline(int index)
{
  vtkHyperStreamline *currStreamline;

  // Delete actual streamline
  vtkDebugMacro( << "Delete stream" );
  currStreamline = vtkHyperStreamline::SafeDownCast(this->Streamlines->GetItemAsObject(index));
  if (currStreamline != NULL)
    {
      this->Streamlines->RemoveItem(index);
      currStreamline->Delete();
    }

  vtkDebugMacro( << "Done deleting streamline");

}
