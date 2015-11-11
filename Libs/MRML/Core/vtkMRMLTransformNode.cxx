/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLTransformNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/

#include "vtkMRMLTransformNode.h"

// MRML includes
#include "vtkMRMLBSplineTransformNode.h"
#include "vtkMRMLGridTransformNode.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTransformStorageNode.h"
#include "vtkMRMLTransformDisplayNode.h"
#include "vtkOrientedBSplineTransform.h"
#include "vtkOrientedGridTransform.h"

// VTK includes
#include <vtkCommand.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkGeneralTransform.h>
#include <vtkImageData.h>
#include <vtkLinearTransform.h>
#include <vtkHomogeneousTransform.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkThinPlateSplineTransform.h>
#include <vtkTransform.h>

// STD includes
#include <sstream>
#include <stack>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTransformNode);

//----------------------------------------------------------------------------
vtkMRMLTransformNode::vtkMRMLTransformNode()
{
  this->TransformToParent=NULL;
  this->TransformFromParent=NULL;
  this->ReadAsTransformToParent=0;

  this->CachedMatrixTransformToParent=vtkMatrix4x4::New();
  this->CachedMatrixTransformFromParent=vtkMatrix4x4::New();
}

//----------------------------------------------------------------------------
vtkMRMLTransformNode::~vtkMRMLTransformNode()
{
  vtkSetAndObserveMRMLObjectMacro(this->TransformToParent, NULL);
  vtkSetAndObserveMRMLObjectMacro(this->TransformFromParent, NULL);

  this->CachedMatrixTransformToParent->Delete();
  this->CachedMatrixTransformToParent=NULL;
  this->CachedMatrixTransformFromParent->Delete();
  this->CachedMatrixTransformFromParent=NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::WriteXML(ostream& of, int nIndent)
{
  vtkIndent indent(nIndent);
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    // For backward compatibility only
    if (!strcmp(attName, "readWriteAsTransformToParent"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->ReadAsTransformToParent = 1;
        }
      else
        {
        this->ReadAsTransformToParent = 0;
        }
      }

    }

  this->EndModify(disabledModify);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformNode::FlattenGeneralTransform(vtkCollection* outputTransformList, vtkAbstractTransform* inputTransform)
{
  outputTransformList->RemoveAllItems();
  if (inputTransform==NULL)
    {
    return;
    }

  vtkGeneralTransform* inputGeneralTransform=vtkGeneralTransform::SafeDownCast(inputTransform);
  if (inputGeneralTransform)
    {
    inputGeneralTransform->Update();
    }

  // Push the transforms onto the stack in reverse order
  std::stack< vtkAbstractTransform* > tstack;
  tstack.push(inputTransform);

  // Write out all the transforms on the stack
  while (!tstack.empty())
    {
    vtkAbstractTransform *transform = tstack.top();
    tstack.pop();
    if (transform->IsA("vtkGeneralTransform"))
      {
      // Decompose general transforms
      vtkGeneralTransform *gtrans = (vtkGeneralTransform *)transform;
      gtrans->Update();
      int n = gtrans->GetNumberOfConcatenatedTransforms();
      while (n > 0)
        {
        tstack.push(gtrans->GetConcatenatedTransform(--n));
        }
      }
    else
      {
      // Simple transform, just add to the list
      outputTransformList->AddItem(transform);
      }
    }
}

//----------------------------------------------------------------------------
int vtkMRMLTransformNode::DeepCopyTransform(vtkAbstractTransform* dst, vtkAbstractTransform* src)
{
  if (src==NULL || dst==NULL)
    {
    // we should log an error, but unfortunately we cannot log a VTK macro in a static method
    return 0;
    }

  if (src->IsA("vtkGeneralTransform"))
    {
    // DeepCopy of GeneralTransforms copies concatenated transforms by reference
    // (so it is actually a shallow copy), we make a true deepcopy

    // Flatten the transform list to make the copying simpler
    vtkGeneralTransform* dstGeneral=vtkGeneralTransform::SafeDownCast(dst);
    vtkNew<vtkCollection> sourceTransformList;
    FlattenGeneralTransform(sourceTransformList.GetPointer(), src);

    // Copy the concatentated transforms
    vtkCollectionSimpleIterator it;
    vtkAbstractTransform* concatenatedTransform = NULL;
    for (sourceTransformList->InitTraversal(it); (concatenatedTransform = vtkAbstractTransform::SafeDownCast(sourceTransformList->GetNextItemAsObject(it))) ;)
      {
      vtkAbstractTransform* concatenatedTransformCopy=concatenatedTransform->MakeTransform();
      DeepCopyTransform(concatenatedTransformCopy,concatenatedTransform);
      dstGeneral->Concatenate(concatenatedTransformCopy);
      concatenatedTransformCopy->Delete();
      }
    }
  else if (src->IsA("vtkBSplineTransform")) // this handles vtkOrientedBSplineTransform as well
    {
    // Fix up the DeepCopy for vtkBSplineTransform (it performs only a ShallowCopy on the coefficient grid)
    dst->DeepCopy(src);
#if (VTK_MAJOR_VERSION <= 5)
    vtkImageData* srcCoefficients=vtkBSplineTransform::SafeDownCast(src)->GetCoefficients();
#else
    vtkImageData* srcCoefficients=vtkBSplineTransform::SafeDownCast(src)->GetCoefficientData();
#endif

    if (srcCoefficients)
      {
      vtkNew<vtkImageData> dstCoefficients;
      dstCoefficients->DeepCopy(srcCoefficients);
#if (VTK_MAJOR_VERSION <= 5)
      vtkBSplineTransform::SafeDownCast(dst)->SetCoefficients(dstCoefficients.GetPointer());
#else
      vtkBSplineTransform::SafeDownCast(dst)->SetCoefficientData(dstCoefficients.GetPointer());
#endif
      }
    }
  else if (src->IsA("vtkGridTransform")) // this handles vtkOrientedGridTransform as well
    {
    // Fix up the DeepCopy for vtkGridTransform (it performs only a ShallowCopy on the displacement grid)
    dst->DeepCopy(src);
    vtkImageData* srcDisplacementGrid=vtkGridTransform::SafeDownCast(src)->GetDisplacementGrid();
    if (srcDisplacementGrid)
      {
      vtkNew<vtkImageData> dstDisplacementGrid;
      dstDisplacementGrid->DeepCopy(srcDisplacementGrid);
#if (VTK_MAJOR_VERSION <= 5)
      vtkGridTransform::SafeDownCast(dst)->SetDisplacementGrid(dstDisplacementGrid.GetPointer());
#else
      vtkGridTransform::SafeDownCast(dst)->SetDisplacementGridData(dstDisplacementGrid.GetPointer());
#endif
      }
    }
  else if (src->IsA("vtkThinPlateSplineTransform"))
    {
    // Fix up the DeepCopy for vtkThinPlateSplineTransform (it performs only a ShallowCopy on the landmark points)
    dst->DeepCopy(src);
    vtkPoints* srcSourceLandmarks=vtkThinPlateSplineTransform::SafeDownCast(src)->GetSourceLandmarks();
    if (srcSourceLandmarks)
      {
      vtkNew<vtkPoints> dstSourceLandmarks;
      dstSourceLandmarks->DeepCopy(srcSourceLandmarks);
      vtkThinPlateSplineTransform::SafeDownCast(dst)->SetSourceLandmarks(dstSourceLandmarks.GetPointer());
      }
    vtkPoints* srcTargetLandmarks=vtkThinPlateSplineTransform::SafeDownCast(src)->GetTargetLandmarks();
    if (srcTargetLandmarks)
      {
      vtkNew<vtkPoints> dstTargetLandmarks;
      dstTargetLandmarks->DeepCopy(srcTargetLandmarks);
      vtkThinPlateSplineTransform::SafeDownCast(dst)->SetTargetLandmarks(dstTargetLandmarks.GetPointer());
      }
    }
  else
    {
    // In other cases the actual deepcopy works well
    dst->DeepCopy(src);
    }

  return 1;
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLTransformNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLTransformNode *node = (vtkMRMLTransformNode *) anode;
  if (node==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::Copy: input node type is incompatible");
    return;
    }

  this->SetReadAsTransformToParent(node->GetReadAsTransformToParent());

  // Unfortunately VTK transform DeepCopy actually performs a shallow copy (only data object
  // pointers are copied, but not the contents itself), so we have to apply our custom DeepCopy
  // operation.
  if (node->TransformToParent)
    {
    vtkAbstractTransform* transformCopy=node->TransformToParent->MakeTransform();
    DeepCopyTransform(transformCopy,node->TransformToParent);
    vtkSetAndObserveMRMLObjectMacro(this->TransformToParent, transformCopy);
    transformCopy->Delete();
    }
  if (node->TransformFromParent)
    {
    vtkAbstractTransform* transformCopy=node->TransformFromParent->MakeTransform();
    DeepCopyTransform(transformCopy,node->TransformFromParent);
    vtkSetAndObserveMRMLObjectMacro(this->TransformFromParent, transformCopy);
    transformCopy->Delete();
    }

  this->Modified();
  this->TransformModified();

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << indent << "ReadAsTransformToParent: " << this->ReadAsTransformToParent << "\n";

  // Flatten the transform list to make the copying simpler
  if (this->TransformToParent)
    {
    os << indent << "TransformToParent: \n";
    vtkNew<vtkCollection> sourceTransformList;
    FlattenGeneralTransform(sourceTransformList.GetPointer(), this->TransformToParent);
    vtkCollectionSimpleIterator it;
    vtkAbstractTransform* concatenatedTransform = NULL;
    for (sourceTransformList->InitTraversal(it); (concatenatedTransform = vtkAbstractTransform::SafeDownCast(sourceTransformList->GetNextItemAsObject(it))) ;)
      {
      os << indent.GetNextIndent() << "Transform: "<<concatenatedTransform->GetClassName()<<"\n";
      concatenatedTransform->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
      }
    }

  // Flatten the transform list to make the copying simpler
  if (this->TransformFromParent)
    {
    os << indent << "TransformFromParent: \n";
    vtkNew<vtkCollection> sourceTransformList;
    FlattenGeneralTransform(sourceTransformList.GetPointer(), this->TransformFromParent);
    vtkCollectionSimpleIterator it;
    vtkAbstractTransform* concatenatedTransform = NULL;
    for (sourceTransformList->InitTraversal(it); (concatenatedTransform = vtkAbstractTransform::SafeDownCast(sourceTransformList->GetNextItemAsObject(it))) ;)
      {
      os << indent.GetNextIndent() << "Transform: "<<concatenatedTransform->GetClassName()<<"\n";
      concatenatedTransform->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
      }
    }

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
  if (!this->IsLinear())
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
  if (transformToWorld==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformToWorld failed: transformToWorld is invalid");
    return;
    }

  if (transformToWorld->GetNumberOfConcatenatedTransforms() == 0)
    {
    transformToWorld->Identity();
    }

  transformToWorld->PostMultiply();

  vtkAbstractTransform* transformToParent=this->GetTransformToParent();
  if (transformToParent!=NULL)
    {
    transformToWorld->Concatenate(transformToParent);
    }

  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL)
    {
    parent->GetTransformToWorld(transformToWorld);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::GetTransformFromWorld(vtkGeneralTransform* transformFromWorld)
{
  if (transformFromWorld==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::GetTransformFromWorld failed: transformToWorld is invalid");
    return;
    }

  if (transformFromWorld->GetNumberOfConcatenatedTransforms() == 0)
    {
    transformFromWorld->Identity();
    }

  transformFromWorld->PreMultiply();

  vtkAbstractTransform* transformFromParent=this->GetTransformFromParent();
  if (transformFromParent!=NULL)
    {
    transformFromWorld->Concatenate(transformFromParent);
    }

  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL)
    {
    parent->GetTransformFromWorld(transformFromWorld);
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
      vtkAbstractTransform* transformToParent=this->GetTransformToParent();
      if (transformToParent)
        {
        transformToNode->Concatenate(transformToParent);
        }
      if (strcmp(parent->GetID(), node->GetID()) )
        {
        this->GetTransformToNode(node, transformToNode);
        }
      }
    else if (this->GetTransformToParent())
      {
      vtkAbstractTransform* transformToParent=this->GetTransformToParent();
      if (transformToParent)
        {
        transformToNode->Concatenate(transformToParent);
        }
      }
    }
  else if (this->IsTransformNodeMyChild(node))
    {
    vtkMRMLTransformNode *parent = node->GetParentTransformNode();
    if (parent != NULL)
      {
      vtkAbstractTransform* transformToParent=node->GetTransformToParent();
      if (transformToParent)
        {
        transformToNode->Concatenate(transformToParent);
        }
      if (strcmp(parent->GetID(), this->GetID()) )
        {
        node->GetTransformToNode(this, transformToNode);
        }
      }
    else
      {
      vtkAbstractTransform* transformToParent=node->GetTransformToParent();
      if (transformToParent)
        {
        transformToNode->Concatenate(transformToParent);
        }
      }
    }
  else
    {
    this->GetTransformToWorld(transformToNode);

    vtkNew<vtkGeneralTransform> transformToWorld2;
    node->GetTransformToWorld(transformToWorld2.GetPointer());
    transformToWorld2->Inverse();

    transformToNode->Concatenate(transformToWorld2.GetPointer());
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
  if (transform==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::ApplyTransform failed: input transform is invalid");
    return;
    }

  // Get the applied transform components
  vtkSmartPointer<vtkAbstractTransform> transformCopy=vtkSmartPointer<vtkAbstractTransform>::Take(transform->MakeTransform());
  DeepCopyTransform(transformCopy, transform);
  // Flatten the transform list that will be applied to make the resulting general transform simpler
  // (have a simple list instead of a complex hierarchy)
  vtkNew<vtkCollection> transformCopyList;
  FlattenGeneralTransform(transformCopyList.GetPointer(), transformCopy);

  vtkAbstractTransform* oldTransformToParent = GetTransformToParent();
  if (oldTransformToParent==NULL && transformCopyList->GetNumberOfItems()==1)
    {
    // The transform was empty before and a non-composite transform is applied,
    // therefore there is no need to create a composite transform, just set the applied transform.
    vtkAbstractTransform* appliedTransform = vtkAbstractTransform::SafeDownCast(transformCopyList->GetItemAsObject(0));
    SetAndObserveTransformToParent(appliedTransform);
    return;
    }

  // We need the current transform to be a vtkGeneralTransform, which can store all the transform components.
  // (if the current transform is already a general transform tben we can just use that, otherwise we convert)
  // We arbitrarily pick the ToParent transform to store the new composited transform.
  vtkSmartPointer<vtkGeneralTransform> transformToParentGeneral = vtkGeneralTransform::SafeDownCast(oldTransformToParent);
  if (transformToParentGeneral.GetPointer()==NULL)
    {
    transformToParentGeneral = vtkSmartPointer<vtkGeneralTransform>::New();
    if (oldTransformToParent!=NULL)
      {
      transformToParentGeneral->Concatenate(oldTransformToParent);
      }
    }

  // Add components
  transformToParentGeneral->PostMultiply();
  for (int transformComponentIndex = transformCopyList->GetNumberOfItems()-1; transformComponentIndex>=0; transformComponentIndex--)
    {
    vtkAbstractTransform* transformComponent = vtkAbstractTransform::SafeDownCast(transformCopyList->GetItemAsObject(transformComponentIndex));
    transformToParentGeneral->Concatenate(transformComponent);
    }

  // Save the new transform
  SetAndObserveTransformToParent(transformToParentGeneral);
}

//-----------------------------------------------------------------------------
int vtkMRMLTransformNode::Split()
{
  if (!IsComposite())
    {
    // not composite, cannot split
    return 0;
    }
  vtkNew<vtkCollection> transformComponentList;
  vtkAbstractTransform* transformToParent = this->GetTransformToParent();
  if (transformToParent==NULL)
    {
    // no transform available, cannot split
    return 0;
    }
  FlattenGeneralTransform(transformComponentList.GetPointer(), transformToParent);
  int numberOfTransformComponents = transformComponentList->GetNumberOfItems();
  if (numberOfTransformComponents<1)
    {
    // no items, nothing to split
    return 0;
    }
  // If number of items is 1 we still continue, in this case we simplify the transform
  // (as one transform can be in a general transform hierarchy)
  vtkMRMLTransformNode* parentTransformNode = this->GetParentTransformNode();
  for (int transformComponentIndex = numberOfTransformComponents-1; transformComponentIndex>=0; transformComponentIndex--)
    {
    vtkAbstractTransform* transformComponent = vtkAbstractTransform::SafeDownCast(transformComponentList->GetItemAsObject(transformComponentIndex));
    vtkSmartPointer<vtkMRMLTransformNode> transformComponentNode;
    // Create a new transform node if for all transforms (parent transforms) but the last (the transform that is being split)
    if (transformComponentIndex>0)
      {
      // Create a new transform node with the most suitable type.
      // The generic vtkMRMLTransformNode could handle everything but at a couple of places
      // specific transform node classes are still used. When vtkMRMLLinearTransformNode, vtkMRMLBSplineTransformNode, and
      // vtkMRMLGridTransformNode classes will be removed then we can simply create a vtkMRMLTransformNode regardless the VTK transform type.
      if (transformComponent->IsA("vtkLinearTransform"))
        {
        transformComponentNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
        }
      else if (transformComponent->IsA("vtkBSplineTransform"))
        {
        transformComponentNode = vtkSmartPointer<vtkMRMLBSplineTransformNode>::New();
        }
      else if (transformComponent->IsA("vtkGridTransform"))
        {
        transformComponentNode = vtkSmartPointer<vtkMRMLGridTransformNode>::New();
        }
      else
        {
        // vtkThinPlateSplineTransform and general transform
        transformComponentNode = vtkSmartPointer<vtkMRMLTransformNode>::New();
        }
      std::string baseName = std::string(this->GetName())+"_Component";
      std::string uniqueName = this->GetScene()->GenerateUniqueName(baseName.c_str());
      transformComponentNode->SetName(uniqueName.c_str());
      this->GetScene()->AddNode(transformComponentNode.GetPointer());
      }
    else
      {
      transformComponentNode = this;
      }

    // Set as to/from parent so that the transform will not be inverted
    // It is important to set the non-inverted transform because when the
    // transform is edited then we have to update the forward transform
    // (inverse transform is computed, therefore changing an inverse transform would not affect the forward transform
    // which would lead to inconsistency between the forward and inverse transform;
    // also, any update of the forward transform overwrites the computed inverse transform)
    bool transformComputedFromInverse = vtkMRMLTransformNode::IsAbstractTransformComputedFromInverse(transformComponent);
    if (transformComputedFromInverse)
      {
      transformComponentNode->SetAndObserveTransformFromParent(transformComponent->GetInverse());
      }
    else
      {
      transformComponentNode->SetAndObserveTransformToParent(transformComponent);
      }
    transformComponentNode->SetAndObserveTransformNodeID(parentTransformNode ? parentTransformNode->GetID() : NULL);
    parentTransformNode=transformComponentNode.GetPointer();
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkMRMLTransformNode::IsAbstractTransformComputedFromInverse(vtkAbstractTransform* abstractTransform)
{
  if (abstractTransform==NULL)
    {
    return false;
    }

  vtkTransform* linearTransformComponent = vtkTransform::SafeDownCast(abstractTransform);
  if (linearTransformComponent)
    {
    linearTransformComponent->Update(); // Update is needed because it refreshes the inverse flag (the flag may be out-of-date if the transform depends on its inverse)
    return linearTransformComponent->GetInverseFlag();
    }

  vtkWarpTransform* warpTransformComponent = vtkWarpTransform::SafeDownCast(abstractTransform);
  if (warpTransformComponent)
    {
    warpTransformComponent->Update(); // Update is needed because it refreshes the inverse flag (the flag may be out-of-date if the transform depends on its inverse)
    return warpTransformComponent->GetInverseFlag();
    }

  vtkGeneralTransform* generalTransformComponent = vtkGeneralTransform::SafeDownCast(abstractTransform);
  if (generalTransformComponent)
    {
    generalTransformComponent->Update(); // Update is needed because it refreshes the inverse flag (the flag may be out-of-date if the transform depends on its inverse)
    return generalTransformComponent->GetInverseFlag();
    }

  vtkGenericWarningMacro("vtkMRMLTransformNode::IsTransformComputedFromInverse cannot determine if input transform " << abstractTransform->GetClassName() << " is inverted or not. Assuming not inverted.");
  return false;
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLTransformNode::CreateDefaultStorageNode()
{
  return vtkMRMLTransformStorageNode::New();
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::CreateDefaultDisplayNodes()
{
  if (vtkMRMLTransformDisplayNode::SafeDownCast(this->GetDisplayNode())!=NULL)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkNew<vtkMRMLTransformDisplayNode> dispNode;
  this->GetScene()->AddNode(dispNode.GetPointer());
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformNode::GetModifiedSinceRead()
{
  if (this->Superclass::GetModifiedSinceRead())
    {
    return true;
    }
  if (this->TransformToParent && this->TransformToParent->GetMTime() > this->GetStoredTime())
    {
    return true;
    }
  if (this->TransformFromParent && this->TransformFromParent->GetMTime() > this->GetStoredTime())
    {
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
int vtkMRMLTransformNode::GetMatrixTransformToParent(vtkMatrix4x4* matrix)
{
  if (matrix==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::GetMatrixTransformToParent failed: matrix is invalid");
    return 0;
    }
  // No transform means identity transform, which is a linear transform
  if (this->TransformToParent==NULL && this->TransformFromParent==NULL)
    {
    matrix->Identity();
    return 1;
    }
  vtkLinearTransform* transform=vtkLinearTransform::SafeDownCast(GetTransformToParentAs("vtkLinearTransform", false));
  if (transform==NULL)
    {
    vtkWarningMacro("Failed to get transformation matrix because transform is not linear");
    matrix->Identity();
    return 0;
    }
  transform->GetMatrix(matrix);
  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLTransformNode::GetMatrixTransformFromParent(vtkMatrix4x4* matrix)
{
  vtkNew<vtkMatrix4x4> transformToParentMatrix;
  int result = GetMatrixTransformToParent(transformToParentMatrix.GetPointer());
  vtkMatrix4x4::Invert(transformToParentMatrix.GetPointer(), matrix);
  return result;
}

//----------------------------------------------------------------------------
int  vtkMRMLTransformNode::GetMatrixTransformToWorld(vtkMatrix4x4* transformToWorld)
{
  if (!this->IsTransformToWorldLinear())
    {
    vtkWarningMacro("Failed to retrieve matrix to world from transform, the requested transform is not linear");
    transformToWorld->Identity();
    return 0;
    }

  // vtkMatrix4x4::Multiply4x4 computes the result in a separate buffer, so it is safe to use the input as output as well
  vtkNew<vtkMatrix4x4> matrixTransformToParent;
  if (!this->GetMatrixTransformToParent(matrixTransformToParent.GetPointer()))
    {
    vtkErrorMacro("Failed to retrieve matrix from linear transform");
    transformToWorld->Identity();
    return 0;
    }
  vtkMatrix4x4::Multiply4x4(matrixTransformToParent.GetPointer(), transformToWorld, transformToWorld);

  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL)
    {
    if (parent->IsLinear())
      {
      return (parent->GetMatrixTransformToWorld(transformToWorld));
      }
    else
      {
      vtkErrorMacro("vtkMRMLTransformNode::GetMatrixTransformToWorld failed: expected parent linear transform");
      transformToWorld->Identity();
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int  vtkMRMLTransformNode::GetMatrixTransformToNode(vtkMRMLTransformNode* node, vtkMatrix4x4* transformToNode)
{
  if (node == NULL)
    {
    return this->GetMatrixTransformToWorld(transformToNode);
    }
  if (!this->IsTransformToNodeLinear(node))
    {
    vtkErrorMacro("vtkMRMLTransformNode::GetMatrixTransformToNode failed: expected linear transforms between nodes");
    transformToNode->Identity();
    return 0;
    }

  if (this->IsTransformNodeMyParent(node))
    {
    vtkMRMLTransformNode *parent = this->GetParentTransformNode();
    vtkNew<vtkMatrix4x4> toParentMatrix;
    this->GetMatrixTransformToParent(toParentMatrix.GetPointer());
    if (parent != NULL)
      {
      vtkMatrix4x4::Multiply4x4(toParentMatrix.GetPointer(), transformToNode, transformToNode);
      if (strcmp(parent->GetID(), node->GetID()) )
        {
        this->GetMatrixTransformToNode(node, transformToNode);
        }
      }
    else
      {
      vtkMatrix4x4::Multiply4x4(toParentMatrix.GetPointer(), transformToNode, transformToNode);
      }
    }
  else if (this->IsTransformNodeMyChild(node))
    {
    vtkNew<vtkMatrix4x4> toParentMatrix;
    node->GetMatrixTransformToParent(toParentMatrix.GetPointer());
    vtkMRMLTransformNode *parent = vtkMRMLTransformNode::SafeDownCast(node->GetParentTransformNode());
    if (parent != NULL)
      {
      vtkMatrix4x4::Multiply4x4(toParentMatrix.GetPointer(), transformToNode, transformToNode);
      if (strcmp(parent->GetID(), this->GetID()) )
        {
        this->GetMatrixTransformToNode(this, transformToNode);
        }
      }
    else
      {
      vtkMatrix4x4::Multiply4x4(toParentMatrix.GetPointer(), transformToNode, transformToNode);
      }
    }
  else
    {
    this->GetMatrixTransformToWorld(transformToNode);
    vtkNew<vtkMatrix4x4> transformToWorld2;

    node->GetMatrixTransformToWorld(transformToWorld2.GetPointer());
    transformToWorld2->Invert();

    vtkMatrix4x4::Multiply4x4(transformToWorld2.GetPointer(), transformToNode, transformToNode);
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkMRMLTransformNode::GetAbstractTransformAs(vtkAbstractTransform* inputTransform, const char* transformClassName, bool logErrorIfFails)
{
  if (transformClassName==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: transformClassName is invalid");
    return NULL;
    }
  if (inputTransform==NULL)
    {
    if (logErrorIfFails)
      {
      vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: inputTransform is invalid");
      }
    return NULL;
    }
  if (inputTransform->IsA(transformClassName))
    {
    return inputTransform;
    }

  // Flatten the transform list to make the copying simpler
  vtkGeneralTransform* generalTransform=vtkGeneralTransform::SafeDownCast(inputTransform);
  if (generalTransform==NULL)
    {
    if (logErrorIfFails)
      {
      vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: expected a "<<transformClassName<<" type class and found "<<inputTransform->GetClassName()<<" instead");
      }
    return NULL;
    }

  vtkNew<vtkCollection> transformList;
  FlattenGeneralTransform(transformList.GetPointer(), generalTransform);

  if (transformList->GetNumberOfItems()==1)
    {
    vtkAbstractTransform* transform=vtkAbstractTransform::SafeDownCast(transformList->GetItemAsObject(0));
    if (transform==NULL)
      {
      vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: the stored transform is invalid");
      return NULL;
      }
    if (!transform->IsA(transformClassName))
      {
      if (logErrorIfFails)
        {
        vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: expected a "<<transformClassName<<" type class and found "<<transform->GetClassName()<<" instead");
        }
      return NULL;
      }
    return transform;
    }
  else if (transformList->GetNumberOfItems()==0)
    {
    if (logErrorIfFails)
      {
      vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: the input transform does not contain any transforms");
      }
    return NULL;
    }
  else
    {
    if (logErrorIfFails)
      {
      std::stringstream ss;
      for (int i=0; i<transformList->GetNumberOfItems(); i++)
        {
        const char* className=transformList->GetItemAsObject(i)->GetClassName();
        ss << " " << (className?className:"undefined");
        }
      ss << std::ends;
      vtkErrorMacro("vtkMRMLTransformNode::GetAbstractTransformAs failed: "<<generalTransform->GetNumberOfConcatenatedTransforms()
        <<" transforms are saved in a single node:"<<ss.str());
      }
    return NULL;
    }
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkMRMLTransformNode::GetTransformToParentAs(const char* transformClassName, bool logErrorIfFails/* =true */)
{
  if (this->TransformToParent)
    {
    return GetAbstractTransformAs(this->TransformToParent, transformClassName, logErrorIfFails);
    }
  else if (this->TransformFromParent)
    {
    vtkAbstractTransform *transform = GetAbstractTransformAs(this->TransformFromParent, transformClassName, logErrorIfFails);
    if (transform!=NULL)
      {
      return transform->GetInverse();
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkMRMLTransformNode::GetTransformFromParentAs(const char* transformClassName, bool logErrorIfFails/* =true */)
{
  if (this->TransformFromParent)
    {
    return GetAbstractTransformAs(this->TransformFromParent, transformClassName, logErrorIfFails);
    }
  else if (this->TransformToParent)
    {
    vtkAbstractTransform *transform = GetAbstractTransformAs(this->TransformToParent, transformClassName, logErrorIfFails);
    if (transform!=NULL)
      {
      return transform->GetInverse();
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::SetAndObserveTransform(vtkAbstractTransform** originalTransformPtr, vtkAbstractTransform** inverseTransformPtr, vtkAbstractTransform *transform)
{
  if ((*originalTransformPtr)==transform)
    {
    // no change
    return;
    }

  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int disabledModify = this->StartModify();

  vtkSetAndObserveMRMLObjectMacro((*originalTransformPtr), transform);

  // We set the inverse to NULL, which means that it's unknown and will be computed atuomatically from the original transform
  vtkSetAndObserveMRMLObjectMacro((*inverseTransformPtr), NULL);

  this->StorableModifiedTime.Modified();
  this->TransformModified();

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::SetAndObserveTransformToParent(vtkAbstractTransform *transform)
{
  SetAndObserveTransform(&(this->TransformToParent), &(this->TransformFromParent), transform);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::SetAndObserveTransformFromParent(vtkAbstractTransform *transform)
{
  SetAndObserveTransform(&(this->TransformFromParent), &(this->TransformToParent), transform);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformNode::ProcessMRMLEvents ( vtkObject *caller,
                                                    unsigned long event,
                                                    void *callData )
{
  Superclass::ProcessMRMLEvents ( caller, event, callData );

  if (event ==  vtkCommand::ModifiedEvent && caller!=NULL)
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
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::Inverse()
{
  if (this->TransformToParent==this->TransformFromParent)
    {
    // this should only happen if they are both NULL
    return;
    }
  vtkAbstractTransform* oldTransformToParent=this->TransformToParent;
  vtkAbstractTransform* oldTransformFromParent=this->TransformFromParent;
  this->TransformToParent=oldTransformFromParent;
  this->TransformFromParent=oldTransformToParent;

  this->StorableModifiedTime.Modified();
  this->Modified();
  this->TransformModified();
}

//----------------------------------------------------------------------------
unsigned long vtkMRMLTransformNode::GetTransformToWorldMTime()
{
  unsigned long latestMTime=0;
  vtkAbstractTransform* transformToParent=this->GetTransformToParent();
  if (transformToParent!=NULL)
    {
    latestMTime=transformToParent->GetMTime();
    }

  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL)
    {
    unsigned long parentMTime=parent->GetTransformToWorldMTime();
    if (parentMTime>latestMTime)
      {
      latestMTime=parentMTime;
      }
    }
  return latestMTime;
}

//----------------------------------------------------------------------------
const char* vtkMRMLTransformNode::GetTransformToParentInfo()
{
  if (this->TransformToParent==NULL)
    {
    if (this->TransformFromParent==NULL)
      {
      this->TransformInfo="Not specified";
      }
    else
      {
      this->TransformInfo="Computed by inverting transform from parent.";
      }
    return this->TransformInfo.c_str();
    }
  return GetTransformInfo(this->TransformToParent);
}

//----------------------------------------------------------------------------
const char* vtkMRMLTransformNode::GetTransformFromParentInfo()
{
  if (this->TransformFromParent==NULL)
    {
    if (this->TransformToParent==NULL)
      {
      this->TransformInfo="Not specified";
      }
    else
      {
      this->TransformInfo="Computed by inverting transform to parent.";
      }
    return this->TransformInfo.c_str();
    }
  return this->GetTransformInfo(this->TransformFromParent);
}


//----------------------------------------------------------------------------
const char* vtkMRMLTransformNode::GetTransformInfo(vtkAbstractTransform* inputTransform)
{
  this->TransformInfo="Not specified";
  if (inputTransform==NULL)
    {
    // invalid
    return this->TransformInfo.c_str();
    }
  vtkNew<vtkCollection> transformList;
  this->FlattenGeneralTransform(transformList.GetPointer(), inputTransform);

  if (transformList->GetNumberOfItems()==0)
    {
    // empty generic transform
    return this->TransformInfo.c_str();
    }

  std::stringstream ss;
  for (int i=0; i<transformList->GetNumberOfItems(); i++)
    {
    if (transformList->GetNumberOfItems()>1)
      {
      if (i>0)
        {
        ss << std::endl;
        }
      ss << "Transform "<<i+1<<":";
      }
    vtkAbstractTransform* transform=vtkAbstractTransform::SafeDownCast(transformList->GetItemAsObject(i));
    if (transform)
      {
      transform->Update();
      }

    vtkHomogeneousTransform* linearTransform=vtkHomogeneousTransform::SafeDownCast(transform);
    vtkBSplineTransform* bsplineTransform=vtkBSplineTransform::SafeDownCast(transform);
    vtkGridTransform* gridTransform=vtkGridTransform::SafeDownCast(transform);
    vtkThinPlateSplineTransform* tpsTransform=vtkThinPlateSplineTransform::SafeDownCast(transform);
    if (linearTransform!=NULL)
      {
      ss << " Linear";
      vtkMatrix4x4* m=linearTransform->GetMatrix();
      ss << std::endl <<"    "<<m->GetElement(0,0)<<"  "<<m->GetElement(0,1)<<"  "<<m->GetElement(0,2)<<"  "<<m->GetElement(0,3);
      ss << std::endl <<"    "<<m->GetElement(1,0)<<"  "<<m->GetElement(1,1)<<"  "<<m->GetElement(1,2)<<"  "<<m->GetElement(1,3);
      ss << std::endl <<"    "<<m->GetElement(2,0)<<"  "<<m->GetElement(2,1)<<"  "<<m->GetElement(2,2)<<"  "<<m->GetElement(2,3);
      ss << std::endl <<"    "<<m->GetElement(3,0)<<"  "<<m->GetElement(3,1)<<"  "<<m->GetElement(3,2)<<"  "<<m->GetElement(3,3);
      }
    else if (bsplineTransform!=NULL)
      {
      ss << " B-spline:";
      bsplineTransform->Update(); // compute if inverse
#if (VTK_MAJOR_VERSION <= 5)
      vtkImageData* coefficients=bsplineTransform->GetCoefficients();
#else
      vtkImageData* coefficients=bsplineTransform->GetCoefficientData();
#endif
      if (coefficients!=NULL)
        {
        int* extent = coefficients->GetExtent();
        int gridSize[3]={extent[1]-extent[0]+1, extent[3]-extent[2]+1, extent[5]-extent[4]+1};
        ss << std::endl << "  Grid size: " << gridSize[0] << " " << gridSize[1] << " " <<gridSize[2];
        double* gridOrigin = coefficients->GetOrigin();
        ss << std::endl << "  Grid origin: " << gridOrigin[0] << " " << gridOrigin[1] << " " <<gridOrigin[2];
        double* gridSpacing = coefficients->GetSpacing();
        ss << std::endl << "  Grid spacing: " << gridSpacing[0] << " " << gridSpacing[1] << " " <<gridSpacing[2];
        vtkOrientedBSplineTransform* orientedBsplineTransform=vtkOrientedBSplineTransform::SafeDownCast(transform);
        if (orientedBsplineTransform!=NULL)
          {
          vtkMatrix4x4* gridOrientation = orientedBsplineTransform->GetGridDirectionMatrix();
          if (gridOrientation!=NULL)
            {
            ss << std::endl << "  Grid orientation:";
            for (int i=0; i<3; i++)
              {
              ss << std::endl <<"    "<<gridOrientation->GetElement(i,0)<<"  "<<gridOrientation->GetElement(i,1)<<"  "<<gridOrientation->GetElement(i,2);
              }
            }
          vtkMatrix4x4* bulkTransform = orientedBsplineTransform->GetBulkTransformMatrix();
          if (bulkTransform!=NULL)
            {
            ss << std::endl << "  Additive bulk transform:";
            for (int i=0; i<4; i++)
              {
              ss << std::endl <<"    "<<bulkTransform->GetElement(i,0)
                <<"  "<<bulkTransform->GetElement(i,1)<<"  "<<bulkTransform->GetElement(i,2)<<"  "<<bulkTransform->GetElement(i,3);
              }
            }
          }
        }
      if (bsplineTransform->GetInverseFlag())
        {
        ss << std::endl << "  Computed from its inverse.";
        }
      }
    else if (gridTransform!=NULL)
      {
      ss << " Displacement field:";
      gridTransform->Update(); // compute if inverse
      vtkImageData* displacementField=gridTransform->GetDisplacementGrid();
      if (displacementField!=NULL)
        {
        int* extent=displacementField->GetExtent();
        ss << std::endl << "  Grid size: " << (extent[1]-extent[0]+1) << " " << (extent[3]-extent[2]+1) << " " << (extent[5]-extent[4]+1);
        double* origin=displacementField->GetOrigin();
        ss << std::endl << "  Grid origin: " << origin[0] << " " << origin[1] << " " << origin[2];
        double* spacing=displacementField->GetSpacing();
        ss << std::endl << "  Grid spacing: " << spacing[0] << " " << spacing[1] << " " << spacing[2];
        vtkOrientedGridTransform* orientedGridTransform=vtkOrientedGridTransform::SafeDownCast(transform);
        if (orientedGridTransform!=NULL)
          {
          vtkMatrix4x4* gridOrientation = orientedGridTransform->GetGridDirectionMatrix();
          if (gridOrientation!=NULL)
            {
            ss << std::endl << "  Grid orientation:";
            for (int i=0; i<3; i++)
              {
              ss << std::endl <<"    "<<gridOrientation->GetElement(i,0)<<"  "<<gridOrientation->GetElement(i,1)<<"  "<<gridOrientation->GetElement(i,2);
              }
            }
          }
        }
      else
        {
        ss << std::endl << "  Displacement field is invalid.";
        }
      if (gridTransform->GetInverseFlag())
        {
        ss << std::endl << "  Computed from its inverse.";
        }
      }
    else if (tpsTransform!=NULL)
      {
      ss << " Thin plate spline:";
      tpsTransform->Update(); // compute if inverse
      int numberOfSourceLandmarks = 0;
      if (tpsTransform->GetSourceLandmarks()!=NULL)
        {
        numberOfSourceLandmarks = tpsTransform->GetSourceLandmarks()->GetNumberOfPoints();
        }
      int numberOfTargetLandmarks = 0;
      if (tpsTransform->GetTargetLandmarks()!=NULL)
        {
        numberOfTargetLandmarks = tpsTransform->GetTargetLandmarks()->GetNumberOfPoints();
        }
      ss << std::endl << "  Number of source landmarks: " << numberOfSourceLandmarks;
      ss << std::endl << "  Number of target landmarks: " << numberOfTargetLandmarks;
      if (tpsTransform->GetInverseFlag())
        {
        ss << std::endl << "  Computed from its inverse.";
        }
      }
    else
      {
      const char* className=transform->GetClassName();
      ss << " " << (className?className:"invalid");
      }
    }

  ss << std::ends;
  this->TransformInfo=ss.str();
  return this->TransformInfo.c_str();
}


//----------------------------------------------------------------------------
int vtkMRMLTransformNode::IsLinear()
{
  // Most often linear transform is a single vtkTransform stored in this->TransformToParent.
  // Do a simple check for this specific case first to make this method as fast as possible.
  if (this->TransformToParent!=NULL && this->TransformToParent->IsA("vtkLinearTransform"))
    {
    return 1;
    }
  if (this->TransformFromParent!=NULL && this->TransformFromParent->IsA("vtkLinearTransform"))
    {
    return 1;
    }
  // No transform means identity transform, which is a linear transform
  if (this->TransformToParent==NULL && this->TransformFromParent==NULL)
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkMRMLTransformNode::IsComposite()
{
  if (this->TransformToParent!=NULL && this->TransformToParent->IsA("vtkGeneralTransform"))
    {
    return 1;
    }
  if (this->TransformFromParent!=NULL && this->TransformFromParent->IsA("vtkGeneralTransform"))
    {
    return 1;
    }
  return 0;
}

// Deprecated method - kept temporarily for compatibility with extensions that are not yet updated
//----------------------------------------------------------------------------
vtkMatrix4x4* vtkMRMLTransformNode::GetMatrixTransformToParent()
{
  vtkWarningMacro("vtkMRMLTransformNode::GetMatrixTransformToParent() method is deprecated. Use vtkMRMLTransformNode::GetMatrixTransformToParent(vtkMatrix4x4*) instead");
  GetMatrixTransformToParent(this->CachedMatrixTransformToParent);
  return this->CachedMatrixTransformToParent;
}

// Deprecated method - kept temporarily for compatibility with extensions that are not yet updated
//----------------------------------------------------------------------------
vtkMatrix4x4* vtkMRMLTransformNode::GetMatrixTransformFromParent()
{
  vtkWarningMacro("vtkMRMLTransformNode::GetMatrixTransformFromParent() method is deprecated. Use vtkMRMLTransformNode::GetMatrixTransformFromParent(vtkMatrix4x4*) instead");
  GetMatrixTransformFromParent(this->CachedMatrixTransformFromParent);
  return this->CachedMatrixTransformFromParent;
}

//----------------------------------------------------------------------------
int vtkMRMLTransformNode::SetMatrixTransformToParent(vtkMatrix4x4 *matrix)
{
  if (!this->IsLinear())
    {
    vtkWarningMacro("Cannot set matrix because vtkMRMLTransformNode contains a composite or non-linear transform. To overwrite the transform, first reset it by calling SetAndObserveTransformToParent(NULL).");
    return 0;
    }

  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldModify=this->StartModify();

  vtkTransform* currentTransform = NULL;
  if (this->TransformToParent!=NULL)
    {
    currentTransform = vtkTransform::SafeDownCast(GetTransformToParentAs("vtkTransform"));
    }

  // If current transform is an inverse transform then don't reuse it
  // (inverse transform is computed, therefore changing an inverse transform would not affect the forward transform
  // which would lead to inconsistency between the forward and inverse transform;
  // also, any update of the forward transform overwrites the computed inverse transform)
  if (currentTransform)
    {
    currentTransform->Update();
    if (currentTransform->GetInverseFlag())
      {
      currentTransform = NULL;
      }
    }

  if (currentTransform)
    {
    // Set matrix
    if (matrix)
      {
      currentTransform->SetMatrix(matrix);
      }
    else
      {
      currentTransform->Identity();
      }
    }
  else
    {
    // Transform does not exist or not the right type, replace it with a new one
    vtkNew<vtkTransform> transform;
    if (matrix)
      {
      transform->SetMatrix(matrix);
      }
    this->SetAndObserveTransformToParent(transform.GetPointer());
    }

  this->TransformToParent->Modified();
  this->EndModify(oldModify);
  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLTransformNode::SetMatrixTransformFromParent(vtkMatrix4x4 *matrix)
{
  vtkNew<vtkMatrix4x4> inverseMatrix;
  vtkMatrix4x4::Invert(matrix, inverseMatrix.GetPointer());
  return SetMatrixTransformToParent(inverseMatrix.GetPointer());
}

//----------------------------------------------------------------------------
void vtkMRMLTransformNode::ApplyTransformMatrix(vtkMatrix4x4* transformMatrix)
{
  if (transformMatrix==NULL)
    {
    vtkErrorMacro("vtkMRMLTransformNode::ApplyTransformMatrix failed: input transform is invalid");
    return;
    }
  if (!this->IsLinear())
    {
    // This object stores a non-linear transform, so we cannot merge the matrix with the existing transform, so
    // concatenate the linear transform (defined by transformMatrix) instead.
    vtkNew<vtkTransform> transform;
    transform->SetMatrix(transformMatrix);
    ApplyTransform(transform.GetPointer());
    return;
    }
  vtkNew<vtkMatrix4x4> matrixToParent;
  this->GetMatrixTransformToParent(matrixToParent.GetPointer());
  // vtkMatrix4x4::Multiply4x4 computes the output in an internal buffer and then
  // copies the result to the output matrix, therefore it is safe to use
  // one of the input matrices as output
  vtkMatrix4x4::Multiply4x4(transformMatrix, matrixToParent.GetPointer(), matrixToParent.GetPointer());
  SetMatrixTransformToParent(matrixToParent.GetPointer());
}

// Deprecated method - kept temporarily for compatibility with extensions that are not yet updated
//----------------------------------------------------------------------------
int vtkMRMLTransformNode::SetAndObserveMatrixTransformToParent(vtkMatrix4x4 *matrix)
{
  vtkWarningMacro("vtkMRMLTransformNode::SetAndObserveMatrixTransformToParent method is deprecated. Use vtkMRMLTransformNode::SetMatrixTransformToParent instead");
  return SetMatrixTransformToParent(matrix);
}

// Deprecated method - kept temporarily for compatibility with extensions that are not yet updated
//----------------------------------------------------------------------------
int vtkMRMLTransformNode::SetAndObserveMatrixTransformFromParent(vtkMatrix4x4 *matrix)
{
  vtkWarningMacro("vtkMRMLTransformNode::SetAndObserveMatrixTransformFromParent method is deprecated. Use vtkMRMLTransformNode::SetMatrixTransformFromParent instead");
  return SetMatrixTransformFromParent(matrix);
}

//----------------------------------------------------------------------------
bool vtkMRMLTransformNode::IsGeneralTransformLinear(vtkAbstractTransform* inputTransform, vtkTransform* concatenatedLinearTransform/*=NULL*/)
{
  if (inputTransform==NULL)
    {
    return true;
    }

  if (concatenatedLinearTransform)
    {
    concatenatedLinearTransform->Identity();
    concatenatedLinearTransform->PostMultiply();
    }

  vtkHomogeneousTransform* inputHomogeneousTransform=vtkHomogeneousTransform::SafeDownCast(inputTransform);
  if (inputHomogeneousTransform)
    {
    if (concatenatedLinearTransform)
      {
      concatenatedLinearTransform->Concatenate(inputHomogeneousTransform->GetMatrix());
      }
    return true;
    }

  // Push the transforms onto the stack in reverse order (use a stack to avoid recursive method call)
  std::stack< vtkAbstractTransform* > tstack;
  tstack.push(inputTransform);

  // Put all the transforms on the stack
  while (!tstack.empty())
    {
    vtkAbstractTransform *transform = tstack.top();
    tstack.pop();
    if (transform->IsA("vtkGeneralTransform"))
      {
      // Decompose general transforms
      vtkGeneralTransform *gtrans = (vtkGeneralTransform *)transform;
      gtrans->Update();
      int n = gtrans->GetNumberOfConcatenatedTransforms();
      while (n > 0)
        {
        tstack.push(gtrans->GetConcatenatedTransform(--n));
        }
      }
    else
      {
      // Simple transform, just concatenate (if non-linear then return with false)
      vtkHomogeneousTransform* homogeneousTransform=vtkHomogeneousTransform::SafeDownCast(transform);
      if (homogeneousTransform)
        {
        if (concatenatedLinearTransform)
          {
          concatenatedLinearTransform->Concatenate(homogeneousTransform->GetMatrix());
          }
        }
      else
        {
        return false;
        }
      }
    }
  return true;
}
