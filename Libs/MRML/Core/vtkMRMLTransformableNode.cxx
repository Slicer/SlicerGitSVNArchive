/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLTransformNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkIntArray.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkMatrix4x4.h>

//----------------------------------------------------------------------------
vtkCxxSetReferenceStringMacro(vtkMRMLTransformableNode, TransformNodeID);

//----------------------------------------------------------------------------
vtkMRMLTransformableNode::vtkMRMLTransformableNode()
{
  this->HideFromEditors = 0;

  this->TransformNodeID = NULL;
  this->TransformNode = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLTransformableNode::~vtkMRMLTransformableNode()
{
  if (this->TransformNodeID) 
    {
    SetAndObserveTransformNodeID(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformableNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  if (this->TransformNodeID != NULL) 
    {
    of << indent << " transformNodeRef=\"" << this->TransformNodeID << "\"";
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformableNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID, newID);
  if (this->TransformNodeID && !strcmp(oldID, this->TransformNodeID))
    {
    this->SetAndObserveTransformNodeID(newID);
    }
}
//----------------------------------------------------------------------------
void vtkMRMLTransformableNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "transformNodeRef")) 
      {
      this->SetAndObserveTransformNodeID(attValue);
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLTransformableNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLTransformableNode *node = (vtkMRMLTransformableNode *) anode;
  this->SetTransformNodeID(node->TransformNodeID);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformableNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << indent << "TransformNodeID: " <<
    (this->TransformNodeID ? this->TransformNodeID : "(none)") << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLTransformNode* vtkMRMLTransformableNode::GetParentTransformNode()
{
  vtkMRMLTransformNode* node = NULL;
  if (this->GetScene() && this->TransformNodeID != NULL )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->TransformNodeID);
    node = vtkMRMLTransformNode::SafeDownCast(snode);
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLTransformableNode::SetAndObserveTransformNodeID(const char *transformNodeID)
{
  if (transformNodeID == 0 && this->GetTransformNodeID() == 0)
    {
    // was NULL and still NULL, nothing to do
    return;
    }

  vtkMRMLTransformNode* tnode = vtkMRMLTransformNode::SafeDownCast(
    this->GetScene() != 0 ?this->GetScene()->GetNodeByID(transformNodeID) : 0);

  // Prevent transform cycles
  if (tnode && tnode->GetParentTransformNode() == this)
    {
    tnode->SetAndObserveTransformNodeID(0);
    }

  // We don't want people to catch the Modified event generated by
  // SetTransformNodeID until we observe the Transform node.
  int wasModifying = this->StartModify();

  // GetParentTransformNode() only works if the transform node ID is set.
  this->SetTransformNodeID(transformNodeID);

  vtkIntArray *events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  vtkSetAndObserveMRMLObjectEventsMacro(this->TransformNode, tnode, events);
  events->Delete();

  this->EndModify(wasModifying);
  this->InvokeEvent(vtkMRMLTransformableNode::TransformModifiedEvent);

  if (this->Scene)
    {
    this->Scene->AddReferencedNodeID(this->TransformNodeID, this);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformableNode::ProcessMRMLEvents ( vtkObject *caller,
                                                  unsigned long event, 
                                                  void *vtkNotUsed(callData) )
{
  // as retrieving the parent transform node can be costly (browse the scene)
  // do some checks here to prevent retrieving the node for nothing.
  if (caller == NULL ||
      event != vtkMRMLTransformableNode::TransformModifiedEvent)
    {
    return;
    }
  vtkMRMLTransformNode *tnode = this->GetParentTransformNode();
  if (tnode == caller)
    {
    //TODO don't send even on the scene but rather have vtkMRMLSliceLayerLogic listen to
    // TransformModifiedEvent
    //this->GetScene()->InvokeEvent(vtkCommand::ModifiedEvent, NULL);
    this->InvokeEvent(vtkMRMLTransformableNode::TransformModifiedEvent, NULL);
    }
}

//-----------------------------------------------------------
void vtkMRMLTransformableNode::SetSceneReferences()
{
  this->Superclass::SetSceneReferences();
  this->Scene->AddReferencedNodeID(this->TransformNodeID, this);
}

//-----------------------------------------------------------
void vtkMRMLTransformableNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
  this->SetAndObserveTransformNodeID(this->TransformNodeID);
}

//-----------------------------------------------------------
void vtkMRMLTransformableNode::UpdateReferences()
{
   Superclass::UpdateReferences();

  if (this->TransformNodeID != NULL && this->Scene->GetNodeByID(this->TransformNodeID) == NULL)
    {
    this->SetAndObserveTransformNodeID(NULL);
    }
}

//-----------------------------------------------------------
bool vtkMRMLTransformableNode::CanApplyNonLinearTransforms()const
{
  return false;
}

//-----------------------------------------------------------
void vtkMRMLTransformableNode::ApplyTransformMatrix(vtkMatrix4x4* transformMatrix)
{
  vtkMatrixToLinearTransform* transform = vtkMatrixToLinearTransform::New();
  transform->SetInput(transformMatrix);
  this->ApplyTransform(transform);
  transform->Delete();
}

//-----------------------------------------------------------
void vtkMRMLTransformableNode::ApplyTransform(vtkAbstractTransform* transform)
{
  vtkHomogeneousTransform* linearTransform = vtkHomogeneousTransform::SafeDownCast(transform);
  if (linearTransform)
    {
    this->ApplyTransformMatrix(linearTransform->GetMatrix());
    return;
    }
  if (!this->CanApplyNonLinearTransforms())
    {
    vtkErrorMacro("Can't apply a non-linear transform");
    return;
    }
}

//-----------------------------------------------------------
void vtkMRMLTransformableNode::TransformPointToWorld(const double in[4], double out[4])
{
  // get the nodes's transform node
  vtkMRMLTransformNode* tnode = this->GetParentTransformNode();
  if (tnode != NULL && tnode->IsLinear())
    {
    vtkMRMLLinearTransformNode *lnode = vtkMRMLLinearTransformNode::SafeDownCast(tnode);
    vtkMatrix4x4* transformToWorld = vtkMatrix4x4::New();
    transformToWorld->Identity();
    lnode->GetMatrixTransformToWorld(transformToWorld);
    transformToWorld->MultiplyPoint(in, out);
    transformToWorld->Delete();
    }
  else if (tnode == NULL)
    {
    for (int i=0; i<4; i++)
      {
      out[i] = in[i];
      }
    }
  else 
    {
    vtkErrorMacro("TransformPointToWorld: not a linear transform");
    }
}

//-----------------------------------------------------------
void vtkMRMLTransformableNode::TransformPointFromWorld(const double in[4], double out[4])
{
  // get the nodes's transform node
  vtkMRMLTransformNode* tnode = this->GetParentTransformNode();
  if (tnode != NULL && tnode->IsLinear())
    {
    vtkMRMLLinearTransformNode *lnode = vtkMRMLLinearTransformNode::SafeDownCast(tnode);
    vtkMatrix4x4* transformToWorld = vtkMatrix4x4::New();
    transformToWorld->Identity();
    lnode->GetMatrixTransformToWorld(transformToWorld);
    transformToWorld->Invert();
    transformToWorld->MultiplyPoint(in, out);
    transformToWorld->Delete();
    }
  else if (tnode == NULL)
    {
    for (int i=0; i<4; i++)
      {
      out[i] = in[i];
      }
    }
  else 
    {
    vtkErrorMacro("TransformPointToWorld: not a linear transform");
    }
}
