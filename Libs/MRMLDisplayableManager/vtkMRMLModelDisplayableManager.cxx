/*=========================================================================

  Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Base/GUI/vtkMRMLModelDisplayableManager.cxx $
  Date:      $Date: 2010-05-27 14:32:23 -0400 (Thu, 27 May 2010) $
  Version:   $Revision: 13525 $

==========================================================================*/

// MRMLLogic includes

// MRMLDisplayableManager includes
#include "vtkMRMLModelDisplayableManager.h"
#include "vtkThreeDViewInteractorStyle.h"

// MRML includes
#include <vtkEventBroker.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLProceduralColorNode.h>

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkCellArray.h>
#include <vtkImplicitBoolean.h>
#include <vtkPlane.h>
#include <vtkClipPolyData.h>
#include <vtkLookupTable.h>

// for picking
#include <vtkWorldPointPicker.h>
#include <vtkPropPicker.h>
#include <vtkCellPicker.h>
#include <vtkPointPicker.h>
#include <vtkRendererCollection.h>

// STD includes
#include <cassert>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLModelDisplayableManager );
vtkCxxRevisionMacro ( vtkMRMLModelDisplayableManager, "$Revision: 13525 $");

//---------------------------------------------------------------------------
class vtkMRMLModelDisplayableManager::vtkInternal
{
public:
  vtkInternal();
  ~vtkInternal();

  void CreateClipSlices();

  /// Reset all the pick vars
  void ResetPick();

  std::map<std::string, vtkProp3D *>               DisplayedActors;
  std::map<std::string, vtkMRMLDisplayNode *>      DisplayedNodes;
  std::map<std::string, int>                       DisplayedClipState;
  std::map<std::string, int>                       DisplayedVisibility;
  std::map<std::string, vtkMRMLDisplayableNode *>  DisplayableNodes;
  std::map<std::string, int>                       RegisteredModelHierarchies;

  vtkMRMLSliceNode *   RedSliceNode;
  vtkMRMLSliceNode *   GreenSliceNode;
  vtkMRMLSliceNode *   YellowSliceNode;

  vtkSmartPointer<vtkImplicitBoolean> SlicePlanes;
  vtkSmartPointer<vtkPlane>           RedSlicePlane;
  vtkSmartPointer<vtkPlane>           GreenSlicePlane;
  vtkSmartPointer<vtkPlane>           YellowSlicePlane;

  vtkMRMLClipModelsNode * ClipModelsNode;
  int                     ClipType;
  int                     RedSliceClipState;
  int                     YellowSliceClipState;
  int                     GreenSliceClipState;
  bool                    ClippingOn;

  bool                         ModelHierarchiesPresent;
  vtkMRMLModelHierarchyLogic * ModelHierarchyLogic;

  vtkSmartPointer<vtkWorldPointPicker> WorldPointPicker;
  vtkSmartPointer<vtkPropPicker>       PropPicker;
  vtkSmartPointer<vtkCellPicker>       CellPicker;
  vtkSmartPointer<vtkPointPicker>      PointPicker;

  /// Information about a pick event
  std::string  PickedNodeName;
  double       PickedRAS[3];
  vtkIdType    PickedCellID;
  vtkIdType    PickedPointID;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLModelDisplayableManager::vtkInternal::vtkInternal()
{
  this->ClipModelsNode = 0;
  this->RedSliceNode = 0;
  this->GreenSliceNode = 0;
  this->YellowSliceNode = 0;

  this->ModelHierarchiesPresent = false;
  this->ModelHierarchyLogic = vtkMRMLModelHierarchyLogic::New();

  // Instantiate and initialize Pickers
  this->WorldPointPicker = vtkSmartPointer<vtkWorldPointPicker>::New();
  this->PropPicker = vtkSmartPointer<vtkPropPicker>::New();
  this->CellPicker = vtkSmartPointer<vtkCellPicker>::New();
  this->CellPicker->SetTolerance(0.00001);
  this->PointPicker = vtkSmartPointer<vtkPointPicker>::New();
  this->ResetPick();
}

//---------------------------------------------------------------------------
vtkMRMLModelDisplayableManager::vtkInternal::~vtkInternal()
{

}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::vtkInternal::CreateClipSlices()
{
  this->SlicePlanes = vtkSmartPointer<vtkImplicitBoolean>::New();
  this->SlicePlanes->SetOperationTypeToIntersection();

  this->RedSlicePlane = vtkSmartPointer<vtkPlane>::New();
  this->GreenSlicePlane = vtkSmartPointer<vtkPlane>::New();
  this->YellowSlicePlane = vtkSmartPointer<vtkPlane>::New();

  this->ClipType = vtkMRMLClipModelsNode::ClipIntersection;

  this->RedSliceClipState = vtkMRMLClipModelsNode::ClipOff;
  this->YellowSliceClipState = vtkMRMLClipModelsNode::ClipOff;
  this->GreenSliceClipState = vtkMRMLClipModelsNode::ClipOff;

  this->ClippingOn = false;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::vtkInternal::ResetPick()
{
  this->PickedNodeName.clear();
  for (int i=0; i < 3; i++)
    {
    this->PickedRAS[i] = 0.0;
    }
  this->PickedCellID = -1;
  this->PickedPointID = -1;
}

//---------------------------------------------------------------------------
// vtkMRMLModelDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLModelDisplayableManager::vtkMRMLModelDisplayableManager()
{
  this->Internal = new vtkInternal();

  this->Internal->CreateClipSlices();
}

//---------------------------------------------------------------------------
vtkMRMLModelDisplayableManager::~vtkMRMLModelDisplayableManager()
{
  this->SetModelHierarchyLogic(0);

  vtkSetMRMLNodeMacro(this->Internal->ClipModelsNode, 0);
  vtkSetMRMLNodeMacro(this->Internal->RedSliceNode, 0);
  vtkSetMRMLNodeMacro(this->Internal->GreenSliceNode, 0);
  vtkSetMRMLNodeMacro(this->Internal->YellowSliceNode, 0);

  // release the DisplayedModelActors
  this->Internal->DisplayedActors.clear();

  if (this->Internal->ModelHierarchyLogic)
    {
    this->Internal->ModelHierarchyLogic->Delete();
    }

  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::PrintSelf ( ostream& os, vtkIndent indent )
{
  this->vtkObject::PrintSelf ( os, indent );

  os << indent << "vtkMRMLModelDisplayableManager: " << this->GetClassName() << "\n";
    
  os << indent << "ClipType = " << this->Internal->ClipType << "\n";
  os << indent << "RedSliceClipState = " << this->Internal->RedSliceClipState << "\n";
  os << indent << "YellowSliceClipState = " << this->Internal->YellowSliceClipState << "\n";
  os << indent << "GreenSliceClipState = " << this->Internal->GreenSliceClipState << "\n";
  os << indent << "ClippingOn = " << (this->Internal->ClippingOn ? "true" : "false") << "\n";

  os << indent << "ModelHierarchiesPresent = " << this->Internal->ModelHierarchiesPresent << "\n";
    
  os << indent << "PickedNodeName = " << this->Internal->PickedNodeName.c_str() << "\n";
  os << indent << "PickedRAS = (" << this->Internal->PickedRAS[0] << ", "
      << this->Internal->PickedRAS[1] << ", "<< this->Internal->PickedRAS[2] << ")\n";
  os << indent << "PickedCellID = " << this->Internal->PickedCellID << "\n";
  os << indent << "PickedPointID = " << this->Internal->PickedPointID << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::AdditionnalInitializeStep()
{
  vtkRenderWindowInteractor * interactor = this->GetInteractor();
  if (interactor)
    {
    vtkThreeDViewInteractorStyle * interactorStyle =
        vtkThreeDViewInteractorStyle::SafeDownCast(interactor->GetInteractorStyle());
    if (interactorStyle)
      {
      interactorStyle->SetModelDisplayableManager(this);
      }
    }
}

//---------------------------------------------------------------------------
vtkMRMLClipModelsNode* vtkMRMLModelDisplayableManager::GetClipModelsNode()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): "
                << "returning Internal->ClipModelsNode of "
                << this->Internal->ClipModelsNode);
  return this->Internal->ClipModelsNode;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::SetClipModelsNode(vtkMRMLClipModelsNode *snode)
{
  vtkSetAndObserveMRMLNodeMacro(this->Internal->ClipModelsNode, snode);
}

//---------------------------------------------------------------------------
int vtkMRMLModelDisplayableManager::UpdateClipSlicesFromMRML()
{
  if (this->GetMRMLScene() == 0)
    {
    return 0;
    }

  // update ClipModels node
  vtkMRMLClipModelsNode *clipNode = vtkMRMLClipModelsNode::SafeDownCast(this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLClipModelsNode"));
  if (clipNode != this->Internal->ClipModelsNode)
    {
    vtkSetAndObserveMRMLNodeMacro(this->Internal->ClipModelsNode, clipNode);
    }

  if (this->Internal->ClipModelsNode == 0)
    {
    return 0;
    }

  // update Slice nodes
  vtkMRMLSliceNode *node= 0;
  vtkMRMLSliceNode *nodeRed= 0;
  vtkMRMLSliceNode *nodeGreen= 0;
  vtkMRMLSliceNode *nodeYellow= 0;
  
  std::vector<vtkMRMLNode *> snodes;
  int nnodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLSliceNode", snodes);
  for (int n=0; n<nnodes; n++)
    {
    node = vtkMRMLSliceNode::SafeDownCast (snodes[n]);
    // TODO use perhaps SliceLogic to get the name instead of "Red" etc.
    if (!strcmp(node->GetLayoutName(), "Red"))
      {
      nodeRed = node;
      }
    else if (!strcmp(node->GetLayoutName(), "Green"))
      {
      nodeGreen = node;
      }
    else if (!strcmp(node->GetLayoutName(), "Yellow"))
      {
      nodeYellow = node;
      }
    node = 0;
    }

  if (nodeRed != this->Internal->RedSliceNode)
    {
    vtkSetAndObserveMRMLNodeMacro(this->Internal->RedSliceNode, nodeRed);
    }
  if (nodeGreen != this->Internal->GreenSliceNode)
    {
    vtkSetAndObserveMRMLNodeMacro(this->Internal->GreenSliceNode, nodeGreen);
    }
  if (nodeYellow != this->Internal->YellowSliceNode)
    {
    vtkSetAndObserveMRMLNodeMacro(this->Internal->YellowSliceNode, nodeYellow);
    }

  if (this->Internal->RedSliceNode == 0 ||
      this->Internal->GreenSliceNode == 0 ||
      this->Internal->YellowSliceNode == 0)
    {
    return 0;
    }

  int modifiedState = 0;

  if ( this->Internal->ClipModelsNode->GetClipType() != this->Internal->ClipType)
    {
    modifiedState = 1;
    this->Internal->ClipType = this->Internal->ClipModelsNode->GetClipType();
    if (this->Internal->ClipType == vtkMRMLClipModelsNode::ClipIntersection)
      {
      this->Internal->SlicePlanes->SetOperationTypeToIntersection();
      }
    else if (this->Internal->ClipType == vtkMRMLClipModelsNode::ClipUnion)
      {
      this->Internal->SlicePlanes->SetOperationTypeToUnion();
      }
    else 
      {
      vtkErrorMacro("vtkMRMLClipModelsNode:: Invalid Clip Type");
      }
    }

  if (this->Internal->ClipModelsNode->GetRedSliceClipState() != this->Internal->RedSliceClipState)
    {
    if (this->Internal->RedSliceClipState == vtkMRMLClipModelsNode::ClipOff)
      {
      this->Internal->SlicePlanes->AddFunction(this->Internal->RedSlicePlane);
      }
    else if (this->Internal->ClipModelsNode->GetRedSliceClipState() == vtkMRMLClipModelsNode::ClipOff)
      {
      this->Internal->SlicePlanes->RemoveFunction(this->Internal->RedSlicePlane);
      }
    modifiedState = 1;
    this->Internal->RedSliceClipState = this->Internal->ClipModelsNode->GetRedSliceClipState();
    }

  if (this->Internal->ClipModelsNode->GetGreenSliceClipState() != this->Internal->GreenSliceClipState)
    {
    if (this->Internal->GreenSliceClipState == vtkMRMLClipModelsNode::ClipOff)
      {
      this->Internal->SlicePlanes->AddFunction(this->Internal->GreenSlicePlane);
      }
    else if (this->Internal->ClipModelsNode->GetGreenSliceClipState() == vtkMRMLClipModelsNode::ClipOff)
      {
      this->Internal->SlicePlanes->RemoveFunction(this->Internal->GreenSlicePlane);
      }
    modifiedState = 1;
    this->Internal->GreenSliceClipState = this->Internal->ClipModelsNode->GetGreenSliceClipState();
    }

  if (this->Internal->ClipModelsNode->GetYellowSliceClipState() != this->Internal->YellowSliceClipState)
    {
    if (this->Internal->YellowSliceClipState == vtkMRMLClipModelsNode::ClipOff)
      {
      this->Internal->SlicePlanes->AddFunction(this->Internal->YellowSlicePlane);
      }
    else if (this->Internal->ClipModelsNode->GetYellowSliceClipState() == vtkMRMLClipModelsNode::ClipOff)
      {
      this->Internal->SlicePlanes->RemoveFunction(this->Internal->YellowSlicePlane);
      }
    modifiedState = 1;
    this->Internal->YellowSliceClipState = this->Internal->ClipModelsNode->GetYellowSliceClipState();
    }

  // compute clipping on/off
  if (this->Internal->ClipModelsNode->GetRedSliceClipState() == vtkMRMLClipModelsNode::ClipOff &&
      this->Internal->ClipModelsNode->GetGreenSliceClipState() == vtkMRMLClipModelsNode::ClipOff &&
      this->Internal->ClipModelsNode->GetYellowSliceClipState() == vtkMRMLClipModelsNode::ClipOff )
    {
    this->Internal->ClippingOn = false;
    }
  else
    {
    this->Internal->ClippingOn = true;
    }

  // set slice plane normals and origins
  vtkMatrix4x4 *sliceMatrix = 0;
  int planeDirection = 1;

  sliceMatrix = this->Internal->RedSliceNode->GetSliceToRAS();
  planeDirection = (this->Internal->RedSliceClipState == vtkMRMLClipModelsNode::ClipNegativeSpace) ? -1 : 1;
  this->SetClipPlaneFromMatrix(sliceMatrix, planeDirection, this->Internal->RedSlicePlane);

  sliceMatrix = this->Internal->GreenSliceNode->GetSliceToRAS();
  planeDirection = (this->Internal->GreenSliceClipState == vtkMRMLClipModelsNode::ClipNegativeSpace) ? -1 : 1;
  this->SetClipPlaneFromMatrix(sliceMatrix, planeDirection, this->Internal->GreenSlicePlane);

  sliceMatrix = this->Internal->YellowSliceNode->GetSliceToRAS();
  planeDirection = (this->Internal->YellowSliceClipState == vtkMRMLClipModelsNode::ClipNegativeSpace) ? -1 : 1;
  this->SetClipPlaneFromMatrix(sliceMatrix, planeDirection, this->Internal->YellowSlicePlane);

  return modifiedState;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::SetClipPlaneFromMatrix(vtkMatrix4x4 *sliceMatrix,
                                                            int planeDirection,
                                                            vtkPlane *plane)
{
  double normal[3];
  double origin[3];

  for (int i = 0; i < 3; i++)
    {
    normal[i] = planeDirection * sliceMatrix->GetElement(i,2);
    origin[i] = sliceMatrix->GetElement(i,3);
    }
  plane->SetNormal(normal);
  plane->SetOrigin(origin);
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::ProcessMRMLEvents(vtkObject *caller,
                                                       unsigned long event,
                                                       void *callData)
{
  bool isUpdating = this->GetMRMLScene()->GetIsUpdating();
  if (vtkMRMLDisplayableNode::SafeDownCast(caller))
    {
    // There is no need to request a render (which can be expensive if the
    // volume rendering is on) if nothing visible has changed.
    bool requestRender = true;
    switch (event)
      {
      case vtkCommand::ModifiedEvent:
      case vtkMRMLDisplayableNode::PolyDataModifiedEvent:
      case vtkMRMLDisplayableNode::DisplayModifiedEvent:
        requestRender = this->OnMRMLDisplayableModelNodeModifiedEvent(
          vtkMRMLDisplayableNode::SafeDownCast(caller));
        break;
      default:
        this->SetUpdateFromMRMLRequested(1);
        break;
      }
    if (!isUpdating && requestRender)
      {
      this->RequestRender();
      }
    }
  else if (vtkMRMLClipModelsNode::SafeDownCast(caller))
    {
    if (event == vtkCommand::ModifiedEvent)
      {
      this->SetUpdateFromMRMLRequested(1);  
      }
    if (!isUpdating)
      {
      this->RequestRender();
      }
    }
  else if (vtkMRMLSliceNode::SafeDownCast(caller))
    {
    bool requestRender = true;
    if (event == vtkCommand::ModifiedEvent)
      {
      if (this->UpdateClipSlicesFromMRML() || this->Internal->ClippingOn)
        {
        this->SetUpdateFromMRMLRequested(1);
        }
      else
        {
        requestRender = vtkMRMLSliceNode::SafeDownCast(caller)->GetSliceVisible() == 1;
        }
      }
    if (!isUpdating && requestRender)
      {
      this->RequestRender();
      }
    }
  else if (vtkMRMLModelHierarchyNode::SafeDownCast(caller))
    {
    if (event == vtkCommand::ModifiedEvent)
      {
      this->UpdateModelHierarchies();
      this->SetUpdateFromMRMLRequested(1);
      }
    if (!isUpdating)
      {
      this->RequestRender();
      }
    }
  else
    {
    this->Superclass::ProcessMRMLEvents(caller, event, callData);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::OnMRMLSceneAboutToBeClosedEvent()
{
  this->RemoveHierarchyObservers(0);
  this->RemoveModelObservers(0);
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::OnMRMLSceneClosedEvent()
{
  // Clean
  this->RemoveModelProps();
  this->RemoveHierarchyObservers(1);
  this->RemoveModelObservers(1);

  this->SetUpdateFromMRMLRequested(1);
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::OnMRMLSceneImportedEvent()
{
  // UpdateFromMRML will be executed only if there has been some actions
  // during the import that requested it (don't call
  // SetUpdateFromMRMLRequested(1) here, it should be done somewhere else
  // maybe in OnMRMLSceneNodeAddedEvent, OnMRMLSceneNodeRemovedEvent or
  // OnMRMLDisplayableModelNodeModifiedEvent).
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::OnMRMLSceneRestoredEvent()
{
  // UpdateFromMRML will be executed only if there has been some actions
  // during the restoration that requested it (don't call
  // SetUpdateFromMRMLRequested(1) here, it should be done somewhere else
  // maybe in OnMRMLSceneNodeAddedEvent, OnMRMLSceneNodeRemovedEvent or
  // OnMRMLDisplayableModelNodeModifiedEvent).
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::OnMRMLSceneNodeAddedEvent(vtkMRMLNode* node)
{
  if (!node->IsA("vtkMRMLDisplayableNode") &&
      !node->IsA("vtkMRMLDisplayNode") &&
      !node->IsA("vtkMRMLModelHierarchyNode") &&
      node->IsA("vtkMRMLClipModelsNode"))
    {
    return;
    }

  this->SetUpdateFromMRMLRequested(1);

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->GetIsUpdating()){ return; }

  // Node specific processing
  if (node->IsA("vtkMRMLModelHierarchyNode"))
    {
    this->UpdateModelHierarchies();
    }
  else if (node->IsA("vtkMRMLClipModelsNode"))
    {
    vtkSetAndObserveMRMLNodeMacro(this->Internal->ClipModelsNode, node);
    }

  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::OnMRMLSceneNodeRemovedEvent(vtkMRMLNode* node)
{
  if (!node->IsA("vtkMRMLDisplayableNode") &&
      !node->IsA("vtkMRMLDisplayNode") &&
      !node->IsA("vtkMRMLModelHierarchyNode") &&
      !node->IsA("vtkMRMLClipModelsNode"))
    {
    return;
    }

  this->SetUpdateFromMRMLRequested(1);

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->GetIsUpdating()){ return; }

  // Node specific processing
  if (node->IsA("vtkMRMLDisplayableNode"))
    {
    this->RemoveDisplayable(vtkMRMLDisplayableNode::SafeDownCast(node));
    }
  else if (node->IsA("vtkMRMLModelHierarchyNode"))
    {
    this->UpdateModelHierarchies();
    }
  else if (node->IsA("vtkMRMLClipModelsNode"))
    {
    vtkSetMRMLNodeMacro(this->Internal->ClipModelsNode, 0);
    }

  this->RequestRender();
}

//---------------------------------------------------------------------------
bool vtkMRMLModelDisplayableManager::OnMRMLDisplayableModelNodeModifiedEvent(
    vtkMRMLDisplayableNode * modelNode)
{
  assert(modelNode);

  // If the node is already cached with an actor process only this one
  // If it was not visible and is still not visible do nothing
  std::vector< vtkMRMLDisplayNode *> dnodes = this->GetDisplayNode(modelNode);
  bool updateModel = false;
  bool updateMRML = false;
  for (unsigned int i=0; i<dnodes.size(); i++)
    {
    vtkMRMLDisplayNode *dnode = dnodes[i];
    assert(dnode);
    int visibility = dnode->GetVisibility();
    // If the displayNode is visible and doesn't have actors yet, then request
    // an updated
    if (visibility == 1 &&
        this->Internal->DisplayedActors.find(dnode->GetID()) == this->Internal->DisplayedActors.end())
      {
      updateMRML = true;
      break;
      }
    // If the displayNode visibility has changed or displayNode is visible, then
    // update the model.
    if (!(visibility == 0 && this->GetDisplayedModelsVisibility(dnode) == 0))
      {
      updateModel = true;
      break;
      }
    }
  if (updateModel)
    {
    this->UpdateClipSlicesFromMRML();
    this->UpdateModifiedModel(modelNode);
    this->SetUpdateFromMRMLRequested(1);
    }
  if (updateMRML)
    {
    this->SetUpdateFromMRMLRequested(1);
    }
  return updateModel || updateMRML;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::UpdateFromMRML()
{
  this->UpdateClipSlicesFromMRML();

  this->RemoveModelProps();
  
  this->UpdateModelsFromMRML();

  this->SetUpdateFromMRMLRequested(0);
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::UpdateModelsFromMRML()
{
  vtkMRMLScene *scene = this->GetMRMLScene();
  vtkMRMLNode *node = 0;
  std::vector<vtkMRMLDisplayableNode *> slices;

  // find volume slices
  bool clearDisplayedModels = scene ? false : true;
  
  std::vector<vtkMRMLNode *> dnodes;
  int nnodes = scene ? scene->GetNodesByClass("vtkMRMLDisplayableNode", dnodes) : 0;
  for (int n=0; n<nnodes; n++)
    {
    node = dnodes[n];
    vtkMRMLDisplayableNode *model = vtkMRMLDisplayableNode::SafeDownCast(node);
    // render slices last so that transparent objects are rendered in front of them
    if (!strcmp(model->GetName(), "Red Volume Slice") ||
        !strcmp(model->GetName(), "Green Volume Slice") ||
        !strcmp(model->GetName(), "Yellow Volume Slice"))
      {
      slices.push_back(model);
      vtkMRMLDisplayNode *dnode = model->GetDisplayNode();
      if (dnode && this->Internal->DisplayedActors.find(dnode->GetID()) == this->Internal->DisplayedActors.end() )
        {
        clearDisplayedModels = true;
        }
      }
    }

  if (clearDisplayedModels)
    {
    std::map<std::string, vtkProp3D *>::iterator iter;
    for (iter = this->Internal->DisplayedActors.begin(); iter != this->Internal->DisplayedActors.end(); iter++)
      {
      this->GetRenderer()->RemoveViewProp(iter->second);
      }
    this->RemoveModelObservers(1);
    this->RemoveHierarchyObservers(1);
    this->Internal->DisplayedActors.clear();
    this->Internal->DisplayedNodes.clear();
    this->Internal->DisplayedClipState.clear();
    this->Internal->DisplayedVisibility.clear();
    this->UpdateModelHierarchies();
    }

  // render slices first
  for (unsigned int i=0; i<slices.size(); i++)
    {
    vtkMRMLDisplayableNode *model = slices[i];
    // add nodes that are not in the list yet
    vtkMRMLDisplayNode *dnode = model->GetDisplayNode();
    if (dnode && this->Internal->DisplayedActors.find(dnode->GetID()) == this->Internal->DisplayedActors.end() )
      {
      this->UpdateModel(model);
      } 
    this->SetModelDisplayProperty(model);
    }
  
  // render the rest of the models
  //int nmodels = scene->GetNumberOfNodesByClass("vtkMRMLDisplayableNode");
  for (int n=0; n<nnodes; n++)
    {
    vtkMRMLDisplayableNode *model = vtkMRMLDisplayableNode::SafeDownCast(dnodes[n]);
    // render slices last so that transparent objects are rendered in fron of them
    if (model)
      {
      if (!strcmp(model->GetName(), "Red Volume Slice") ||
          !strcmp(model->GetName(), "Green Volume Slice") ||
          !strcmp(model->GetName(), "Yellow Volume Slice"))
        {
        continue;
        }
      this->UpdateModifiedModel(model);
      }
    } // end while
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::UpdateModifiedModel(vtkMRMLDisplayableNode *model)
{

  this->UpdateModelHierarchyDisplay(model);
  this->UpdateModel(model);
  this->SetModelDisplayProperty(model);
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::UpdateModelPolyData(vtkMRMLDisplayableNode *model)
{
  std::vector< vtkMRMLDisplayNode *> displayNodes = this->GetDisplayNode(model);
  vtkMRMLDisplayNode *hdnode = this->GetHierarchyDisplayNode(model);
  
  for (unsigned int i=0; i<displayNodes.size(); i++)
    {
    vtkMRMLDisplayNode *modelDisplayNode = displayNodes[i];
    vtkProp3D* prop = 0;
    bool hasPolyData = true;

    int clipping = modelDisplayNode->GetClipping();
    int visibility = modelDisplayNode->GetVisibility();
    vtkPolyData *polyData = modelDisplayNode->GetPolyData();

    if (hdnode) 
      {
      clipping = hdnode->GetClipping();
      //visibility = hdnode->GetVisibility();
      polyData = hdnode->GetPolyData();
      }  
    // hierarchy display nodes may not have poly data pointer
    if (polyData == 0)
      {
      polyData = model->GetPolyData();
      }
    if (polyData == 0)
      {
      hasPolyData = false;
      }
     
    std::map<std::string, vtkProp3D *>::iterator ait;
    ait = this->Internal->DisplayedActors.find(modelDisplayNode->GetID());
    if (ait == this->Internal->DisplayedActors.end() )
      {
      if(polyData)
        {
#ifdef USE_IMAGE_ACTOR
        if ( polyData->GetNumberOfCells() == 1 )
          {
          prop = vtkImageActor::New();
          }
#endif
        }
      if (!prop)
        {
        prop = vtkActor::New();
        }
      }
    else
      {
      prop = (*ait).second;
      std::map<std::string, int>::iterator cit = this->Internal->DisplayedClipState.find(modelDisplayNode->GetID());
      if (modelDisplayNode && cit != this->Internal->DisplayedClipState.end() && cit->second == clipping )
        {
        this->Internal->DisplayedVisibility[modelDisplayNode->GetID()] = visibility;
        // make sure that we are looking at the current polydata (most of the code in here 
        // assumes a display node will never change what polydata it wants to view and hence
        // caches information to skip steps if the display node has already rendered. but we
        // can have rendered a display node but not rendered its current polydata.
        vtkActor *actor = vtkActor::SafeDownCast(prop);
        if (actor)
          {
          vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
          if (mapper && mapper->GetInput() != polyData && !(this->Internal->ClippingOn && clipping))
            {
            mapper->SetInput(polyData);
            }
          }
        vtkMRMLTransformNode* tnode = model->GetParentTransformNode();
        // clipped model could be transformed
        if (clipping == 0 || tnode == 0 || !tnode->IsLinear())
          {
          continue;
          }
        }
      }

    vtkClipPolyData *clipper = 0;
    vtkActor * actor = vtkActor::SafeDownCast(prop);
    if(actor)
      {
      if (this->Internal->ClippingOn && modelDisplayNode != 0 && clipping)
        {
        clipper = this->CreateTransformedClipper(model);
        }

      vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
     
      if (clipper)
        {
        clipper->SetInput(polyData);
        clipper->Update();
        mapper->SetInput(clipper->GetOutput());
        }
      else
        {
        mapper->SetInput(polyData);
        }

      actor->SetMapper(mapper);
      mapper->Delete();
      }

    if (hasPolyData && ait == this->Internal->DisplayedActors.end())
      {
      this->GetRenderer()->AddViewProp(prop);
      this->Internal->DisplayedActors[modelDisplayNode->GetID()] = prop;
      this->Internal->DisplayedNodes[std::string(modelDisplayNode->GetID())] = modelDisplayNode;
    
      if (modelDisplayNode)
        {
        this->Internal->DisplayedVisibility[modelDisplayNode->GetID()] = visibility;
        }
      else
        {
        this->Internal->DisplayedVisibility[modelDisplayNode->GetID()] = 1;
        }
      
      if (clipper)
        {
        this->Internal->DisplayedClipState[modelDisplayNode->GetID()] = 1;
        clipper->Delete();
        }
      else
        {
        this->Internal->DisplayedClipState[modelDisplayNode->GetID()] = 0;
        }
      prop->Delete();
      }
    else if (!hasPolyData)
      {
      prop->Delete();
      }
    else 
      {
      if (clipper)
        {
        this->Internal->DisplayedClipState[modelDisplayNode->GetID()] = 1;
        clipper->Delete();
        }
      else 
        {
        this->Internal->DisplayedClipState[modelDisplayNode->GetID()] = 0;
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::UpdateModel(vtkMRMLDisplayableNode *model)
{
  this->UpdateModelPolyData(model);

  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  std::vector< vtkObservation *> observations;
  // observe polydata
  observations = broker->GetObservations(model, vtkMRMLDisplayableNode::PolyDataModifiedEvent,
                                         this, this->GetMRMLCallbackCommand());
  if (observations.size() == 0)
    {
    broker->AddObservation(model, vtkMRMLDisplayableNode::PolyDataModifiedEvent,
                           this, this->GetMRMLCallbackCommand());
    this->Internal->DisplayableNodes[model->GetID()] = model;
    }
  // observe display node
  observations = broker->GetObservations(model, vtkMRMLDisplayableNode::DisplayModifiedEvent,
                                         this, this->GetMRMLCallbackCommand());
  if (observations.size() == 0)
    {
    broker->AddObservation(model, vtkMRMLDisplayableNode::DisplayModifiedEvent,
                           this, this->GetMRMLCallbackCommand());
    }

  observations = broker->GetObservations(model, vtkMRMLTransformableNode::TransformModifiedEvent,
                                         this, this->GetMRMLCallbackCommand());
  if (observations.size() == 0)
    {
    broker->AddObservation(model, vtkMRMLTransformableNode::TransformModifiedEvent,
                           this, this->GetMRMLCallbackCommand());
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::CheckModelHierarchies()
{
  if (this->GetMRMLScene() == 0 || this->Internal->ModelHierarchyLogic == 0)
    {
    return;
    }
  this->Internal->ModelHierarchyLogic->SetMRMLScene(this->GetMRMLScene());
  int nnodes = this->Internal->ModelHierarchyLogic->GetNumberOfModelsInHierarchy();
  this->Internal->ModelHierarchiesPresent = nnodes > 0 ? true:false;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::AddHierarchyObservers()
{
  if (this->GetMRMLScene() == 0)
    {
    return;
    }
  vtkMRMLModelHierarchyNode *node;
  
  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  std::vector<vtkMRMLNode *> hnodes;
  int nnodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLModelHierarchyNode", hnodes);

  for (int n=0; n<nnodes; n++)
    {
    node = vtkMRMLModelHierarchyNode::SafeDownCast (hnodes[n]);
    bool found = false;
    std::map<std::string, int>::iterator iter;
    // search for matching string (can't use find, since it would look for 
    // matching pointer not matching content)
    for(iter=this->Internal->RegisteredModelHierarchies.begin(); iter != this->Internal->RegisteredModelHierarchies.end(); iter++)
      {
      if ( iter->first.c_str() && !strcmp( iter->first.c_str(), node->GetID() ) )
        {
        found = true;
        break;
        }
      }
    if (!found)
      {
      broker->AddObservation( node, vtkCommand::ModifiedEvent, this, this->GetMRMLCallbackCommand() );
      this->Internal->RegisteredModelHierarchies[node->GetID()] = 0;
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::UpdateModelHierarchies()
{
  this->CheckModelHierarchies();
  this->AddHierarchyObservers();
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::UpdateModelHierarchyVisibility(vtkMRMLModelHierarchyNode* mhnode, int visibility )
{
  vtkMRMLDisplayNode* dnode = mhnode->GetDisplayNode();
  if (dnode)
    {
    std::map<std::string, vtkProp3D *>::iterator iter = this->Internal->DisplayedActors.find(dnode->GetID());
    if (iter != this->Internal->DisplayedActors.end())
      {
      vtkProp3D *actor = iter->second;
      actor->SetVisibility(visibility);
      this->Internal->DisplayedVisibility[dnode->GetID()] = visibility;
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::UpdateModelHierarchyDisplay(vtkMRMLDisplayableNode *model)
{
  if (model && this->Internal->ModelHierarchyLogic)
    {
    vtkMRMLModelHierarchyNode* mhnode = vtkMRMLModelHierarchyNode::SafeDownCast(
        vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(this->GetMRMLScene(), model->GetID()));

    if (mhnode) 
      {
      // turn off visibility of this node
      int ndnodes = model->GetNumberOfDisplayNodes();
      for (int i=0; i<ndnodes; i++)
        {
        vtkMRMLDisplayNode *dnode = model->GetNthDisplayNode(i);
        if (dnode)
          {
          std::map<std::string, vtkProp3D *>::iterator iter = this->Internal->DisplayedActors.find(dnode->GetID());
          if (iter != this->Internal->DisplayedActors.end())
            {
            vtkProp3D *actor = iter->second;
            actor->SetVisibility(0);
            this->Internal->DisplayedVisibility[dnode->GetID()] = 0;
            }
          }
        }

      // turn off visibility for hierarchy nodes in the tree
      vtkMRMLModelHierarchyNode *parent = mhnode;
      do 
        {
        this->UpdateModelHierarchyVisibility(parent, 0);
        parent = vtkMRMLModelHierarchyNode::SafeDownCast(parent->GetParentNode());
        }
      while (parent != 0);
      }
    }
}

//---------------------------------------------------------------------------
std::vector< vtkMRMLDisplayNode* > vtkMRMLModelDisplayableManager::GetDisplayNode(vtkMRMLDisplayableNode *model)
{
  std::vector< vtkMRMLDisplayNode* > dnodes;
  vtkMRMLDisplayNode* dnode = 0;

  int ndnodes = model->GetNumberOfDisplayNodes();
  for (int i=0; i<ndnodes; i++)
    {
    dnode = model->GetNthDisplayNode(i);
    if (dnode)
      {
      dnodes.push_back(dnode);
      }
    }
    
  return dnodes;
}

//---------------------------------------------------------------------------
vtkMRMLDisplayNode*  vtkMRMLModelDisplayableManager::GetHierarchyDisplayNode(vtkMRMLDisplayableNode *model)
{
  vtkMRMLDisplayNode* dnode = 0;
  if (this->Internal->ModelHierarchiesPresent)
    {
    vtkMRMLModelHierarchyNode* mhnode = 0;
    vtkMRMLModelHierarchyNode* phnode = 0;

    mhnode = vtkMRMLModelHierarchyNode::SafeDownCast(
        vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(this->GetMRMLScene(), model->GetID()));
    if (mhnode) 
      {
      phnode = mhnode->GetUnExpandedParentNode();
      }
    if (phnode) 
      {
      dnode = phnode->GetDisplayNode();
      }
    }
  return dnode;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::RemoveModelProps()
{
  std::map<std::string, vtkProp3D *>::iterator iter;
  std::map<std::string, int>::iterator clipIter;
  std::vector<std::string> removedIDs;
  for(iter=this->Internal->DisplayedActors.begin(); iter != this->Internal->DisplayedActors.end(); iter++)
    {
    vtkMRMLDisplayNode *modelDisplayNode = vtkMRMLDisplayNode::SafeDownCast(
      this->GetMRMLScene() ? this->GetMRMLScene()->GetNodeByID(iter->first) : 0);
    if (modelDisplayNode == 0)
      {
      this->GetRenderer()->RemoveViewProp(iter->second);
      removedIDs.push_back(iter->first);
      }
    else
      {
      int clipModel = 0;
      if (modelDisplayNode != 0)
        {
        clipModel = modelDisplayNode->GetClipping();
        }
      clipIter = this->Internal->DisplayedClipState.find(iter->first);
      if (clipIter == this->Internal->DisplayedClipState.end())
        {
        vtkErrorMacro ("vtkMRMLModelDisplayableManager::RemoveModelProps() Unknown clip state\n");
        }
      else 
        {
        if ((clipIter->second && !this->Internal->ClippingOn) ||
            (this->Internal->ClippingOn && clipIter->second != clipModel))
          {
          this->GetRenderer()->RemoveViewProp(iter->second);
          removedIDs.push_back(iter->first);
          }     
        }
      }
    }
  for (unsigned int i=0; i< removedIDs.size(); i++)
    {
    this->RemoveDispalyedID(removedIDs[i]);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::RemoveDisplayable(vtkMRMLDisplayableNode* model)
{
  if (!model)
    {
    return;
    }  
  int ndnodes = model->GetNumberOfDisplayNodes();
  std::map<std::string, vtkProp3D *>::iterator iter;
  std::vector<std::string> removedIDs;
  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLDisplayNode *displayNode = model->GetNthDisplayNode(i);
    if (displayNode)
      {
      iter = this->Internal->DisplayedActors.find(displayNode->GetID());
      if (iter != this->Internal->DisplayedActors.end())
        {
        this->GetRenderer()->RemoveViewProp(iter->second);
        removedIDs.push_back(iter->first);
        }
      }
    }

  for (unsigned int i=0; i< removedIDs.size(); i++)
    {
    this->RemoveDispalyedID(removedIDs[i]);
    }
  this->RemoveDisplayableNodeObservers(model);
  this->Internal->DisplayableNodes.erase(model->GetID());
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::RemoveDispalyedID(std::string &id)
{
  std::map<std::string, vtkMRMLDisplayNode *>::iterator modelIter;
  this->Internal->DisplayedActors.erase(id);
  this->Internal->DisplayedClipState.erase(id);
  this->Internal->DisplayedVisibility.erase(id);
  modelIter = this->Internal->DisplayedNodes.find(id);
  if(modelIter != this->Internal->DisplayedNodes.end())
    {
    //this->RemoveModelObservers(modelIter->second);
    this->Internal->DisplayedNodes.erase(modelIter->first);
    }
}

//---------------------------------------------------------------------------
int vtkMRMLModelDisplayableManager::GetDisplayedModelsVisibility(vtkMRMLDisplayNode *model)
{
  int visibility = 1;
  
  std::map<std::string, int>::iterator iter;
  iter = this->Internal->DisplayedVisibility.find(model->GetID());
  if (iter != this->Internal->DisplayedVisibility.end())
    {
    visibility = iter->second;
    }

  return visibility;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::RemoveMRMLObservers()
{
  this->RemoveModelObservers(1);
  this->RemoveHierarchyObservers(1);

  this->Superclass::RemoveMRMLObservers();
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::RemoveModelObservers(int clearCache)
{
  std::map<std::string, vtkMRMLDisplayableNode *>::iterator iter;

  for (iter=this->Internal->DisplayableNodes.begin();
       iter!=this->Internal->DisplayableNodes.end();
       iter++)
    {
    this->RemoveDisplayableNodeObservers(iter->second);
    }
  if (clearCache)
    {
    this->Internal->DisplayableNodes.clear();
    this->Internal->DisplayedActors.clear();
    this->Internal->DisplayedNodes.clear();
    this->Internal->DisplayedClipState.clear();
    this->Internal->DisplayedVisibility.clear();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::RemoveDisplayableNodeObservers(vtkMRMLDisplayableNode *model)
{
  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  std::vector< vtkObservation *> observations;
  if (model != 0)
    {
    observations = broker->GetObservations( 
      model, vtkMRMLDisplayableNode::PolyDataModifiedEvent, this, this->GetMRMLCallbackCommand() );
    broker->RemoveObservations(observations);
    observations = broker->GetObservations( 
      model, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, this->GetMRMLCallbackCommand() );
    broker->RemoveObservations(observations);
    observations = broker->GetObservations( 
      model, vtkMRMLTransformableNode::TransformModifiedEvent, this, this->GetMRMLCallbackCommand() );
    broker->RemoveObservations(observations);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::RemoveHierarchyObservers(int clearCache)
{
  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  std::vector< vtkObservation *> observations;
  std::map<std::string, int>::iterator iter;

  for(iter=this->Internal->RegisteredModelHierarchies.begin();
      iter != this->Internal->RegisteredModelHierarchies.end();
      iter++)
    {
    vtkMRMLModelHierarchyNode *node = vtkMRMLModelHierarchyNode::SafeDownCast(
      this->GetMRMLScene() ? this->GetMRMLScene()->GetNodeByID(iter->first) : 0);
    if (node)
      {
      observations = broker->GetObservations( 
        node, vtkCommand::ModifiedEvent, this, this->GetMRMLCallbackCommand() );
      broker->RemoveObservations(observations);
      }
    }
  if (clearCache)
    {
    this->Internal->RegisteredModelHierarchies.clear();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::SetModelDisplayProperty(vtkMRMLDisplayableNode *model)
{
  vtkMRMLTransformNode* tnode = model->GetParentTransformNode();
  vtkSmartPointer<vtkMatrix4x4> transformToWorld = vtkSmartPointer<vtkMatrix4x4>::New();
  transformToWorld->Identity();
  if (tnode != 0 && tnode->IsLinear())
    {
    vtkMRMLLinearTransformNode *lnode = vtkMRMLLinearTransformNode::SafeDownCast(tnode);
    lnode->GetMatrixTransformToWorld(transformToWorld);
    }
 
  std::vector<vtkMRMLDisplayNode *> dnodes = this->GetDisplayNode(model);
  vtkMRMLDisplayNode *hdnode = this->GetHierarchyDisplayNode(model);
  
  for (unsigned int i=0; i<dnodes.size(); i++)
    {
    vtkMRMLDisplayNode *dnode = dnodes[i];
    vtkMRMLDisplayNode *mdnode = dnode;
    if (dnode != 0)
      {
      vtkProp3D *prop = this->GetActorByID(dnode->GetID());
      if (prop == 0)
        {
        continue;
        }
      // use hierarchy dispaly node if it exists
      if (hdnode)
        {
        dnode = hdnode;
        }
        
      vtkActor *actor = vtkActor::SafeDownCast(prop);
      vtkImageActor *imageActor = vtkImageActor::SafeDownCast(prop);
      prop->SetUserMatrix(transformToWorld);

      prop->SetVisibility(mdnode->GetVisibility());
      this->Internal->DisplayedVisibility[dnode->GetID()] = mdnode->GetVisibility();

      if (actor)
        {
        actor->GetMapper()->SetScalarVisibility(mdnode->GetScalarVisibility());
        // if the scalars are visible, set active scalars, try to get the lookup
        // table
        if (mdnode->GetScalarVisibility())
          {
          if (mdnode->GetColorNode() != 0)
            {
            if (mdnode->GetColorNode()->GetLookupTable() != 0)
              {
              actor->GetMapper()->SetLookupTable(mdnode->GetColorNode()->GetLookupTable());
              }
            else if (mdnode->GetColorNode()->IsA("vtkMRMLProceduralColorNode") &&
                     vtkMRMLProceduralColorNode::SafeDownCast(mdnode->GetColorNode())->GetColorTransferFunction() != 0)
              {
              actor->GetMapper()->SetLookupTable((vtkScalarsToColors*)(vtkMRMLProceduralColorNode::SafeDownCast(mdnode->GetColorNode())->GetColorTransferFunction()));
              }
            }

          int cellScalarsActive = 0;
          if (mdnode->GetActiveScalarName() == 0)
            {
            // see if there are scalars on the poly data that are not set as
            // active on the display node
            vtkMRMLModelNode *mnode = vtkMRMLModelNode::SafeDownCast(model);
            if (mnode)
              {
              std::string pointScalarName = std::string(mnode->GetActivePointScalarName("scalars"));
              std::string cellScalarName = std::string(mnode->GetActiveCellScalarName("scalars"));
              vtkDebugMacro("Display node active scalar name was 0, but the node says active point scalar name = '" << pointScalarName.c_str() << "', cell = '" << cellScalarName.c_str() << "'");
              if (pointScalarName.compare("") != 0)
                {
                vtkDebugMacro("Setting the display node's active scalar to " << pointScalarName.c_str());
                dnode->SetActiveScalarName(pointScalarName.c_str());
                }
              else
                {
                if (cellScalarName.compare("") != 0)
                  {
                  vtkDebugMacro("Setting the display node's active scalar to " << cellScalarName.c_str());
                  dnode->SetActiveScalarName(cellScalarName.c_str());
                  }
                else
                  {
                  vtkDebugMacro("No active scalars");
                  }
                }
              }
            }
          if (mdnode->GetActiveScalarName() != 0)
            {
            vtkMRMLModelNode *mnode = vtkMRMLModelNode::SafeDownCast(model);
            if (mnode)
              {
              mnode->SetActiveScalars(mdnode->GetActiveScalarName(), "Scalars");
              if (strcmp(mdnode->GetActiveScalarName(), mnode->GetActiveCellScalarName("scalars")) == 0)
                {
                cellScalarsActive = 1;
                }
              }
            actor->GetMapper()->SelectColorArray(mdnode->GetActiveScalarName());
            }
          if (!cellScalarsActive)
            {
            // set the scalar range
            actor->GetMapper()->SetScalarRange(mdnode->GetScalarRange());
            //if (!(dnode->IsA("vtkMRMLFiberBundleDisplayNode")))
            //  {
            // WHY need this, does not show glyph colors otherwise
            //actor->GetMapper()->SetScalarModeToUsePointFieldData();
            // }
            actor->GetMapper()->SetScalarModeToUsePointData();            
            actor->GetMapper()->SetColorModeToMapScalars();            
            actor->GetMapper()->UseLookupTableScalarRangeOff();
            actor->GetMapper()->SetScalarRange(mdnode->GetScalarRange());
            }
          else
            {
            actor->GetMapper()->SetScalarModeToUseCellFieldData();
            actor->GetMapper()->SetColorModeToDefault();
            actor->GetMapper()->UseLookupTableScalarRangeOff();
            actor->GetMapper()->SetScalarRange(mdnode->GetScalarRange());
            }
          }
        //// }
        actor->GetProperty()->SetBackfaceCulling(dnode->GetBackfaceCulling());

        if (mdnode)
          {
          if (mdnode->GetSelected())
            {
            vtkDebugMacro("Model display node " << mdnode->GetName() << " is selected...");
            actor->GetProperty()->SetColor(mdnode->GetSelectedColor());
            actor->GetProperty()->SetAmbient(mdnode->GetSelectedAmbient());
            actor->GetProperty()->SetSpecular(mdnode->GetSelectedSpecular());
            }
          else
            {
            //vtkWarningMacro("Model display node " << mdnode->GetName() << " is not selected...");
            actor->GetProperty()->SetColor(dnode->GetColor());
            actor->GetProperty()->SetAmbient(dnode->GetAmbient());
            actor->GetProperty()->SetSpecular(dnode->GetSpecular());
            }
          }
        actor->GetProperty()->SetOpacity(dnode->GetOpacity());
        actor->GetProperty()->SetDiffuse(dnode->GetDiffuse());
        actor->GetProperty()->SetSpecularPower(dnode->GetPower());
        if (dnode->GetTextureImageData() != 0)
          {
          if (actor->GetTexture() == 0)
            {
            vtkTexture *texture = vtkTexture::New();
            texture->SetInterpolate(0);
            actor->SetTexture(texture);
            texture->Delete();
            }
          actor->GetTexture()->SetInput(dnode->GetTextureImageData());
          actor->GetProperty()->SetColor(1., 1., 1.);
          }
        else
          {
          actor->SetTexture(0);
          }
        }
      else if (imageActor)
        {
        if (dnode->GetTextureImageData() != 0)
          {
          imageActor->SetInput(dnode->GetTextureImageData());
          }
        else
          {
          imageActor->SetInput(0);
          }
        imageActor->SetDisplayExtent(-1, 0, 0, 0, 0, 0);
        }
      }
    }

}

//---------------------------------------------------------------------------
// Description:
// return the current actor corresponding to a give MRML ID
vtkProp3D * vtkMRMLModelDisplayableManager::GetActorByID(const char *id)
{
  if ( !id )
    {
    return (0);
    }

  std::map<std::string, vtkProp3D *>::iterator iter;
  // search for matching string (can't use find, since it would look for 
  // matching pointer not matching content)
  for(iter=this->Internal->DisplayedActors.begin(); iter != this->Internal->DisplayedActors.end(); iter++)
    {
    if ( iter->first.c_str() && !strcmp( iter->first.c_str(), id ) )
      {
      return (iter->second);
      }
    }
  return (0);
}

//---------------------------------------------------------------------------
// Description:
// return the ID for the given actor 
const char * vtkMRMLModelDisplayableManager::GetIDByActor(vtkProp3D *actor)
{
  if ( !actor )
    {
    return (0);
    }

  std::map<std::string, vtkProp3D *>::iterator iter;
  for(iter=this->Internal->DisplayedActors.begin();
      iter != this->Internal->DisplayedActors.end();
      iter++)
    {
    if ( iter->second && ( iter->second == actor ) )
      {
      return (iter->first.c_str());
      }
    }
  return (0);
}


//---------------------------------------------------------------------------
vtkWorldPointPicker* vtkMRMLModelDisplayableManager::GetWorldPointPicker()
{
  vtkDebugMacro(<< "returning Internal->WorldPointPicker address "
                << this->Internal->WorldPointPicker.GetPointer());
  return this->Internal->WorldPointPicker;
}

//---------------------------------------------------------------------------
vtkPropPicker* vtkMRMLModelDisplayableManager::GetPropPicker()
{
  vtkDebugMacro(<< "returning Internal->PropPicker address "
                << this->Internal->PropPicker.GetPointer());
  return this->Internal->PropPicker;
}

//---------------------------------------------------------------------------
vtkCellPicker* vtkMRMLModelDisplayableManager::GetCellPicker()
{
  vtkDebugMacro(<< "returning Internal->CellPicker address "
                << this->Internal->CellPicker.GetPointer());
  return this->Internal->CellPicker;
}

//---------------------------------------------------------------------------
vtkPointPicker* vtkMRMLModelDisplayableManager::GetPointPicker()
{
  vtkDebugMacro(<< "returning Internal->PointPicker address "
                << this->Internal->PointPicker.GetPointer());
  return this->Internal->PointPicker;
}

//---------------------------------------------------------------------------
int vtkMRMLModelDisplayableManager::Pick(int x, int y)
{
  double RASPoint[3] = {0.0, 0.0, 0.0};
  double pickPoint[3] = {0.0, 0.0, 0.0};

  // Reset the pick vars
  this->Internal->ResetPick();
  
  vtkRenderer *ren;
  if (this->GetRenderer() != 0)
    {
    ren = this->GetRenderer();
    }
  else
    {
    vtkErrorMacro("Pick: unable to get renderer\n");
    return 0;
    }
  // get the current renderer's size
  int *renSize = ren->GetSize();
  // resize the interactor?
  
  // pass the event's display point to the world point picker
  double displayPoint[3];
  displayPoint[0] = x;
  displayPoint[1] = renSize[1] - y;
  displayPoint[2] = 0.0;

  if (this->Internal->CellPicker->Pick(displayPoint[0], displayPoint[1], displayPoint[2], ren))
    {
    this->Internal->CellPicker->GetPickPosition(pickPoint);
    this->SetPickedCellID(this->Internal->CellPicker->GetCellId());
    // get the pointer to the poly data that the cell was in
    vtkPolyData *polyData = vtkPolyData::SafeDownCast(this->Internal->CellPicker->GetDataSet());
    if (polyData != 0)
      {
      // now find the model this poly data belongs to
      std::map<std::string, vtkMRMLDisplayNode *>::iterator modelIter;
      for (modelIter = this->Internal->DisplayedNodes.begin();
           modelIter != this->Internal->DisplayedNodes.end();
           modelIter++)
        {
        vtkDebugMacro("Checking model " << modelIter->first.c_str() << "'s polydata");
        if (modelIter->second != 0)
          {
          if (modelIter->second->GetPolyData() == polyData)
            {
            vtkDebugMacro("Found matching poly data, pick was on model " << modelIter->first.c_str());
            this->Internal->PickedNodeName = modelIter->first;
            
            // figure out the closest vertex in the picked cell to the picked RAS
            // point. Only doing this on model nodes for now.
            vtkCell *cell = polyData->GetCell(this->GetPickedCellID());
            if (cell != 0)
              {
              int numPoints = cell->GetNumberOfPoints();
              int closestPointId = -1;
              double closestDistance = 0.0l;
              for (int p = 0; p < numPoints; p++)
                {
                int pointId = cell->GetPointId(p);
                double *pointCoords = polyData->GetPoint(pointId);
                if (pointCoords != 0)
                  {
                  double distance = sqrt(pow(pointCoords[0]-pickPoint[0], 2) +
                                         pow(pointCoords[1]-pickPoint[1], 2) +
                                         pow(pointCoords[2]-pickPoint[2], 2));
                  if (p == 0 ||
                      distance < closestDistance)
                    {
                    closestDistance = distance;
                    closestPointId = pointId;
                    }
                  }
                }
              vtkDebugMacro("Pick: found closest point id = " << closestPointId << ", distance = " << closestDistance);
              this->SetPickedPointID(closestPointId);
              }
            continue;
            }
          }
        }
      }
    }
  else
    {
    return 0;
    }
  /**
  if (this->PropPicker->PickProp(x, y, ren))
    {
    this->PropPicker->GetPickPosition(pickPoint);
    }
  else
    {
    return 0;
    }
    **/

  // world point picker's Pick always returns 0
  /**
  this->WorldPointPicker->Pick(displayPoint, ren);
  this->WorldPointPicker->GetPickPosition(pickPoint);
  **/

  // translate world to RAS
  for (int p = 0; p < 3; p++)
    {
    RASPoint[p] = pickPoint[p];
    }
  
  // now set up the class vars
  this->SetPickedRAS(RASPoint);

  return 1;
}     

//---------------------------------------------------------------------------
const char * vtkMRMLModelDisplayableManager::GetPickedNodeName()
{
  vtkDebugMacro(<< "returning this->Internal->PickedNodeName of "
                << (this->Internal->PickedNodeName.empty()?
                    "(empty)":this->Internal->PickedNodeName));
  return this->Internal->PickedNodeName.c_str();
}

//---------------------------------------------------------------------------
double* vtkMRMLModelDisplayableManager::GetPickedRAS()
{
  vtkDebugMacro(<< "returning Internal->PickedRAS pointer " << this->Internal->PickedRAS);
  return this->Internal->PickedRAS;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::SetPickedRAS(double* newPickedRAS)
{
  int i;
  for (i=0; i<3; i++) { if ( newPickedRAS[i] != this->Internal->PickedRAS[i] ) { break; }}
  if (i < 3)
    {
    for (i=0; i<3; i++) { this->Internal->PickedRAS[i] = newPickedRAS[i]; }
    this->Modified();
    }
}

//---------------------------------------------------------------------------
vtkIdType vtkMRMLModelDisplayableManager::GetPickedCellID()
{
  vtkDebugMacro(<< "returning this->Internal->PickedCellID of " << this->Internal->PickedCellID);
  return this->Internal->PickedCellID;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::SetPickedCellID(vtkIdType newCellID)
{
  vtkDebugMacro(<< "setting PickedCellID to " << newCellID);
  if (this->Internal->PickedCellID != newCellID)
    {
    this->Internal->PickedCellID = newCellID;
    this->Modified();
    }
}

//---------------------------------------------------------------------------
vtkIdType vtkMRMLModelDisplayableManager::GetPickedPointID()
{
  vtkDebugMacro(<< "returning this->Internal->PickedPointID of " << this->Internal->PickedPointID);
  return this->Internal->PickedPointID;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::SetPickedPointID(vtkIdType newPointID)
{
  vtkDebugMacro(<< "setting PickedPointID to " << newPointID);
  if (this->Internal->PickedPointID != newPointID)
    {
    this->Internal->PickedPointID = newPointID;
    this->Modified();
    }
}

//---------------------------------------------------------------------------
vtkMRMLModelHierarchyLogic* vtkMRMLModelDisplayableManager::GetModelHierarchyLogic()
{
  vtkDebugMacro("returning Internal->ModelHierarchyLogic address "
                << this->Internal->ModelHierarchyLogic);
  return this->Internal->ModelHierarchyLogic;
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayableManager::SetModelHierarchyLogic(
    vtkMRMLModelHierarchyLogic* newModelHierarchyLogic)
{
  vtkSetObjectBodyMacro(Internal->ModelHierarchyLogic, vtkMRMLModelHierarchyLogic,
                        newModelHierarchyLogic);
}

//---------------------------------------------------------------------------
vtkClipPolyData* vtkMRMLModelDisplayableManager::CreateTransformedClipper(
    vtkMRMLDisplayableNode *model)
{
  vtkClipPolyData *clipper = vtkClipPolyData::New();
  clipper->SetValue( 0.0);

  vtkMRMLTransformNode* tnode = model->GetParentTransformNode();
  vtkSmartPointer<vtkMatrix4x4> transformToWorld = vtkSmartPointer<vtkMatrix4x4>::New();
  transformToWorld->Identity();
  if (tnode != 0 && tnode->IsLinear())
    {
    vtkMRMLLinearTransformNode *lnode = vtkMRMLLinearTransformNode::SafeDownCast(tnode);
    lnode->GetMatrixTransformToWorld(transformToWorld);

    vtkImplicitBoolean *slicePlanes =   vtkImplicitBoolean::New();
    if (this->Internal->ClipType == vtkMRMLClipModelsNode::ClipIntersection)
      {
      slicePlanes->SetOperationTypeToIntersection();
      }
    else if (this->Internal->ClipType == vtkMRMLClipModelsNode::ClipUnion)
      {
      slicePlanes->SetOperationTypeToUnion();
      }

    vtkPlane *redSlicePlane = vtkPlane::New();
    vtkPlane *greenSlicePlane = vtkPlane::New();
    vtkPlane *yellowSlicePlane = vtkPlane::New();


    if (this->Internal->RedSliceClipState != vtkMRMLClipModelsNode::ClipOff)
      {
      slicePlanes->AddFunction(redSlicePlane);
      }

    if (this->Internal->GreenSliceClipState != vtkMRMLClipModelsNode::ClipOff)
      {
      slicePlanes->AddFunction(greenSlicePlane);
      }

    if (this->Internal->YellowSliceClipState != vtkMRMLClipModelsNode::ClipOff)
      {
      slicePlanes->AddFunction(yellowSlicePlane);
      }

    vtkMatrix4x4 *sliceMatrix = 0;
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    int planeDirection = 1;
    transformToWorld->Invert();

    sliceMatrix = this->Internal->RedSliceNode->GetSliceToRAS();
    mat->Identity();
    vtkMatrix4x4::Multiply4x4(transformToWorld, sliceMatrix, mat);
    planeDirection = (this->Internal->RedSliceClipState == vtkMRMLClipModelsNode::ClipNegativeSpace) ? -1 : 1;
    this->SetClipPlaneFromMatrix(mat, planeDirection, redSlicePlane);

    sliceMatrix = this->Internal->GreenSliceNode->GetSliceToRAS();
    mat->Identity();
    vtkMatrix4x4::Multiply4x4(transformToWorld, sliceMatrix, mat);
    planeDirection = (this->Internal->GreenSliceClipState == vtkMRMLClipModelsNode::ClipNegativeSpace) ? -1 : 1;
    this->SetClipPlaneFromMatrix(mat, planeDirection, greenSlicePlane);

    sliceMatrix = this->Internal->YellowSliceNode->GetSliceToRAS();
    mat->Identity();
    vtkMatrix4x4::Multiply4x4(transformToWorld, sliceMatrix, mat);
    planeDirection = (this->Internal->YellowSliceClipState == vtkMRMLClipModelsNode::ClipNegativeSpace) ? -1 : 1;
    this->SetClipPlaneFromMatrix(mat, planeDirection, yellowSlicePlane);

    clipper->SetClipFunction(slicePlanes);
    
    slicePlanes->Delete();
    redSlicePlane->Delete();
    greenSlicePlane->Delete();
    yellowSlicePlane->Delete();

    }
  else 
    {
    clipper->SetClipFunction(this->Internal->SlicePlanes);
    }
  return clipper;
}
