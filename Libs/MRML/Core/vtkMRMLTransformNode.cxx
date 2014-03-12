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
  this->TransformToParent=NULL;
  this->TransformFromParent=NULL;
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

  if (this->ReadWriteAsTransformToParent)
    {
    vtkNew<vtkGeneralTransform> transform;
    //ConcatenateTransformCopy(transform,node->TransformToParent); TODO: implement this!
    this->SetAndObserveTransformToParent(transform.GetPointer());
    }
  else
    {
    vtkNew<vtkGeneralTransform> transform;
    //ConcatenateTransformCopy(transform,node->TransformFromParent); TODO: implement this!
    this->SetAndObserveTransformFromParent(transform.GetPointer());
    }

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
vtkAbstractTransform* vtkMRMLTransformNode::GetTransformToParent()
{
  if (this->TransformToParent)
    {
    return this->TransformToParent;
    }
  else if (this->TransformFromParent)
    {
    return this->TransformFromParent->GetInverse();
    }
  else
    {
    return NULL;
    }
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkMRMLTransformNode::GetTransformFromParent()
{
  if (this->TransformFromParent)
    {
    return this->TransformFromParent;
    }
  else if (this->TransformToParent)
    {
    return this->TransformToParent->GetInverse();
    }
  else
    {
    return NULL;
    }
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
  if (this->TransformToParent)
    {
    this->TransformToParent->Concatenate(transform);
    }
  else if (this->TransformFromParent)
    {
    this->TransformFromParent->Inverse();
    this->TransformFromParent->Concatenate(transform);
    this->TransformFromParent->Inverse();
    }
  else
    {
    vtkNew<vtkGeneralTransform> newTransform;
    vtkSetAndObserveMRMLObjectMacro(this->TransformToParent, newTransform.GetPointer());
    this->TransformToParent->Concatenate(transform);
    }
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLTransformNode::CreateDefaultStorageNode()
{
  return vtkMRMLTransformStorageNode::New();
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformNode::GetModifiedSinceRead()
{
  if (this->Superclass::GetModifiedSinceRead())
    {
    return true;
    }
  if (ReadWriteAsTransformToParent)
    {
    if (this->TransformToParent && this->TransformToParent->GetMTime() > this->GetStoredTime())
      {
      return true;
      }
    }
  else
    {
    if (this->TransformFromParent && this->TransformFromParent->GetMTime() > this->GetStoredTime())
      {
      return true;
      }
    }
  return false;
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
vtkAbstractTransform* vtkMRMLTransformNode::GetAbstractTransformAs(vtkGeneralTransform* generalTransform, const char* transformClassName)
{
  if (transformClassName==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: transformClassName is invalid");
    return NULL;
    }
  if (generalTransform==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: generalTransform is invalid");
    return NULL;
    }
  if (generalTransform->GetNumberOfConcatenatedTransforms()==0)
    {
    // no transform is defined
    return NULL;
    }
  if (generalTransform->GetNumberOfConcatenatedTransforms()>1)
    {
    std::stringstream ss;
    for (int i=0; i<generalTransform->GetNumberOfConcatenatedTransforms(); i++)
      {
      const char* className=generalTransform->GetConcatenatedTransform(i)->GetClassName();
      ss << " " << (className?className:"undefined");
      }
    ss << std::ends;
    vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: "<<generalTransform->GetNumberOfConcatenatedTransforms()
      <<" transforms are saved in a single node:"<<ss.str());
    return NULL;
    }
  vtkAbstractTransform* transform=generalTransform->GetConcatenatedTransform(0);
  if (transform==NULL)
  {
    vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: the stored transform is invalid");
    return NULL;
  }
  if (!transform->IsA(transformClassName))
  {
    vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: expected a "<<transformClassName<<" type class and found "<<transform->GetClassName()<<" instead");
    return NULL;
  }
  return transform;
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkMRMLTransformNode::GetTransformToParentAs(const char* transformClassName)
{
  if (this->TransformToParent)
    {
    return GetAbstractTransformAs(this->TransformToParent, transformClassName);
    }
  else if (this->TransformFromParent)
    {
    vtkAbstractTransform *transform = GetAbstractTransformAs(this->TransformFromParent, transformClassName);
    if (transform!=NULL)
      {
      return transform->GetInverse();
      }
    }
 return NULL;
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkMRMLTransformNode::GetTransformFromParentAs(const char* transformClassName)
{
  if (this->TransformFromParent)
    {
    return GetAbstractTransformAs(this->TransformFromParent, transformClassName);
    }
  else if (this->TransformToParent)
    {
    vtkAbstractTransform *transform = GetAbstractTransformAs(this->TransformToParent, transformClassName);
    if (transform!=NULL)
      {
      return transform->GetInverse();
      }
    }
 return NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::SetAndObserveTransform(vtkGeneralTransform** originalTransformPtr, vtkGeneralTransform** inverseTransformPtr, vtkAbstractTransform *transform)
{
  if ((*originalTransformPtr)!=NULL
    && (*originalTransformPtr)->GetNumberOfConcatenatedTransforms()==1
    && (*originalTransformPtr)->GetConcatenatedTransform(0)==transform
    && !(*originalTransformPtr)->GetInverseFlag())
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
    // Reset the transform
    if ((*originalTransformPtr))
      {
      (*originalTransformPtr)->Identity();
      if ((*originalTransformPtr)->GetInverseFlag())
        {
        // the inverse flag was set, so turn it off
        (*originalTransformPtr)->Inverse();
        }
      }
    else
      {
      vtkNew<vtkGeneralTransform> newTransform;
      vtkSetAndObserveMRMLObjectMacro((*originalTransformPtr), newTransform.GetPointer());
      }

    (*originalTransformPtr)->Concatenate(transform);

    // We set the inverse to NULL, which means that it's unknown and will be computed atuomatically from the original transform
    vtkSetAndObserveMRMLObjectMacro((*inverseTransformPtr), NULL);

    //(*inverseTransformPtr)->Update();
    }
  else
    {
    // Clear TransformFromParent and TransformToParent
    vtkSetAndObserveMRMLObjectMacro((*originalTransformPtr), NULL);
    vtkSetAndObserveMRMLObjectMacro((*inverseTransformPtr), NULL);
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
void vtkMRMLTransformNode::SetAndObserveTransformToParent(vtkAbstractTransform *transform)
{
  // Save the transform as it was set (inverse could be saved as well but that might be inaccurate)
  this->ReadWriteAsTransformToParent=true;
  SetAndObserveTransform(&(this->TransformToParent), &(this->TransformFromParent), transform);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::SetAndObserveTransformFromParent(vtkAbstractTransform *transform)
{
  // Save the transform as it was set (inverse could be saved as well but that might be inaccurate)
  this->ReadWriteAsTransformToParent=false;
  SetAndObserveTransform(&(this->TransformFromParent), &(this->TransformToParent), transform);
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
void vtkMRMLTransformNode::UpdateTransformObservers(vtkCollection* observedTransforms, vtkGeneralTransform* generalTransform)
{
  // Quickly check if anything has changed and return if no changes are necessary
  if (generalTransform!=NULL)
    {
    if (observedTransforms->GetNumberOfItems()==generalTransform->GetNumberOfConcatenatedTransforms())
      {
        bool changed=false;
        for (int i=0; i<generalTransform->GetNumberOfConcatenatedTransforms(); i++)
        {
          if (generalTransform->GetConcatenatedTransform(i)!=observedTransforms->GetItemAsObject(i))
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
    }
  else if (observedTransforms->GetNumberOfItems()==0)
    {
    return;
    }

  // Remove old observers
  vtkCollectionIterator* iter = observedTransforms->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    this->MRMLObserverManager->RemoveObjectEvents(iter->GetCurrentObject());
    }
  iter->Delete();
  observedTransforms->RemoveAllItems();

  // Add new observers
  if (generalTransform!=NULL)
  {
    for (int i=0; i<generalTransform->GetNumberOfConcatenatedTransforms(); i++)
      {
      vtkAbstractTransform* transform=generalTransform->GetConcatenatedTransform(i);
      this->MRMLObserverManager->ObserveObject(transform);
      observedTransforms->AddItem(transform);
      }
  }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformNode::UpdateTransformToParentObservers()
{
  UpdateTransformObservers(this->ObservedTransformToParentTransforms, this->TransformToParent);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformNode::UpdateTransformFromParentObservers()
{
  UpdateTransformObservers(this->ObservedTransformFromParentTransforms, this->TransformFromParent);
}
