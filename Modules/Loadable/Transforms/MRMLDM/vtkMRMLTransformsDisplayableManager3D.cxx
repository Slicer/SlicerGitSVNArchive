/*=========================================================================

  Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Base/GUI/vtkMRMLTransformsDisplayableManager3D.cxx $
  Date:      $Date: 2010-05-27 14:32:23 -0400 (Thu, 27 May 2010) $
  Version:   $Revision: 13525 $

==========================================================================*/

// MRMLLogic includes

// MRMLDisplayableManager includes
#include "vtkMRMLTransformsDisplayableManager3D.h"
#include "TransformsDisplayableManagerHelper.h"

// MRML includes
#include <vtkEventBroker.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLProceduralColorNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformDisplayNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkDataSetAttributes.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

// STD includes
#include <cassert>

//---------------------------------------------------------------------------
vtkStandardNewMacro ( vtkMRMLTransformsDisplayableManager3D );

//---------------------------------------------------------------------------
class vtkMRMLTransformsDisplayableManager3D::vtkInternal
{
public:
  vtkInternal();
  ~vtkInternal();

  // 3D model of the visualized transform
  vtkPolyData* CachedPolyData3d;

  std::map<std::string, vtkProp3D *>               DisplayedActors;
  std::map<std::string, vtkMRMLDisplayNode *>      DisplayedNodes;
  std::map<std::string, int>                       DisplayedVisibility;
  std::map<std::string, vtkMRMLDisplayableNode *>  DisplayableNodes;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLTransformsDisplayableManager3D::vtkInternal::vtkInternal()
{
  this->CachedPolyData3d=vtkPolyData::New();
}

//---------------------------------------------------------------------------
vtkMRMLTransformsDisplayableManager3D::vtkInternal::~vtkInternal()
{
  this->CachedPolyData3d->Delete();
  this->CachedPolyData3d=NULL;
}

//---------------------------------------------------------------------------
// vtkMRMLTransformsDisplayableManager3D methods

//---------------------------------------------------------------------------
vtkMRMLTransformsDisplayableManager3D::vtkMRMLTransformsDisplayableManager3D()
{
  this->Internal = new vtkInternal();
}

//---------------------------------------------------------------------------
vtkMRMLTransformsDisplayableManager3D::~vtkMRMLTransformsDisplayableManager3D()
{
  // release the DisplayedActors
  this->Internal->DisplayedActors.clear();

  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::PrintSelf ( ostream& os, vtkIndent indent )
{
  this->vtkObject::PrintSelf ( os, indent );

  os << indent << "vtkMRMLTransformsDisplayableManager3D: " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::ProcessMRMLNodesEvents(vtkObject *caller,
                                                           unsigned long event,
                                                           void *callData)
{
  if (this->GetMRMLScene() == 0)
    {
    return;
    }
  if ( this->GetInteractor() &&
     this->GetInteractor()->GetRenderWindow() &&
     this->GetInteractor()->GetRenderWindow()->CheckInRenderStatus())
    {
    vtkDebugMacro("skipping ProcessMRMLNodesEvents during render");
    return;
    }

  bool isUpdating = this->GetMRMLScene()->IsBatchProcessing();
  if (vtkMRMLDisplayableNode::SafeDownCast(caller))
    {
    // There is no need to request a render (which can be expensive if the
    // volume rendering is on) if nothing visible has changed.
    bool requestRender = true;
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(caller);
    switch (event)
      {
       case vtkMRMLDisplayableNode::DisplayModifiedEvent:
         // don't go any further if the modified display node is not a transform
         if (!this->IsTransformDisplayable(displayableNode) &&
             !this->IsTransformDisplayable(reinterpret_cast<vtkMRMLDisplayNode*>(callData)))
          {
          requestRender = false;
          break;
          }
      case vtkCommand::ModifiedEvent:
      case vtkMRMLModelNode::PolyDataModifiedEvent:
        requestRender = this->OnMRMLDisplayableNodeModifiedEvent(displayableNode);
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
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UnobserveMRMLScene()
{
  this->RemoveProps();
  this->RemoveDisplayableObservers(1);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneStartClose()
{
  this->RemoveDisplayableObservers(0);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneEndClose()
{
  // Clean
  this->RemoveProps();
  this->RemoveDisplayableObservers(1);

  this->SetUpdateFromMRMLRequested(1);
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateFromMRMLScene()
{
  // UpdateFromMRML will be executed only if there has been some actions
  // during the import that requested it (don't call
  // SetUpdateFromMRMLRequested(1) here, it should be done somewhere else
  // maybe in OnMRMLSceneNodeAddedEvent, OnMRMLSceneNodeRemovedEvent or
  // OnMRMLDisplayableNodeModifiedEvent).
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node->IsA("vtkMRMLDisplayableNode") &&
      !node->IsA("vtkMRMLDisplayNode") )
    {
    return;
    }

  this->SetUpdateFromMRMLRequested(1);

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    return;
    }

  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node->IsA("vtkMRMLDisplayableNode") &&
      !node->IsA("vtkMRMLDisplayNode") )
    {
    return;
    }

  this->SetUpdateFromMRMLRequested(1);

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    return;
    }

  // Node specific processing
  if (node->IsA("vtkMRMLDisplayableNode"))
    {
    this->RemoveDisplayable(vtkMRMLDisplayableNode::SafeDownCast(node));
    }

  this->RequestRender();
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::IsTransformDisplayable(vtkMRMLDisplayableNode* node)const
{
  vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast(node);
  if (!node)
    {
    return false;
    }
  bool displayable = false;
  for (int i = 0; i < node->GetNumberOfDisplayNodes(); ++i)
    {
    displayable |= this->IsTransformDisplayable(node->GetNthDisplayNode(i));
    if (displayable)
      {// Optimization: no need to search any further.
      break;
      }
    }
  return displayable;
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::IsTransformDisplayable(vtkMRMLDisplayNode* node)const
{
  vtkMRMLTransformDisplayNode* displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(node);
  if (!displayNode)
    {
    return false;
    }
  return TransformsDisplayableManagerHelper::GetOutputPolyData(displayNode, this->Internal->CachedPolyData3d) ? true : false;
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::OnMRMLDisplayableNodeModifiedEvent(
    vtkMRMLDisplayableNode * transformNode)
{
  if (!this->IsTransformDisplayable(transformNode))
    {
    return false;
    }
  // If the node is already cached with an actor process only this one
  // If it was not visible and is still not visible do nothing
  int ndnodes = transformNode->GetNumberOfDisplayNodes();
  bool updateModel = false;
  bool updateMRML = false;
  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLDisplayNode *dnode = transformNode->GetNthDisplayNode(i);
    assert(dnode);
    bool visible = (dnode->GetVisibility() == 1) && this->IsTransformDisplayable(dnode);
    bool hasActor = this->Internal->DisplayedActors.find(dnode->GetID()) != this->Internal->DisplayedActors.end();
    // If the displayNode is visible and doesn't have actors yet, then request
    // an updated
    if (visible && !hasActor)
      {
      updateMRML = true;
      break;
      }
    // If the displayNode visibility has changed or displayNode is visible, then
    // update the model.
    if (!(!visible && this->GetDisplayedModelsVisibility(dnode) == 0))
      {
      updateModel = true;
      break;
      }
    }
  if (updateModel)
    {
    this->UpdateModifiedModel(transformNode);
    this->SetUpdateFromMRMLRequested(1);
    }
  if (updateMRML)
    {
    this->SetUpdateFromMRMLRequested(1);
    }
  return updateModel || updateMRML;
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateFromMRML()
{
  if ( this->GetInteractor() &&
       this->GetInteractor()->GetRenderWindow() &&
       this->GetInteractor()->GetRenderWindow()->CheckInRenderStatus())
    {
    vtkDebugMacro("skipping update during render");
    return;
    }

  this->RemoveProps();

  this->UpdateModelsFromMRML();

  this->SetUpdateFromMRMLRequested(0);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateModelsFromMRML()
{
  vtkMRMLScene *scene = this->GetMRMLScene();

  // find volume slices
  bool clearDisplayedModels = scene ? false : true;

  std::vector<vtkMRMLNode *> dnodes;
  int nnodes = scene ? scene->GetNodesByClass("vtkMRMLDisplayableNode", dnodes) : 0;
  for (int n=0; n<nnodes; n++)
    {
    vtkMRMLNode *node = dnodes[n];
    vtkMRMLDisplayableNode *model = vtkMRMLDisplayableNode::SafeDownCast(node);
    }

  if (clearDisplayedModels)
    {
    std::map<std::string, vtkProp3D *>::iterator iter;
    for (iter = this->Internal->DisplayedActors.begin(); iter != this->Internal->DisplayedActors.end(); iter++)
      {
      this->GetRenderer()->RemoveViewProp(iter->second);
      }
    this->RemoveDisplayableObservers(1);
    this->Internal->DisplayedActors.clear();
    this->Internal->DisplayedNodes.clear();
    this->Internal->DisplayedVisibility.clear();
    }

  for (int n=0; n<nnodes; n++)
    {
    vtkMRMLDisplayableNode *model = vtkMRMLDisplayableNode::SafeDownCast(dnodes[n]);
    if (model)
      {
      this->UpdateModifiedModel(model);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateModifiedModel(vtkMRMLDisplayableNode *model)
{
  this->UpdateModel(model);
  this->SetModelDisplayProperty(model);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D
::UpdateModelPolyData(vtkMRMLDisplayableNode *displayableNode)
{
  int ndnodes = displayableNode->GetNumberOfDisplayNodes();

  // if no transform display nodes found, return
  int transformDisplayNodeCount = 0;
  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLDisplayNode *dNode = displayableNode->GetNthDisplayNode(i);
    if (vtkMRMLTransformDisplayNode::SafeDownCast(dNode) != NULL)
      {
      transformDisplayNodeCount++;
      }
    }
  if (transformDisplayNodeCount == 0)
    {
    return;
    }

  vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast(displayableNode);

  bool hasNonLinearTransform = false;
  vtkMRMLTransformNode* tnode = displayableNode->GetParentTransformNode();

  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLTransformDisplayNode *displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(displayableNode->GetNthDisplayNode(i));
    if (displayNode == NULL)
      {
      continue;
      }

    int visibility = displayNode->GetVisibility();
    vtkPolyData *polyData = NULL;
    if (this->IsTransformDisplayable(displayNode))
      {
      polyData = TransformsDisplayableManagerHelper::GetOutputPolyData(displayNode, this->Internal->CachedPolyData3d);
      }
    if (polyData == 0)
      {
      continue;
      }

    vtkProp3D* prop = 0;
    std::map<std::string, vtkProp3D *>::iterator ait;
    ait = this->Internal->DisplayedActors.find(displayNode->GetID());
    if (ait == this->Internal->DisplayedActors.end() )
      {
      if (!prop)
        {
        prop = vtkActor::New();
        }
      }
    else
      {
      prop = (*ait).second;
      }

    vtkActor * actor = vtkActor::SafeDownCast(prop);
    if(actor)
      {
      vtkNew<vtkPolyDataMapper> mapper;
      mapper->SetInput(polyData);
      actor->SetMapper(mapper.GetPointer());
      }

    if (ait == this->Internal->DisplayedActors.end())
      {
      this->GetRenderer()->AddViewProp(prop);
      this->Internal->DisplayedActors[displayNode->GetID()] = prop;
      this->Internal->DisplayedNodes[std::string(displayNode->GetID())] = displayNode;

      if (displayNode)
        {
        this->Internal->DisplayedVisibility[displayNode->GetID()] = visibility;
        }
      else
        {
        this->Internal->DisplayedVisibility[displayNode->GetID()] = 1;
        }

      prop->Delete();
      }
    else if (polyData == 0)
      {
      prop->Delete();
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateModel(vtkMRMLDisplayableNode *model)
{
  this->UpdateModelPolyData(model);

  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  // observe polydata;
  if (!broker->GetObservationExist(model, vtkMRMLModelNode::PolyDataModifiedEvent,
                                         this, this->GetMRMLNodesCallbackCommand()))
    {
    broker->AddObservation(model, vtkMRMLModelNode::PolyDataModifiedEvent,
                           this, this->GetMRMLNodesCallbackCommand());
    this->Internal->DisplayableNodes[model->GetID()] = model;
    }
  // observe display node
  if (!broker->GetObservationExist(model, vtkMRMLDisplayableNode::DisplayModifiedEvent,
                                         this, this->GetMRMLNodesCallbackCommand()))
    {
    broker->AddObservation(model, vtkMRMLDisplayableNode::DisplayModifiedEvent,
                           this, this->GetMRMLNodesCallbackCommand());
    }

  if (!broker->GetObservationExist(model, vtkMRMLTransformableNode::TransformModifiedEvent,
                                         this, this->GetMRMLNodesCallbackCommand()))
    {
    broker->AddObservation(model, vtkMRMLTransformableNode::TransformModifiedEvent,
                           this, this->GetMRMLNodesCallbackCommand());
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveProps()
{
  std::map<std::string, vtkProp3D *>::iterator iter;
  std::map<std::string, int>::iterator clipIter;
  std::vector<std::string> removedIDs;
  for(iter=this->Internal->DisplayedActors.begin(); iter != this->Internal->DisplayedActors.end(); iter++)
    {
    vtkMRMLDisplayNode *displayNode = vtkMRMLDisplayNode::SafeDownCast(
      this->GetMRMLScene() ? this->GetMRMLScene()->GetNodeByID(iter->first) : 0);
    if (displayNode == 0)
      {
      this->GetRenderer()->RemoveViewProp(iter->second);
      removedIDs.push_back(iter->first);
      }
    }
  for (unsigned int i=0; i< removedIDs.size(); i++)
    {
    this->RemoveDispalyedID(removedIDs[i]);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveDisplayable(vtkMRMLDisplayableNode* model)
{
  if (!model)
    {
    return;
    }
  const int ndnodes = model->GetNumberOfDisplayNodes();
  std::vector<std::string> removedIDs;
  for (int i=0; i<ndnodes; i++)
    {
    const char* displayNodeIDToRemove = model->GetNthDisplayNodeID(i);
    std::map<std::string, vtkProp3D *>::iterator iter =
      this->Internal->DisplayedActors.find(displayNodeIDToRemove);
    if (iter != this->Internal->DisplayedActors.end())
      {
      this->GetRenderer()->RemoveViewProp(iter->second);
      removedIDs.push_back(iter->first);
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
void vtkMRMLTransformsDisplayableManager3D::RemoveDispalyedID(std::string &id)
{
  std::map<std::string, vtkMRMLDisplayNode *>::iterator modelIter;
  this->Internal->DisplayedActors.erase(id);
  this->Internal->DisplayedVisibility.erase(id);
  modelIter = this->Internal->DisplayedNodes.find(id);
  if(modelIter != this->Internal->DisplayedNodes.end())
    {
    //this->RemoveDisplayableObservers(modelIter->second);
    this->Internal->DisplayedNodes.erase(modelIter->first);
    }
}

//---------------------------------------------------------------------------
int vtkMRMLTransformsDisplayableManager3D::GetDisplayedModelsVisibility(vtkMRMLDisplayNode *model)
{
  int visibility = 0;

  std::map<std::string, int>::iterator iter;
  iter = this->Internal->DisplayedVisibility.find(model->GetID());
  if (iter != this->Internal->DisplayedVisibility.end())
    {
    visibility = iter->second;
    }

  return visibility;
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveMRMLObservers()
{
  this->RemoveDisplayableObservers(1);

  this->Superclass::RemoveMRMLObservers();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveDisplayableObservers(int clearCache)
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
    this->Internal->DisplayedVisibility.clear();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveDisplayableNodeObservers(vtkMRMLDisplayableNode *model)
{
  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  if (model != 0)
    {
    observations = broker->GetObservations(
      model, vtkMRMLModelNode::PolyDataModifiedEvent, this, this->GetMRMLNodesCallbackCommand() );
    broker->RemoveObservations(observations);
    observations = broker->GetObservations(
      model, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, this->GetMRMLNodesCallbackCommand() );
    broker->RemoveObservations(observations);
    observations = broker->GetObservations(
      model, vtkMRMLTransformableNode::TransformModifiedEvent, this, this->GetMRMLNodesCallbackCommand() );
    broker->RemoveObservations(observations);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::SetModelDisplayProperty(vtkMRMLDisplayableNode *model)
{
  vtkMRMLTransformNode* tnode = model->GetParentTransformNode();

  int ndnodes = model->GetNumberOfDisplayNodes();
  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLDisplayNode *thisDisplayNode = model->GetNthDisplayNode(i);
    vtkMRMLDisplayNode *displayNode = thisDisplayNode;
    if (thisDisplayNode != 0)
      {
      vtkProp3D *prop = this->GetActorByID(thisDisplayNode->GetID());
      if (prop == 0)
        {
        continue;
        }

      vtkActor *actor = vtkActor::SafeDownCast(prop);

      bool visible = displayNode->GetVisibility(this->GetMRMLViewNode()->GetID());
      prop->SetVisibility(visible);
      this->Internal->DisplayedVisibility[displayNode->GetID()] = visible;

      if (actor)
        {
        actor->GetMapper()->SetScalarVisibility(displayNode->GetScalarVisibility());
        // if the scalars are visible, set active scalars, try to get the lookup
        // table
        if (displayNode->GetScalarVisibility())
          {
          if (displayNode->GetColorNode() != 0)
            {
            vtkMRMLProceduralColorNode* proceduralColorNode =
              vtkMRMLProceduralColorNode::SafeDownCast(displayNode->GetColorNode());
            if (proceduralColorNode &&
                proceduralColorNode->GetColorTransferFunction() != 0)
              {
              // \tbd maybe the trick below should be applied here too
              vtkNew<vtkColorTransferFunction> ctf;
              ctf->DeepCopy(proceduralColorNode->GetColorTransferFunction());
              actor->GetMapper()->SetLookupTable(ctf.GetPointer());
              }
            else if (displayNode->GetColorNode()->GetLookupTable() != 0)
              {
              // \tbd: Could slow down if done too often
              // copy lut so that they are not shared between the mappers
              // vtk sets scalar range on lut while rendering
              // that may cause performance problem if lut's are shared
              vtkNew<vtkLookupTable> lut;
              lut->DeepCopy( displayNode->GetColorNode()->GetLookupTable());
              actor->GetMapper()->SetLookupTable(lut.GetPointer());
              }
            }

          actor->GetMapper()->SelectColorArray(TransformsDisplayableManagerHelper::GetDisplacementMagnitudeScalarName());
          // set the scalar range
          actor->GetMapper()->SetScalarRange(displayNode->GetScalarRange());

          actor->GetMapper()->SetScalarModeToUsePointData();
          actor->GetMapper()->SetColorModeToMapScalars();

          actor->GetMapper()->UseLookupTableScalarRangeOff();
          actor->GetMapper()->SetScalarRange(displayNode->GetScalarRange());
          }
          //// }
        actor->GetProperty()->SetRepresentation(displayNode->GetRepresentation());
        actor->GetProperty()->SetPointSize(displayNode->GetPointSize());
        actor->GetProperty()->SetLineWidth(displayNode->GetLineWidth());
        actor->GetProperty()->SetLighting(displayNode->GetLighting());
        actor->GetProperty()->SetInterpolation(displayNode->GetInterpolation());
        actor->GetProperty()->SetShading(displayNode->GetShading());
        actor->GetProperty()->SetFrontfaceCulling(displayNode->GetFrontfaceCulling());
        actor->GetProperty()->SetBackfaceCulling(displayNode->GetBackfaceCulling());

        if (displayNode->GetSelected())
          {
          vtkDebugMacro("Model display node " << displayNode->GetName() << " is selected...");
          actor->GetProperty()->SetColor(displayNode->GetSelectedColor());
          actor->GetProperty()->SetAmbient(displayNode->GetSelectedAmbient());
          actor->GetProperty()->SetSpecular(displayNode->GetSelectedSpecular());
          }
        else
          {
          //vtkWarningMacro("Model display node " << displayNode->GetName() << " is not selected...");
          actor->GetProperty()->SetColor(displayNode->GetColor());
          actor->GetProperty()->SetAmbient(displayNode->GetAmbient());
          actor->GetProperty()->SetSpecular(displayNode->GetSpecular());
          }
        actor->GetProperty()->SetOpacity(displayNode->GetOpacity());
        actor->GetProperty()->SetDiffuse(displayNode->GetDiffuse());
        actor->GetProperty()->SetSpecularPower(displayNode->GetPower());
        actor->GetProperty()->SetEdgeVisibility(displayNode->GetEdgeVisibility());
        actor->GetProperty()->SetEdgeColor(displayNode->GetEdgeColor());

        actor->SetTexture(0);
        }
      }
    }

}

// Description:
// return the current actor corresponding to a give MRML ID
vtkProp3D * vtkMRMLTransformsDisplayableManager3D::GetActorByID(const char *id)
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
const char * vtkMRMLTransformsDisplayableManager3D::GetIDByActor(vtkProp3D *actor)
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
