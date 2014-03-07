/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLNonlinearTransformNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/


#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"

#include "vtkGeneralTransform.h"
#include "vtkGridTransform.h"
#include "vtkWarpTransform.h"


#include "vtkMRMLNonlinearTransformNode.h"

vtkCxxSetObjectMacro(vtkMRMLNonlinearTransformNode,WarpTransformToParent,vtkWarpTransform);
vtkCxxSetObjectMacro(vtkMRMLNonlinearTransformNode,WarpTransformFromParent,vtkWarpTransform);


//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLNonlinearTransformNode);

//----------------------------------------------------------------------------
vtkMRMLNonlinearTransformNode::vtkMRMLNonlinearTransformNode()
{
  this->WarpTransformToParent = NULL;
  this->WarpTransformFromParent = NULL;
  this->ReadWriteAsTransformToParent = 0;
}

//----------------------------------------------------------------------------
vtkMRMLNonlinearTransformNode::~vtkMRMLNonlinearTransformNode()
{
  if (this->WarpTransformToParent)
    {
    this->SetAndObserveWarpTransformToParent(NULL);
    }
  if (this->WarpTransformFromParent)
    {
    this->SetAndObserveWarpTransformFromParent(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLNonlinearTransformNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // The different derived nonlinear transformation classes are going
  // to be so different that it doesn't make sense to write anything
  // here.  Let the derived classes do the work.
}

//----------------------------------------------------------------------------
void vtkMRMLNonlinearTransformNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);

  // The different derived nonlinear transformation classes are going
  // to be so different that it doesn't make sense to read anything
  // here.  Let the derived classes do the work.
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLNonlinearTransformNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);

  vtkMRMLNonlinearTransformNode *node = vtkMRMLNonlinearTransformNode::SafeDownCast(anode);
  if (node==NULL)
    {
    vtkErrorMacro("vtkMRMLNonlinearTransformNode::Copy: input node type is incompatible");
    return;
    }

  if (node->WarpTransformToParent)
    {
    vtkWarpTransform* clone=vtkWarpTransform::SafeDownCast(node->WarpTransformToParent->New());
    if (clone)
      {
      clone->DeepCopy(node->WarpTransformToParent);
      this->SetAndObserveWarpTransformToParent(clone);
      clone->Delete();
      }
    }
  else if (node->WarpTransformFromParent)
    {
    vtkWarpTransform* clone=vtkWarpTransform::SafeDownCast(node->WarpTransformFromParent->New());
    if (clone)
      {
      clone->DeepCopy(node->WarpTransformFromParent);
      this->SetAndObserveWarpTransformFromParent(clone);
      clone->Delete();
      }
    }
  else
    {
    this->SetAndObserveWarpTransformToParent(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLNonlinearTransformNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  if (this->WarpTransformToParent != NULL)
    {
    os << indent << "WarpTransformToParent: " << "\n";
    this->WarpTransformToParent->PrintSelf( os, indent.GetNextIndent() );
    }

  if (this->WarpTransformFromParent != NULL)
    {
    os << indent << "WarpTransformFromParent: " << "\n";
    this->WarpTransformFromParent->PrintSelf( os, indent.GetNextIndent() );
    }
}

//----------------------------------------------------------------------------
vtkGeneralTransform* vtkMRMLNonlinearTransformNode::GetTransformToParent()
{
  if (this->NeedToComputeTransformToParentFromInverse())
  {
    GetWarpTransformToParent(); // this updates this->TransformToParent
  }
  return this->TransformToParent;
}

//----------------------------------------------------------------------------
vtkGeneralTransform* vtkMRMLNonlinearTransformNode::GetTransformFromParent()
{
  if (NeedToComputeTransformFromParentFromInverse())
  {
    GetWarpTransformFromParent(); // this updates this->TransformFromParent
  }
  return this->TransformFromParent;
}

//----------------------------------------------------------------------------
int  vtkMRMLNonlinearTransformNode::GetMatrixTransformToWorld(vtkMatrix4x4* transformToWorld)
{
  if (this->IsTransformToWorldLinear() != 1) 
    {
    transformToWorld->Identity();
    return 0;
    }

  // TODO: what does this return code mean?
  return 1;
}

//----------------------------------------------------------------------------
int  vtkMRMLNonlinearTransformNode::GetMatrixTransformToNode(vtkMRMLTransformNode* node,
                                                          vtkMatrix4x4* transformToNode)
{
  if (this->IsTransformToNodeLinear(node) != 1) 
    {
    transformToNode->Identity();
    return 0;
    }
  
  
  // TODO: what does this return code mean?
  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLNonlinearTransformNode::SetAndObserveWarpTransformToParent(vtkWarpTransform *warp)
{
  if (warp == this->WarpTransformToParent)
    {
    return;
    }

  // Set the specific transform
  int oldTransformModify=this->StartTransformModify();
  int oldModify=this->StartModify();

  vtkSetAndObserveMRMLObjectMacro(this->WarpTransformToParent, warp);
  if (warp==NULL)
    {
    // the transform is cleared, make sure the inverse is cleared, too to avoid
    // computation of the current transform from the inverse
    vtkSetAndObserveMRMLObjectMacro(this->WarpTransformFromParent, NULL);
    }

  // Update the generic transform
  if (warp!=NULL)
    {
    // Update TransformToParent
    this->TransformToParent->Identity();
    this->TransformToParent->Concatenate(this->WarpTransformToParent);
    this->TransformToParentComputedFromInverseMTime=0;
    }
  else
    {
    // Clear TransformFromParent and TransformToParent
    vtkSetMRMLObjectMacro(this->TransformToParent, NULL);
    this->TransformToParentComputedFromInverseMTime=0;
    vtkSetMRMLObjectMacro(this->TransformFromParent, NULL);
    this->TransformFromParentComputedFromInverseMTime=0;
    }

  this->StorableModifiedTime.Modified();
  this->TransformModified();

  this->EndModify(oldModify);
  this->EndTransformModify(oldTransformModify);
}

//----------------------------------------------------------------------------
void vtkMRMLNonlinearTransformNode::SetAndObserveWarpTransformFromParent(vtkWarpTransform *warp)
{
  if (warp == this->WarpTransformFromParent)
    {
    return;
    }

  int oldTransformModify=this->StartTransformModify();
  int oldModify=this->StartModify();

  // Set the specific transform
  vtkSetAndObserveMRMLObjectMacro(this->WarpTransformFromParent, warp);
  if (warp==NULL)
    {
    // the transform is cleared, make sure the inverse is cleared, too to avoid
    // computation of the current transform from the inverse
    vtkSetAndObserveMRMLObjectMacro(this->WarpTransformToParent, NULL);
    }

  // Update the generic transform
  if (warp!=NULL)
    {
    // Update TransformFromParent
    this->TransformFromParent->Identity();
    this->TransformFromParent->Concatenate(this->WarpTransformFromParent);
    this->TransformFromParentComputedFromInverseMTime=0;
    }
  else
    {
    // Clear TransformToParent and TransformFromParent
    vtkSetMRMLObjectMacro(this->TransformFromParent, NULL);
    this->TransformFromParentComputedFromInverseMTime=0;
    vtkSetMRMLObjectMacro(this->TransformToParent, NULL);
    this->TransformToParentComputedFromInverseMTime=0;
    }

  this->StorableModifiedTime.Modified();
  this->TransformModified();

  this->EndModify(oldModify);
  this->EndTransformModify(oldTransformModify);

}

//---------------------------------------------------------------------------
void vtkMRMLNonlinearTransformNode::ProcessMRMLEvents ( vtkObject *caller,
                                                    unsigned long event, 
                                                    void *callData )
{
  Superclass::ProcessMRMLEvents ( caller, event, callData );

  if (this->WarpTransformToParent != NULL && this->WarpTransformToParent == vtkWarpTransform::SafeDownCast(caller) &&
      event ==  vtkCommand::ModifiedEvent)
    {
    this->TransformModified();
    }
  else if (this->WarpTransformFromParent != NULL && this->WarpTransformFromParent == vtkWarpTransform::SafeDownCast(caller) &&
      event ==  vtkCommand::ModifiedEvent)
    {
    this->TransformModified();
    }
}

// End
