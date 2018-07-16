/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLViewLogic.cxx,v $
  Date:      $Date$
  Version:   $Revision$

=========================================================================auto=*/

// MRMLLogic includes
#include "vtkMRMLViewLogic.h"
#include "vtkMRMLSliceLayerLogic.h"

// MRML includes
#include <vtkEventBroker.h>
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkAlgorithmOutput.h>
#include <vtkCallbackCommand.h>
#include <vtkCollection.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkVersion.h>

// VTKAddon includes
#include <vtkAddonMathUtilities.h>

// STD includes
#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLViewLogic);

//----------------------------------------------------------------------------
vtkMRMLViewLogic::vtkMRMLViewLogic()
{
  this->Name = 0;
  this->SetName("");
  this->ViewNode = 0;
  this->CameraNode = 0;
}

//----------------------------------------------------------------------------
vtkMRMLViewLogic::~vtkMRMLViewLogic()
{
  if (this->Name)
    {
    delete [] this->Name;
    }
  this->Name = 0;

  if (this->CameraNode)
    {
    vtkSetAndObserveMRMLNodeMacro(this->CameraNode, 0);
    this->CameraNode = 0;
    }

  if (this->ViewNode)
    {
    vtkSetAndObserveMRMLNodeMacro(this->ViewNode, 0);
    this->ViewNode = 0;
  }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::SetMRMLSceneInternal(vtkMRMLScene *newScene)
{
  // Sanity checks
  if (!this->GetName() || strlen(this->GetName()) == 0)
    {
    vtkErrorMacro(<< "Name is NULL - Make sure you call SetName before SetMRMLScene !");
    return;
    }

  // List of events the slice logics should listen
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);

  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());

  this->ProcessMRMLSceneEvents(newScene, vtkMRMLScene::EndBatchProcessEvent, 0);
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::OnMRMLSceneNodeAdded(vtkMRMLNode *node)
{
  if (node->IsA("vtkMRMLViewNode") || node->IsA("vtkMRMLCameraNode"))
    {
    this->UpdateMRMLNodes();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode *node)
{
  if (node->IsA("vtkMRMLViewNode") || node->IsA("vtkMRMLCameraNode"))
    {
    this->UpdateMRMLNodes();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::UpdateFromMRMLScene()
{
  this->UpdateMRMLNodes();
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::UpdateMRMLNodes()
{
  if (this->GetMRMLScene()
      && this->GetMRMLScene()->IsBatchProcessing())
    {
    return;
    }

  // Set up the nodes
  this->UpdateViewNode();
  this->UpdateCameraNode();
}

//----------------------------------------------------------------------------
vtkMRMLViewNode *vtkMRMLViewLogic::GetViewNode(vtkMRMLScene *scene, const char *layoutName)
{
  if (!scene || !layoutName)
    {
    return 0;
    }

  vtkSmartPointer<vtkCollection> viewNodes = vtkSmartPointer<vtkCollection>::Take
      (scene->GetNodesByClass("vtkMRMLViewNode"));
  for(int viewNodeIndex = 0; viewNodeIndex < viewNodes->GetNumberOfItems(); viewNodeIndex++)
    {
    vtkMRMLViewNode* viewNode =
        vtkMRMLViewNode::SafeDownCast(viewNodes->GetItemAsObject(viewNodeIndex));
    if (viewNode &&
        viewNode->GetLayoutName() &&
        !strcmp(viewNode->GetLayoutName(), layoutName))
      {
      return viewNode;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkMRMLCameraNode *vtkMRMLViewLogic::GetCameraNode(vtkMRMLScene *scene, const char *layoutName)
{
  if (!scene || !layoutName)
    {
    return 0;
    }

  vtkSmartPointer<vtkCollection> cameraNodes = vtkSmartPointer<vtkCollection>::Take
      (scene->GetNodesByClass("vtkMRMLCameraNode"));
  for (int cameraIndex = 0; cameraIndex < cameraNodes->GetNumberOfItems(); cameraIndex++)
    {
    vtkMRMLCameraNode* cameraNode =
        vtkMRMLCameraNode::SafeDownCast(cameraNodes->GetItemAsObject(cameraIndex));
    if (!cameraNode || !cameraNode->GetActiveTag())
      {
      continue;
      }
    vtkSmartPointer<vtkCollection> viewNodes = vtkSmartPointer<vtkCollection>::Take
        (scene->GetNodesByClass("vtkMRMLViewNode"));
    for(int viewNodeIndex = 0; viewNodeIndex < viewNodes->GetNumberOfItems(); viewNodeIndex++)
      {
      vtkMRMLViewNode* viewNode =
          vtkMRMLViewNode::SafeDownCast(viewNodes->GetItemAsObject(viewNodeIndex));
      if (viewNode &&  viewNode->GetLayoutName() &&
          !strcmp(viewNode->GetLayoutName(), layoutName) &&
          !strcmp(cameraNode->GetActiveTag(), viewNode->GetID()))
        {
        return cameraNode;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::SetCameraNode(vtkMRMLCameraNode *newCameraNode)
{
  if (this->CameraNode == newCameraNode)
    {
    return;
    }

  // Observe the camera node for general properties.
  vtkSetAndObserveMRMLNodeMacro(this->CameraNode, newCameraNode);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::SetViewNode(vtkMRMLViewNode *newViewNode)
{
  if (this->ViewNode == newViewNode)
    {
    return;
    }

  // Observe the view node for general properties.
  vtkSetAndObserveMRMLNodeMacro(this->ViewNode, newViewNode);
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkIndent nextIndent;
  nextIndent = indent.GetNextIndent();

  os << indent << "SlicerViewLogic:             " << this->GetClassName() << "\n";

  if (this->CameraNode)
    {
    os << indent << "CameraNode: ";
    os << (this->CameraNode->GetID() ? this->CameraNode->GetID() : "(0 ID)") << "\n";
    this->CameraNode->PrintSelf(os, nextIndent);
    }
  else
    {
    os << indent << "CameraNode: (none)\n";
    }

  if (this->ViewNode)
    {
    os << indent << "ViewNode: ";
    os << (this->ViewNode->GetID() ? this->ViewNode->GetID() : "(0 ID)") << "\n";
    this->ViewNode->PrintSelf(os, nextIndent);
    }
  else
    {
    os << indent << "ViewNode: (none)\n";
    }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::StartViewNodeInteraction(unsigned int parameters)
{
  if (!this->ViewNode)
    {
    return;
    }

  // Cache the flags on what parameters are going to be modified. Need
  // to this this outside the conditional on LinkedControl
  this->ViewNode->SetInteractionFlags(parameters);

  // If we have linked controls, then we want to broadcast changes
  if (this->ViewNode->GetLinkedControl())
    {
    // Activate interaction
    this->ViewNode->InteractingOn();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::EndViewNodeInteraction()
{
  if (!this->ViewNode)
    {
    return;
    }

  // If we have linked controls, then we want to broadcast changes
  if (this->ViewNode->GetLinkedControl())
    {
    // Need to trigger a final message to broadcast to all the nodes
    // that are linked
    this->ViewNode->InteractingOn();
    this->ViewNode->Modified();
    this->ViewNode->InteractingOff();
    this->ViewNode->SetInteractionFlags(0);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::UpdateCameraNode()
{
  if (!this->GetMRMLScene())
    {
    this->SetCameraNode(0);
    return;
    }

  // find ViewNode in the scene
  vtkMRMLCameraNode *node = vtkMRMLViewLogic::GetCameraNode(this->GetMRMLScene(), this->GetName());

  if (this->CameraNode != 0 && node != 0 &&
      (this->CameraNode->GetID() == 0 ||
      strcmp(this->CameraNode->GetID(), node->GetID()) != 0))
    {
    // local ViewNode is out of sync with the scene
    this->SetCameraNode(0);
    }

  // find ViewNode in the scene
  vtkMRMLViewNode *viewNode = vtkMRMLViewLogic::GetViewNode(this->GetMRMLScene(), this->GetName());

  if (this->CameraNode == 0)
    {
    if (node == 0 && viewNode)
      {
      // Use CreateNodeByClass instead of New to use default node specified in the scene
      node = vtkMRMLCameraNode::SafeDownCast(this->GetMRMLScene()->CreateNodeByClass("vtkMRMLCameraNode"));
      node ->SetActiveTag(viewNode->GetID());
      this->SetCameraNode(node);
      node->Delete();
      }
    else
      {
      this->SetCameraNode(node);
      }
    }

  if (this->GetMRMLScene()->GetNodeByID(this->CameraNode->GetID()) == 0)
    {
    // local node not in the scene
    node = this->CameraNode;
    node->Register(this);
    this->SetCameraNode(0);
    this->GetMRMLScene()->AddNode(node);
    this->SetCameraNode(node);
    node->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::UpdateViewNode()
{
  if (!this->GetMRMLScene())
    {
    this->SetViewNode(0);
    return;
    }

  // find ViewNode in the scene
  vtkMRMLViewNode *node = vtkMRMLViewLogic::GetViewNode(this->GetMRMLScene(), this->GetName());

  if (this->ViewNode != 0 && node != 0 &&
      (this->ViewNode->GetID() == 0 ||
      strcmp(this->ViewNode->GetID(), node->GetID()) != 0))
    {
    // local ViewNode is out of sync with the scene
    this->SetViewNode(0);
    }

  if (this->ViewNode == 0)
    {
    if (node == 0)
      {
      // Use CreateNodeByClass instead of New to use default node specified in the scene
      node = vtkMRMLViewNode::SafeDownCast(this->GetMRMLScene()->CreateNodeByClass("vtkMRMLViewNode"));
      node->SetLayoutName(this->GetName());
      this->SetViewNode(node);
      node->Delete();
      }
    else
      {
      this->SetViewNode(node);
      }
    }

  if (this->GetMRMLScene()->GetNodeByID(this->ViewNode->GetID()) == 0)
    {
    // local node not in the scene
    node = this->ViewNode;
    node->Register(this);
    this->SetViewNode(0);
    this->GetMRMLScene()->AddNode(node);
    this->SetViewNode(node);
    node->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::StartCameraNodeInteraction(unsigned int parameters)
{
  if (!this->ViewNode || !this->CameraNode)
    {
    return;
    }

  // Cache the flags on what parameters are going to be modified. Need
  // to this this outside the conditional on LinkedControl
  this->CameraNode->SetInteractionFlags(parameters);

  // If we have hot linked controls, then we want to broadcast changes
  if (this->ViewNode->GetLinkedControl())
    {
    // Activate interaction
    this->CameraNode->InteractingOn();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLViewLogic::EndCameraNodeInteraction()
{
  if (!this->ViewNode || !this->CameraNode)
    {
    return;
    }

  // If we have linked controls, then we want to broadcast changes
  if (this->ViewNode->GetLinkedControl())
    {
    // Need to trigger a final message to broadcast to all the nodes
    // that are linked
    this->CameraNode->InteractingOn();
    this->CameraNode->Modified();
    this->CameraNode->InteractingOff();
    this->CameraNode->SetInteractionFlags(0);
    }
}
