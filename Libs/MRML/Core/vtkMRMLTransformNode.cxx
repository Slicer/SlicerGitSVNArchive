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
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLTransformStorageNode.h"

// VTK includes
#include <vtkCommand.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkGeneralTransform.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTransformNode);

//----------------------------------------------------------------------------
vtkMRMLTransformNode::vtkMRMLTransformNode()
{
  vtkNew<vtkGeneralTransform> toParent;
  this->TransformToParent=NULL;
  vtkSetAndObserveMRMLObjectMacro(this->TransformToParent, toParent.GetPointer());
  vtkNew<vtkGeneralTransform> fromParent;
  this->TransformFromParent=NULL;
  vtkSetAndObserveMRMLObjectMacro(this->TransformFromParent, fromParent.GetPointer());
  this->ReadWriteAsTransformToParent = 1;
  this->DisableTransformModifiedEvent=0;
  this->TransformModifiedEventPending=0;
  this->ObservedTransformToParentTransforms=vtkCollection::New();
  this->ObservedTransformFromParentTransforms=vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkMRMLTransformNode::~vtkMRMLTransformNode()
{
  // Remove all observers
  vtkCollectionIterator* iter1 = this->ObservedTransformFromParentTransforms->NewIterator();
  for (iter1->InitTraversal(); !iter1->IsDoneWithTraversal(); iter1->GoToNextItem())
    {
    this->MRMLObserverManager->RemoveObjectEvents(iter1->GetCurrentObject());
    }
  iter1->Delete();
  this->ObservedTransformFromParentTransforms->RemoveAllItems();
  vtkCollectionIterator* iter2 = this->ObservedTransformFromParentTransforms->NewIterator();
  for (iter2->InitTraversal(); !iter2->IsDoneWithTraversal(); iter2->GoToNextItem())
    {
    this->MRMLObserverManager->RemoveObjectEvents(iter2->GetCurrentObject());
    }
  iter2->Delete();
  this->ObservedTransformFromParentTransforms->RemoveAllItems();

  vtkSetAndObserveMRMLObjectMacro(this->TransformToParent, NULL);
  vtkSetAndObserveMRMLObjectMacro(this->TransformFromParent, NULL);

  if (this->ObservedTransformToParentTransforms!=NULL)
    {
    this->ObservedTransformToParentTransforms->Delete();
    this->ObservedTransformToParentTransforms=NULL;
    }
  if (this->ObservedTransformFromParentTransforms!=NULL)
    {
    this->ObservedTransformFromParentTransforms->Delete();
    this->ObservedTransformFromParentTransforms=NULL;
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::WriteXML(ostream& of, int nIndent)
{
  vtkIndent indent(nIndent);

  Superclass::WriteXML(of, nIndent);
  of << indent << " readWriteAsTransformToParent=\"" << (this->ReadWriteAsTransformToParent ? "true" : "false") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::ReadXMLAttributes(const char** atts)
{
  int disabledTransformModify = this->StartTransformModify();
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "readWriteAsTransformToParent"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->ReadWriteAsTransformToParent = 1;
        }
      else
        {
        this->ReadWriteAsTransformToParent = 0;
        }
      }
    }

  this->EndModify(disabledModify);
  this->EndTransformModify(disabledTransformModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLTransformNode::Copy(vtkMRMLNode *anode)
{
  int disabledTransformModify = this->StartTransformModify();
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLTransformNode *node = (vtkMRMLTransformNode *) anode;
  if (node==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::Copy: input node type is incompatible");
    return;
    }

  this->SetReadWriteAsTransformToParent(node->GetReadWriteAsTransformToParent());

  this->TransformToParent->DeepCopy(node->TransformToParent);
  this->TransformFromParent->DeepCopy(node->TransformFromParent);

  this->Modified();
  this->TransformModified();

  this->EndModify(disabledModify);
  this->EndTransformModify(disabledTransformModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << indent << "ReadWriteAsTransformToParent: " << this->ReadWriteAsTransformToParent << "\n";
}

//----------------------------------------------------------------------------
vtkGeneralTransform* vtkMRMLTransformNode::GetTransformToParent()
{
  return this->TransformToParent;
}

//----------------------------------------------------------------------------
vtkGeneralTransform* vtkMRMLTransformNode::GetTransformFromParent()
{
  return this->TransformFromParent;
}

//----------------------------------------------------------------------------
int  vtkMRMLTransformNode::IsTransformToWorldLinear()
{
  if (this->IsLinear() == 0) 
    {
    return 0;
    }
  else 
    {
    vtkMRMLTransformNode *parent = this->GetParentTransformNode();
    if (parent != NULL) 
      {
      return parent->IsTransformToWorldLinear();
      }
    else 
      {
      return 1;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::GetTransformToWorld(vtkGeneralTransform* transformToWorld)
{
  if (transformToWorld->GetNumberOfConcatenatedTransforms() == 0) 
    {
    transformToWorld->Identity();
    }

  transformToWorld->PostMultiply();
  transformToWorld->Concatenate(this->GetTransformToParent());

  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL) 
    {
    parent->GetTransformToWorld(transformToWorld);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::GetTransformFromWorld(vtkGeneralTransform* transformToWorld)
{
  if (transformToWorld->GetNumberOfConcatenatedTransforms() == 0)
    {
    transformToWorld->Identity();
    }

  transformToWorld->PreMultiply();
  transformToWorld->Concatenate(this->GetTransformFromParent());

  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL)
    {
    parent->GetTransformFromWorld(transformToWorld);
    }
}

//----------------------------------------------------------------------------
int  vtkMRMLTransformNode::IsTransformToNodeLinear(vtkMRMLTransformNode* node)
{
  if (this->IsTransformNodeMyParent(node)) 
    {
    vtkMRMLTransformNode *parent = this->GetParentTransformNode();
    if (parent != NULL) 
      {
      if (!strcmp(parent->GetID(), node->GetID()) )
        {
        return this->IsLinear();
        }
      else 
        {
        return this->IsLinear() * parent->IsTransformToNodeLinear(node);
        }
      }
    else return this->IsLinear();
    }
  else if (this->IsTransformNodeMyChild(node))
    {
    vtkMRMLTransformNode *parent = node->GetParentTransformNode();
    if (parent != NULL) 
      {
      if (!strcmp(parent->GetID(), this->GetID()) ) 
        {
        return node->IsLinear();
        }
      else 
        {
        return node->IsLinear() * parent->IsTransformToNodeLinear(this);
        }
      }
    else return node->IsLinear();
    }
  else if (this->IsTransformToWorldLinear() == 1 && 
           node->IsTransformToWorldLinear() == 1) 
    {
    return 1;
    }
  else 
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void  vtkMRMLTransformNode::GetTransformToNode(vtkMRMLTransformNode* node, 
                                               vtkGeneralTransform* transformToNode)
{
  if (this->IsTransformNodeMyParent(node)) 
    {
    vtkMRMLTransformNode *parent = this->GetParentTransformNode();
    if (parent != NULL) 
      {
      transformToNode->Concatenate(this->GetTransformToParent());
      if (strcmp(parent->GetID(), node->GetID()) ) 
        {
        this->GetTransformToNode(node, transformToNode);
        }
      }
    else if (this->GetTransformToParent()) 
      {
      transformToNode->Concatenate(this->GetTransformToParent());
      }
    }
  else if (this->IsTransformNodeMyChild(node)) 
    {
    vtkMRMLTransformNode *parent = node->GetParentTransformNode();
    if (parent != NULL) 
      {
      transformToNode->Concatenate(node->GetTransformToParent());
      if (strcmp(parent->GetID(), this->GetID()) ) 
        {
        node->GetTransformToNode(this, transformToNode);
        }
      }
    else if (node->GetTransformToParent()) 
      {
      transformToNode->Concatenate(node->GetTransformToParent());
      }
    }
  else 
    {
    this->GetTransformToWorld(transformToNode);
    vtkGeneralTransform* transformToWorld2 = vtkGeneralTransform::New();
    transformToWorld2->Identity();
    
    node->GetTransformToWorld(transformToWorld2);
    transformToWorld2->Inverse();
    
    transformToNode->Concatenate(transformToWorld2);
    transformToWorld2->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkMRMLTransformNode::IsTransformNodeMyParent(vtkMRMLTransformNode* node)
{
  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL) 
    {
    if (!strcmp(parent->GetID(), node->GetID()) ) 
      {
      return 1;
      }
    else 
      {
      return parent->IsTransformNodeMyParent(node);
      }
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkMRMLTransformNode::IsTransformNodeMyChild(vtkMRMLTransformNode* node)
{
  return node->IsTransformNodeMyParent(this);
}

//----------------------------------------------------------------------------
bool vtkMRMLTransformNode::CanApplyNonLinearTransforms()const
{
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::ApplyTransform(vtkAbstractTransform* transform)
{
  this->TransformToParent->Concatenate(transform); 
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLTransformNode::CreateDefaultStorageNode()
{
  return vtkMRMLTransformStorageNode::New();
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformNode::GetModifiedSinceRead()
{
  return this->Superclass::GetModifiedSinceRead() ||
    (this->TransformToParent &&
     this->TransformToParent->GetMTime() > this->GetStoredTime());
}

//----------------------------------------------------------------------------
int  vtkMRMLTransformNode::GetMatrixTransformToWorld(vtkMatrix4x4* transformToWorld)
{
  // The fact that this method is called means that this is a non-linear transform,
  // so we cannot return the transform as a matrix
  transformToWorld->Identity();
  return 0;
}

//----------------------------------------------------------------------------
int  vtkMRMLTransformNode::GetMatrixTransformToNode(vtkMRMLTransformNode* node,
                                                          vtkMatrix4x4* transformToNode)
{
  // The fact that this method is called means that this is a non-linear transform,
  // so we cannot return the transform as a matrix
  vtkWarningMacro("vtkMRMLTransformNode::GetMatrixTransformToNode failed: this is not a linear transform");
  transformToNode->Identity();
  return 0;
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkMRMLTransformNode::GetTransformToParentAs(const char* transformClassName)
{
  if (transformClassName==NULL)
  {
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformToParentAs failed: transformClassName is invalid");
    return NULL;
  }
  if (this->TransformToParent->GetNumberOfConcatenatedTransforms()==0)
    {
    // no transform is defined
    return NULL;
    }
  if (this->TransformToParent->GetNumberOfConcatenatedTransforms()>1)
    {
    std::stringstream ss;
    for (int i=0; i<this->TransformToParent->GetNumberOfConcatenatedTransforms(); i++)
      {
      const char* className=this->TransformToParent->GetConcatenatedTransform(i)->GetClassName();
      ss << " " << (className?className:"undefined");
      }
    ss << std::ends;
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformToParentAs failed: "<<this->TransformToParent->GetNumberOfConcatenatedTransforms()
      <<" transforms are saved in a single node:"<<ss.str());
    return NULL;
    }
  vtkAbstractTransform* transform=this->TransformToParent->GetConcatenatedTransform(0);
  if (transform==NULL)
  {
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformToParentAs failed: the stored transform is invalid");
    return NULL;
  }
  if (!transform->IsA(transformClassName))
  {
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformToParentAs failed: expected a "<<transformClassName<<" type class and found "<<transform->GetClassName()<<" instead");
    return NULL;
  }
  return transform;
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkMRMLTransformNode::GetTransformFromParentAs(const char* transformClassName)
{
  if (transformClassName==NULL)
  {
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformFromParentAs failed: transformClassName is invalid");
    return NULL;
  }
  if (this->TransformFromParent->GetNumberOfConcatenatedTransforms()==0)
    {
    // no transform is defined
    return NULL;
    }
  if (this->TransformFromParent->GetNumberOfConcatenatedTransforms()>1)
    {
    std::stringstream ss;
    for (int i=0; i<this->TransformFromParent->GetNumberOfConcatenatedTransforms(); i++)
      {
      const char* className=this->TransformFromParent->GetConcatenatedTransform(i)->GetClassName();
      ss << " " << (className?className:"undefined");
      }
    ss << std::ends;
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformFromParentAs failed: "<<this->TransformFromParent->GetNumberOfConcatenatedTransforms()
      <<" transforms are saved in a single node:"<<ss.str());
    return NULL;
    }
  vtkAbstractTransform* transform=this->TransformFromParent->GetConcatenatedTransform(0);
  if (transform==NULL)
  {
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformFromParentAs failed: the stored transform is invalid");
    return NULL;
  }
  if (!transform->IsA(transformClassName))
  {
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformFromParentAs failed: expected a "<<transformClassName<<" type class and found "<<transform->GetClassName()<<" instead");
    return NULL;
  }
  return transform;
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::SetAndObserveTransformToParent(vtkAbstractTransform *transform)
{
  if (this->TransformToParent->GetNumberOfConcatenatedTransforms()==1
    && this->TransformToParent->GetConcatenatedTransform(0)==transform)
    {
    // no change
    return;
    }

  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int disabledTransformModify = this->StartTransformModify();
  int disabledModify = this->StartModify();

  // Update the generic transform
  if (transform!=NULL)
    {
    // Update TransformToParent
    this->TransformToParent->Identity();
    this->TransformToParent->Concatenate(transform);
    this->TransformFromParent->Identity();
    this->TransformFromParent->Concatenate(transform->GetInverse());
    }
  else
    {
    // Clear TransformFromParent and TransformToParent
    this->TransformToParent->Identity();
    this->TransformFromParent->Identity();
    }

  this->UpdateTransformToParentObservers();
  this->UpdateTransformFromParentObservers();

  this->StorableModifiedTime.Modified();

  //this->Modified();
  this->TransformModified();

  this->EndModify(disabledModify);
  this->EndTransformModify(disabledTransformModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::SetAndObserveTransformFromParent(vtkAbstractTransform *transform)
{
  if (this->TransformFromParent->GetNumberOfConcatenatedTransforms()==1
    && this->TransformFromParent->GetConcatenatedTransform(0)==transform)
    {
    // no change
    return;
    }

  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int disabledTransformModify = this->StartTransformModify();
  int disabledModify = this->StartModify();

  // Update the generic transform
  if (transform!=NULL)
    {
    // Update TransformFromParent
    this->TransformFromParent->Identity();
    this->TransformFromParent->Concatenate(transform);
    this->TransformToParent->Identity();
    this->TransformToParent->Concatenate(transform->GetInverse());
    }
  else
    {
    // Clear TransformToParent and TransformFromParent
    this->TransformFromParent->Identity();
    this->TransformToParent->Identity();
    }

  this->UpdateTransformToParentObservers();
  this->UpdateTransformFromParentObservers();

  this->StorableModifiedTime.Modified();

  //this->Modified();
  this->TransformModified();

  this->EndModify(disabledModify);
  this->EndTransformModify(disabledTransformModify);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformNode::ProcessMRMLEvents ( vtkObject *caller,
                                                    unsigned long event,
                                                    void *callData )
{
  Superclass::ProcessMRMLEvents ( caller, event, callData );

  if (event ==  vtkCommand::ModifiedEvent && caller!=NULL /* && this->GetDisableModifiedEvent()==0 */)
    {
    if (caller == this->TransformToParent)
      {
      this->TransformModified();
      this->StorableModifiedTime.Modified();
      }
    else if (caller == this->TransformFromParent)
      {
      this->TransformModified();
      this->StorableModifiedTime.Modified();
      }
    else if (this->ObservedTransformToParentTransforms->IsItemPresent(caller))
      {
      this->TransformModified();
      this->StorableModifiedTime.Modified();
      }
    else if (this->ObservedTransformFromParentTransforms->IsItemPresent(caller))
      {
      this->TransformModified();
      this->StorableModifiedTime.Modified();
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformNode::UpdateTransformToParentObservers()
{
  // Quickly check if anything has changed and return if no changes are necessary
  if (this->ObservedTransformToParentTransforms->GetNumberOfItems()==this->TransformToParent->GetNumberOfConcatenatedTransforms())
    {
      bool changed=false;
      for (int i=0; i<this->TransformToParent->GetNumberOfConcatenatedTransforms(); i++)
      {
        if (this->TransformToParent->GetConcatenatedTransform(i)!=this->ObservedTransformToParentTransforms->GetItemAsObject(i))
          {
          changed=true;
          break;
          }
      }
      if (!changed)
        {
        return;
        }
    }
  // Remove old observers
  vtkCollectionIterator* iter = this->ObservedTransformToParentTransforms->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    this->MRMLObserverManager->RemoveObjectEvents(iter->GetCurrentObject());
    }
  iter->Delete();
  this->ObservedTransformToParentTransforms->RemoveAllItems();
  // Add new observers
  if (this->TransformToParent!=NULL)
  {
    for (int i=0; i<this->TransformToParent->GetNumberOfConcatenatedTransforms(); i++)
      {
      vtkAbstractTransform* transform=this->TransformToParent->GetConcatenatedTransform(i);
      this->MRMLObserverManager->ObserveObject(transform);
      this->ObservedTransformToParentTransforms->AddItem(transform);
      }
  }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformNode::UpdateTransformFromParentObservers()
{
  // Quickly check if anything has changed and return if no changes are necessary
  if (this->ObservedTransformFromParentTransforms->GetNumberOfItems()==this->TransformFromParent->GetNumberOfConcatenatedTransforms())
    {
      bool changed=false;
      for (int i=0; i<this->TransformFromParent->GetNumberOfConcatenatedTransforms(); i++)
      {
        if (this->TransformFromParent->GetConcatenatedTransform(i)!=this->ObservedTransformFromParentTransforms->GetItemAsObject(i))
          {
          changed=true;
          break;
          }
      }
      if (!changed)
        {
        return;
        }
    }
  // Remove old observers
  vtkCollectionIterator* iter = this->ObservedTransformFromParentTransforms->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    this->MRMLObserverManager->RemoveObjectEvents(iter->GetCurrentObject());
    }
  iter->Delete();
  this->ObservedTransformFromParentTransforms->RemoveAllItems();
  // Add new observers
  if (this->TransformFromParent!=NULL)
  {
    for (int i=0; i<this->TransformFromParent->GetNumberOfConcatenatedTransforms(); i++)
      {
      vtkAbstractTransform* transform=this->TransformFromParent->GetConcatenatedTransform(i);
      this->MRMLObserverManager->ObserveObject(transform);
      this->ObservedTransformFromParentTransforms->AddItem(transform);
      }
  }
}
