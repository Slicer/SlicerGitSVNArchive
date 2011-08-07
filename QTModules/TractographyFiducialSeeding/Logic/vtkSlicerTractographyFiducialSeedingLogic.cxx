/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkSlicerTractographyFiducialSeedingLogic.cxx,v $
  Date:      $Date: 2006/01/06 17:56:48 $
  Version:   $Revision: 1.58 $

=========================================================================auto=*/

#include "vtkSlicerTractographyFiducialSeedingLogic.h"

// MRML includes
#include <vtkMRMLFiberBundleDisplayNode.h>
#include <vtkMRMLDiffusionTensorVolumeNode.h>
#include <vtkMRMLAnnotationHierarchyNode.h>
#include <vtkMRMLAnnotationControlPointsNode.h>
#include <vtkMRMLFiberBundleNode.h>
#include <vtkMRMLFiberBundleStorageNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkSeedTracts.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkMaskPoints.h>
#include <vtkImageChangeInformation.h>
#include <vtkSmartPointer.h>
#include "vtkMRMLTractographyFiducialSeedingNode.h"

// ITKSYS includes

// STD includes

vtkCxxRevisionMacro(vtkSlicerTractographyFiducialSeedingLogic, "$Revision: 1.9.12.1 $");
vtkStandardNewMacro(vtkSlicerTractographyFiducialSeedingLogic);

//----------------------------------------------------------------------------
vtkSlicerTractographyFiducialSeedingLogic::vtkSlicerTractographyFiducialSeedingLogic()
{
  this->MaskPoints = vtkMaskPoints::New();
  this->TractographyFiducialSeedingNode = NULL;
  this->DiffusionTensorVolumeNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerTractographyFiducialSeedingLogic::~vtkSlicerTractographyFiducialSeedingLogic()
{
  this->MaskPoints->Delete();
  this->RemoveMRMLNodesObservers();
  vtkSetAndObserveMRMLNodeMacro(this->TractographyFiducialSeedingNode, NULL);
  vtkSetAndObserveMRMLNodeMacro(this->DiffusionTensorVolumeNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerTractographyFiducialSeedingLogic::RemoveMRMLNodesObservers()
{
  for (unsigned int i=0; i<this->ObservedNodes.size(); i++)
    {
    vtkSetAndObserveMRMLNodeMacro(this->ObservedNodes[i], NULL);
    }
  this->ObservedNodes.clear();
}

//----------------------------------------------------------------------------
void vtkSlicerTractographyFiducialSeedingLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
void vtkSlicerTractographyFiducialSeedingLogic::SetAndObserveTractographyFiducialSeedingNode(vtkMRMLTractographyFiducialSeedingNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->TractographyFiducialSeedingNode, node);

  this->RemoveMRMLNodesObservers();

  this->AddMRMLNodesObservers();

  return;
}

//----------------------------------------------------------------------------
void vtkSlicerTractographyFiducialSeedingLogic::AddMRMLNodesObservers()
{
  if (this->TractographyFiducialSeedingNode)
    {
    vtkMRMLDiffusionTensorVolumeNode *dtiNode = vtkMRMLDiffusionTensorVolumeNode::SafeDownCast(
          this->GetMRMLScene()->GetNodeByID(this->TractographyFiducialSeedingNode->GetInputVolumeRef()));
    vtkSetAndObserveMRMLNodeMacro(this->DiffusionTensorVolumeNode, dtiNode);

    vtkMRMLNode *seedinNode = this->GetMRMLScene()->GetNodeByID(this->TractographyFiducialSeedingNode->GetInputFiducialRef());

    vtkMRMLAnnotationHierarchyNode *annotationHierarchyNode = vtkMRMLAnnotationHierarchyNode::SafeDownCast(seedinNode);
    vtkMRMLTransformableNode *transformableNode = vtkMRMLTransformableNode::SafeDownCast(seedinNode);

    if (annotationHierarchyNode)
      {
      this->ObservedNodes.push_back(NULL);
      vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
      events->InsertNextValue ( vtkMRMLHierarchyNode::ChildNodeAddedEvent );
      events->InsertNextValue ( vtkMRMLHierarchyNode::ChildNodeRemovedEvent );
      vtkSetAndObserveMRMLNodeEventsMacro(this->ObservedNodes[this->ObservedNodes.size()-1], 
                                          annotationHierarchyNode, events);

      vtkCollection *annotationNodes = vtkCollection::New();
      annotationHierarchyNode->GetDirectChildren(annotationNodes);
      int nf = annotationNodes->GetNumberOfItems();
      for (int f=0; f<nf; f++)
        {
        vtkMRMLAnnotationControlPointsNode *annotationNode = vtkMRMLAnnotationControlPointsNode::SafeDownCast(
                                                      annotationNodes->GetItemAsObject(f));   
        if (annotationNode)
          {
          this->ObservedNodes.push_back(NULL);
          vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
          events->InsertNextValue ( vtkMRMLTransformableNode::TransformModifiedEvent );
          events->InsertNextValue ( vtkMRMLModelNode::PolyDataModifiedEvent );
          events->InsertNextValue ( vtkCommand::ModifiedEvent );
          vtkSetAndObserveMRMLNodeEventsMacro(this->ObservedNodes[this->ObservedNodes.size()-1], 
                                              annotationNode, events);
          }
        }
      annotationNodes->Delete();
      }
    else if (transformableNode)
      {
      this->ObservedNodes.push_back(NULL);
      vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
      events->InsertNextValue ( vtkMRMLTransformableNode::TransformModifiedEvent );
      events->InsertNextValue ( vtkMRMLModelNode::PolyDataModifiedEvent );
      events->InsertNextValue ( vtkCommand::ModifiedEvent );
      vtkSetAndObserveMRMLNodeEventsMacro(this->ObservedNodes[this->ObservedNodes.size()-1], 
                                          transformableNode, events);
      }
    }
  else
    {
    vtkSetAndObserveMRMLNodeMacro(this->DiffusionTensorVolumeNode, NULL);
    }
  return;
}


//----------------------------------------------------------------------------
void vtkSlicerTractographyFiducialSeedingLogic::CreateTractsForOneSeed(vtkSeedTracts *seed,
                                                            vtkMRMLDiffusionTensorVolumeNode *volumeNode,
                                                            vtkMRMLTransformableNode *transformableNode,
                                                            int stoppingMode, 
                                                            double stoppingValue, 
                                                            double stoppingCurvature, 
                                                            double integrationStepLength,
                                                            double mnimumPathLength,
                                                            double regionSize, double sampleStep,
                                                            int maxNumberOfSeeds,
                                                            int seedSelectedFiducials)
{
  double sp[3];
  volumeNode->GetSpacing(sp);

  //2. Set Up matrices
  vtkMRMLTransformNode* vxformNode = volumeNode->GetParentTransformNode();
  vtkMRMLTransformNode* fxformNode = transformableNode->GetParentTransformNode();
  vtkMatrix4x4* transformVolumeToFiducial = vtkMatrix4x4::New();
  transformVolumeToFiducial->Identity();
  if (fxformNode != NULL )
    {
    fxformNode->GetMatrixTransformToNode(vxformNode, transformVolumeToFiducial);
    } 
  else if (vxformNode != NULL ) 
    {
    vxformNode->GetMatrixTransformToNode(fxformNode, transformVolumeToFiducial);
    transformVolumeToFiducial->Invert();
    }
  vtkTransform *transFiducial = vtkTransform::New();
  transFiducial->Identity();
  transFiducial->PreMultiply();
  transFiducial->SetMatrix(transformVolumeToFiducial);

  vtkMatrix4x4 *mat = vtkMatrix4x4::New();
  volumeNode->GetRASToIJKMatrix(mat);
  
  vtkMatrix4x4 *TensorRASToIJK = vtkMatrix4x4::New();
  TensorRASToIJK->DeepCopy(mat);
  mat->Delete();


  vtkTransform *trans = vtkTransform::New();
  trans->Identity();
  trans->PreMultiply();
  trans->SetMatrix(TensorRASToIJK);
  // Trans from IJK to RAS
  trans->Inverse();
  // Take into account spacing to compute Scaled IJK
  trans->Scale(1/sp[0],1/sp[1],1/sp[2]);
  trans->Inverse();
  
  //Set Transformation to seeding class
  seed->SetWorldToTensorScaledIJK(trans);
  
  vtkMatrix4x4 *TensorRASToIJKRotation = vtkMatrix4x4::New();
  TensorRASToIJKRotation->DeepCopy(TensorRASToIJK);
  
  //Set Translation to zero
  for (int i=0;i<3;i++)
    {
    TensorRASToIJKRotation->SetElement(i,3,0);
    }
  //Remove scaling in rasToIjk to make a real roation matrix
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

  //ROI comes from tensor, IJKToRAS is the same
  // as the tensor
  vtkTransform *trans2 = vtkTransform::New();
  trans2->Identity();
  trans2->SetMatrix(TensorRASToIJK);
  trans2->Inverse();
  seed->SetROIToWorld(trans2);
  
  seed->UseVtkHyperStreamlinePoints();
  vtkHyperStreamlineDTMRI *streamer=vtkHyperStreamlineDTMRI::New();
  seed->SetVtkHyperStreamlinePointsSettings(streamer);
  seed->SetMinimumPathLength(mnimumPathLength);

  if ( stoppingMode == 0 )
    {
     streamer->SetStoppingModeToLinearMeasure();
    }
  else
    {  
    streamer->SetStoppingModeToFractionalAnisotropy();
    }

  //streamer->SetMaximumPropagationDistance(this->MaximumPropagationDistance);
  streamer->SetStoppingThreshold(stoppingValue);
  streamer->SetRadiusOfCurvature(stoppingCurvature);
  streamer->SetIntegrationStepLength(integrationStepLength);

  // Temp fix to provide a scalar
  seed->GetInputTensorField()->GetPointData()->SetScalars(volumeNode->GetImageData()->GetPointData()->GetScalars());
  
  vtkMRMLAnnotationControlPointsNode *annotationNode = vtkMRMLAnnotationControlPointsNode::SafeDownCast(transformableNode);
  vtkMRMLModelNode *modelNode = vtkMRMLModelNode::SafeDownCast(transformableNode);


  // if annotation
  if (annotationNode && annotationNode->GetNumberOfControlPoints() &&
     (!seedSelectedFiducials || (seedSelectedFiducials && annotationNode->GetSelected())) ) 
    {
    for (int i=0; i < annotationNode->GetNumberOfControlPoints(); i++)
      {
      double *xyzf = annotationNode->GetControlPointCoordinates(i);
      for (double x = -regionSize/2.0; x <= regionSize/2.0; x+=sampleStep)
        {
        for (double y = -regionSize/2.0; y <= regionSize/2.0; y+=sampleStep)
          {
          for (double z = -regionSize/2.0; z <= regionSize/2.0; z+=sampleStep)
            {
            float newXYZ[3];
            newXYZ[0] = xyzf[0] + x;
            newXYZ[1] = xyzf[1] + y;
            newXYZ[2] = xyzf[2] + z;
            float *xyz = transFiducial->TransformFloatPoint(newXYZ);
            //Run the thing
            seed->SeedStreamlineFromPoint(xyz[0], xyz[1], xyz[2]);
            }
          }
        }
      }
    }
  else if (modelNode) 
    {
    this->MaskPoints->SetInput(modelNode->GetPolyData());
    this->MaskPoints->SetRandomMode(1);
    this->MaskPoints->SetMaximumNumberOfPoints(maxNumberOfSeeds);
    this->MaskPoints->Update();
    vtkPolyData *mpoly = this->MaskPoints->GetOutput();

    int nf = mpoly->GetNumberOfPoints();
    for (int f=0; f<nf; f++)
      {
      double *xyzf = mpoly->GetPoint(f);

      double *xyz = transFiducial->TransformDoublePoint(xyzf);
           
      //Run the thing
      seed->SeedStreamlineFromPoint(xyz[0], xyz[1], xyz[2]);
      }
    }

  // Delete everything: Still trying to figure out what is going on

  TensorRASToIJK->Delete();
  TensorRASToIJKRotation->Delete();
  trans2->Delete();
  trans->Delete();
  streamer->Delete();
  transformVolumeToFiducial->Delete();
  transFiducial->Delete();

}




//----------------------------------------------------------------------------
int vtkSlicerTractographyFiducialSeedingLogic::CreateTracts(vtkMRMLDiffusionTensorVolumeNode *volumeNode,
                                                            vtkMRMLNode *seedingyNode,
                                                            vtkMRMLFiberBundleNode *fiberNode,
                                                            int stoppingMode, 
                                                            double stoppingValue, 
                                                            double stoppingCurvature, 
                                                            double integrationStepLength,
                                                            double mnimumPathLength,
                                                            double regionSize, double sampleStep,
                                                            int maxNumberOfSeeds,
                                                            int seedSelectedFiducials,
                                                            int displayMode)
{
  // 0. check inputs
  if (volumeNode == NULL || seedingyNode == NULL || fiberNode == NULL ||
      volumeNode->GetImageData() == NULL)
    {
    if (fiberNode && fiberNode->GetPolyData())
      {
      fiberNode->GetPolyData()->Reset();
      }
    return 0;
    }
    
  vtkPolyData *oldPoly = fiberNode->GetPolyData();
   
  vtkSeedTracts *seed = vtkSeedTracts::New();
  
  //1. Set Input
  
  //Do scale IJK
  double sp[3];
  volumeNode->GetSpacing(sp);
  vtkImageChangeInformation *ici = vtkImageChangeInformation::New();
  ici->SetOutputSpacing(sp);
  ici->SetInput(volumeNode->GetImageData());
  ici->GetOutput()->Update();

  seed->SetInputTensorField(ici->GetOutput());

  vtkMRMLTransformNode* vxformNode = volumeNode->GetParentTransformNode();

  vtkMRMLAnnotationHierarchyNode *annotationListNode = vtkMRMLAnnotationHierarchyNode::SafeDownCast(seedingyNode);
  vtkMRMLAnnotationControlPointsNode *annotationNode = vtkMRMLAnnotationControlPointsNode::SafeDownCast(seedingyNode);
  vtkMRMLModelNode *modelNode = vtkMRMLModelNode::SafeDownCast(seedingyNode);

  if (annotationListNode) // loop over annotation nodes
    {
    vtkCollection *annotationNodes = vtkCollection::New();
    annotationListNode->GetDirectChildren(annotationNodes);
    int nf = annotationNodes->GetNumberOfItems();
    for (int f=0; f<nf; f++)
      {
      vtkMRMLAnnotationControlPointsNode *annotationNode = vtkMRMLAnnotationControlPointsNode::SafeDownCast(
                                                    annotationNodes->GetItemAsObject(f));
      if (!annotationNode || (seedSelectedFiducials && !annotationNode->GetSelected()))
        {
        continue;
        }


      this->CreateTractsForOneSeed(seed, volumeNode, annotationNode, 
                                   stoppingMode, stoppingValue, stoppingCurvature, 
                                   integrationStepLength, mnimumPathLength, regionSize, 
                                   sampleStep, maxNumberOfSeeds, seedSelectedFiducials);

      }
      annotationNodes->Delete();
    }
  else if (annotationNode) // loop over points in the models
    {
    this->CreateTractsForOneSeed(seed, volumeNode, annotationNode,
                                 stoppingMode, stoppingValue, stoppingCurvature, 
                                 integrationStepLength, mnimumPathLength, regionSize, 
                                 sampleStep, maxNumberOfSeeds, seedSelectedFiducials);

    }

  else if (modelNode) // loop over points in the models
    {
    this->CreateTractsForOneSeed(seed, volumeNode, modelNode,
                                 stoppingMode, stoppingValue, stoppingCurvature, 
                                 integrationStepLength, mnimumPathLength, regionSize, 
                                 sampleStep, maxNumberOfSeeds, seedSelectedFiducials);

    }

    
  //6. Extra5ct PolyData in RAS
  vtkPolyData *outFibers = vtkPolyData::New();
  
  seed->TransformStreamlinesToRASAndAppendToPolyData(outFibers);

  fiberNode->SetAndObservePolyData(outFibers);
  
  int newNode = 0;
  vtkMRMLFiberBundleDisplayNode *dnode = fiberNode->GetTubeDisplayNode();
  if (dnode == NULL || oldPoly == NULL)
    {
    dnode = fiberNode->AddTubeDisplayNode();
    dnode->DisableModifiedEventOn();
    dnode->SetScalarVisibility(1);
    dnode->SetOpacity(1);
    dnode->SetVisibility(1);
    dnode->DisableModifiedEventOff();
    newNode = 1;
    }
  if (displayMode == 1)
    {
    dnode->SetVisibility(1);
    }
  else
    {
    dnode->SetVisibility(0);
    }

  dnode = fiberNode->GetLineDisplayNode();
  if (dnode == NULL || oldPoly == NULL)
    {
    dnode = fiberNode->AddLineDisplayNode();
    if (newNode)
      {
      dnode->DisableModifiedEventOn();
      dnode->SetVisibility(0);
      dnode->SetOpacity(1);
      dnode->SetScalarVisibility(0);
      dnode->DisableModifiedEventOff();
      }
    }
  if (displayMode == 0)
    {
    dnode->SetVisibility(1);
    }
  else
    {
    dnode->SetVisibility(0);
    }

  dnode = fiberNode->GetGlyphDisplayNode();
  if (dnode == NULL || oldPoly == NULL)
    {
    dnode = fiberNode->AddGlyphDisplayNode();
    if (newNode)
      {
      dnode->DisableModifiedEventOn();
      dnode->SetVisibility(0);
      dnode->SetScalarVisibility(0);
      dnode->SetOpacity(1);
      dnode->DisableModifiedEventOff();
      }
    }


  if (fiberNode->GetStorageNode() == NULL) 
    {
    vtkMRMLFiberBundleStorageNode *storageNode = vtkMRMLFiberBundleStorageNode::New();
    fiberNode->GetScene()->AddNodeNoNotify(storageNode);
    fiberNode->SetAndObserveStorageNodeID(storageNode->GetID());
    storageNode->Delete();
    }

  fiberNode->InvokeEvent(vtkMRMLFiberBundleNode::PolyDataModifiedEvent, NULL);
 
  if (vxformNode != NULL )
    { 
    fiberNode->SetAndObserveTransformNodeID(vxformNode->GetID());
    } 
  else 
    {
    fiberNode->SetAndObserveTransformNodeID(NULL);
    }

  fiberNode->SetModifiedSinceRead(1);

  // Delete everything: Still trying to figure out what is going on
  outFibers->Delete();
  ici->Delete();
  seed->Delete();
  return 1;
}


//---------------------------------------------------------------------------
void vtkSlicerTractographyFiducialSeedingLogic::ProcessMRMLEvents(vtkObject *vtkNotUsed(caller),
                                                                  unsigned long vtkNotUsed(event),
                                                                  void *vtkNotUsed(callData)) 
{
  // if parameter node has been added, update GUI widgets with new values
  vtkMRMLTractographyFiducialSeedingNode* snode = this->TractographyFiducialSeedingNode;
  if (snode == NULL || snode->GetEnableSeeding() == 0)
    {
    return;
    }

  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
    {
    return;
    }

  this->RemoveMRMLNodesObservers();

  this->AddMRMLNodesObservers();

  vtkMRMLDiffusionTensorVolumeNode *volumeNode = 
        vtkMRMLDiffusionTensorVolumeNode::SafeDownCast(scene->GetNodeByID(snode->GetInputVolumeRef()));
  vtkMRMLNode *seedingNode = scene->GetNodeByID(snode->GetInputFiducialRef());
  vtkMRMLFiberBundleNode *fiberNode = 
        vtkMRMLFiberBundleNode::SafeDownCast(scene->GetNodeByID(snode->GetOutputFiberRef()));

  if(volumeNode == NULL || seedingNode == NULL || fiberNode == NULL) 
    {
    return;
    }
  
  this->CreateTracts(volumeNode, seedingNode, fiberNode,
                     snode->GetStoppingMode(), 
                     snode->GetStoppingValue(),
                     snode->GetStoppingCurvature(),
                     snode->GetIntegrationStep(),
                     snode->GetMinimumPathLength(),
                     snode->GetSeedingRegionSize(),
                     snode->GetSeedingRegionStep(),
                     snode->GetMaxNumberOfSeeds(),
                     snode->GetSeedSelectedFiducials(),
                     snode->GetDisplayMode()
                     );
  return;
                                                            
}

//----------------------------------------------------------------------------
void vtkSlicerTractographyFiducialSeedingLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
    {
    return;
    }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLTractographyFiducialSeedingNode>::New());
}

