/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLLinearTransformNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLLinearTransformNode.h"

// VTK includes
#include <vtkCommand.h>
#include <vtkGeneralTransform.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLLinearTransformNode);

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode::vtkMRMLLinearTransformNode()
{
  this->MatrixTransformToParent = NULL;
  this->MatrixTransformFromParent = NULL;
  this->ReadWriteAsTransformToParent = 1;
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode::~vtkMRMLLinearTransformNode()
{
  if (this->MatrixTransformToParent) 
    {
    this->SetAndObserveMatrixTransformToParent(NULL);
    }
  if (this->MatrixTransformFromParent)
    {
    this->SetAndObserveMatrixTransformFromParent(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  if (this->GetMatrixTransformToParent() != NULL)
    {
    std::stringstream ss;
    for (int row=0; row<4; row++) 
      {
      for (int col=0; col<4; col++) 
        {
        ss << this->GetMatrixTransformToParent()->GetElement(row, col);
        if (!(row==3 && col==3)) 
          {
          ss << " ";
          }
        }
      if ( row != 3 )
        {
        ss << " ";
        }
      }
    of << indent << " matrixTransformToParent=\"" << ss.str() << "\"";
    }

}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::ReadXMLAttributes(const char** atts)
{
  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldTransformModify=this->StartTransformModify();
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "matrixTransformToParent")) 
      {
      vtkMatrix4x4 *matrix  = vtkMatrix4x4::New();
      matrix->Identity();
      if (this->MatrixTransformToParent != NULL) 
        {
        this->SetAndObserveMatrixTransformToParent(NULL);
        }
      std::stringstream ss;
      double val;
      ss << attValue;
      for (int row=0; row<4; row++) 
        {
        for (int col=0; col<4; col++) 
          {
          ss >> val;
          matrix->SetElement(row, col, val);
          }
        }
      this->SetAndObserveMatrixTransformToParent(matrix);
      matrix->Delete();
      }
    }  
  this->EndModify(disabledModify);
  this->EndTransformModify(oldTransformModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLLinearTransformNode::Copy(vtkMRMLNode *anode)
{
  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldTransformModify=this->StartTransformModify();
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLLinearTransformNode *node = (vtkMRMLLinearTransformNode *) anode;

  if (node->MatrixTransformToParent)
    {
    vtkNew<vtkMatrix4x4> matrix;
    matrix->DeepCopy(node->MatrixTransformToParent);
    this->SetAndObserveMatrixTransformToParent(matrix.GetPointer());
    }
  else if (node->MatrixTransformFromParent)
    {
    vtkNew<vtkMatrix4x4> matrix;
    matrix->DeepCopy(node->MatrixTransformFromParent);
    this->SetAndObserveMatrixTransformFromParent(matrix.GetPointer());
    }
  else
    {
    this->SetAndObserveMatrixTransformToParent(NULL);
    }

  this->EndModify(disabledModify);
  this->EndTransformModify(oldTransformModify);
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  if (this->GetMatrixTransformToParent() != NULL)
    {
    os << indent << "MatrixTransformToParent: " << "\n";
    for (int row=0; row<4; row++) 
      {
      for (int col=0; col<4; col++) 
        {
        os << this->GetMatrixTransformToParent()->GetElement(row, col);
        if (!(row==3 && col==3))
          {
          os << " ";
          }
        else 
          {
          os << "\n";
          }
        } // for (int col
      } // for (int row
    }
}

//----------------------------------------------------------------------------
vtkGeneralTransform* vtkMRMLLinearTransformNode::GetTransformToParent()
{
  // When a transform is set, we do not compute the inverse transform immediately
  // because for non-linear transforms it is an expensive operation.
  // We do not compute it for linear transforms either to make the implementation
  // consistent for all kind of transforms (and also because theoretically we may want
  // to be able to manage projection transforms, which are not invertible).
  if (NeedToComputeTransformToParentFromInverse())
  {
    GetMatrixTransformToParent(); // this updates this->TransformToParent
  }
  return this->TransformToParent;
}

//----------------------------------------------------------------------------
vtkGeneralTransform* vtkMRMLLinearTransformNode::GetTransformFromParent()
{
  // When a transform is set, we do not compute the inverse transform immediately
  // because for non-linear transforms it is an expensive operation.
  // We do not compute it for linear transforms either to make the implementation
  // consistent for all kind of transforms (and also because theoretically we may want
  // to be able to manage projection transforms, which are not invertible)
  {
    GetMatrixTransformFromParent(); // this updates this->TransformToParent
  }
  return this->TransformFromParent;
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkMRMLLinearTransformNode::GetMatrixTransformToParent()
{
  bool computeFromInverse = this->MatrixTransformFromParent && NeedToComputeTransformToParentFromInverse();

  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldTransformModify=this->StartTransformModify();
  int oldModify=this->StartModify();

  // Update the specific transform
  if (this->MatrixTransformToParent == 0)
    {
    vtkNew<vtkMatrix4x4> matrix;
    this->SetAndObserveMatrixTransformToParent(matrix.GetPointer());
    }

  if (computeFromInverse)
    {
    vtkMatrix4x4::Invert(this->MatrixTransformFromParent, this->MatrixTransformToParent);
    }

  // Update the generic transform
  this->TransformToParent->Identity();
  this->TransformToParent->Concatenate(this->MatrixTransformToParent);
  if (computeFromInverse)
    {
    this->TransformToParentComputedFromInverseMTime=this->TransformFromParent->GetMTime();
    }

  this->EndModify(oldModify);
  this->EndTransformModify(oldTransformModify);

  return this->MatrixTransformToParent;
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkMRMLLinearTransformNode::GetMatrixTransformFromParent()
{
  bool computeFromInverse =  this->MatrixTransformToParent && NeedToComputeTransformFromParentFromInverse();

  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldTransformModify=this->StartTransformModify();
  int oldModify=this->StartModify();

  // Update the specific transform
  if (this->MatrixTransformFromParent == 0)
    {
    vtkNew<vtkMatrix4x4> matrix;
    this->SetAndObserveMatrixTransformFromParent(matrix.GetPointer());
    }

  if (computeFromInverse)
    {
    vtkMatrix4x4::Invert(this->MatrixTransformToParent, this->MatrixTransformFromParent);
    }

  // Update the generic transform
  this->TransformFromParent->Identity();
  this->TransformFromParent->Concatenate(this->MatrixTransformFromParent);
  if (computeFromInverse)
    {
    this->TransformFromParentComputedFromInverseMTime=this->TransformToParent->GetMTime();
    }

  this->EndModify(oldModify);
  this->EndTransformModify(oldTransformModify);

  return this->MatrixTransformFromParent;
}

//----------------------------------------------------------------------------
int  vtkMRMLLinearTransformNode::GetMatrixTransformToWorld(vtkMatrix4x4* transformToWorld)
{
  if (this->IsTransformToWorldLinear() != 1) 
    {
    transformToWorld->Identity();
    return 0;
    }

  vtkMatrix4x4 *xform = vtkMatrix4x4::New();
  xform->DeepCopy(transformToWorld);
  vtkMatrix4x4::Multiply4x4(this->GetMatrixTransformToParent(), xform, transformToWorld);
  xform->Delete();

  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL) 
    {
    vtkMRMLLinearTransformNode *lparent = dynamic_cast < vtkMRMLLinearTransformNode* > (parent);
    if (lparent) 
      {
      return (lparent->GetMatrixTransformToWorld(transformToWorld));
      }
    }
  // TODO: what does this return code mean?
  return 1;
}

//----------------------------------------------------------------------------
int  vtkMRMLLinearTransformNode::GetMatrixTransformToNode(vtkMRMLTransformNode* node,
                                                          vtkMatrix4x4* transformToNode)
{
  if (node == NULL) 
    {
    this->GetMatrixTransformToWorld(transformToNode);
    return 1;
    }
  if (this->IsTransformToNodeLinear(node) != 1) 
    {
    transformToNode->Identity();
    return 0;
    }
  
  
  if (this->IsTransformNodeMyParent(node)) 
    {
    vtkMRMLTransformNode *parent = this->GetParentTransformNode();
    if (parent != NULL) 
      {

      vtkMatrix4x4 *xform = vtkMatrix4x4::New();
      xform->DeepCopy(transformToNode);
      vtkMatrix4x4::Multiply4x4(this->GetMatrixTransformToParent(), xform, transformToNode);
      xform->Delete();

      if (strcmp(parent->GetID(), node->GetID()) ) 
        {
        this->GetMatrixTransformToNode(node, transformToNode);
        }
      }
    else if (this->GetMatrixTransformToParent())
      {
      vtkMatrix4x4 *xform = vtkMatrix4x4::New();
      xform->DeepCopy(transformToNode);
      vtkMatrix4x4::Multiply4x4(this->GetMatrixTransformToParent(), xform, transformToNode);
      xform->Delete();
      }
    }
  else if (this->IsTransformNodeMyChild(node)) 
    {
    vtkMRMLLinearTransformNode *lnode = dynamic_cast <vtkMRMLLinearTransformNode *> (node);
    vtkMRMLLinearTransformNode *parent = dynamic_cast <vtkMRMLLinearTransformNode *> (node->GetParentTransformNode());
    if (parent != NULL) 
      {

      vtkMatrix4x4 *xform = vtkMatrix4x4::New();
      xform->DeepCopy(transformToNode);
      vtkMatrix4x4::Multiply4x4(lnode->GetMatrixTransformToParent(), xform, transformToNode);
      xform->Delete();

      if (strcmp(parent->GetID(), this->GetID()) ) 
        {
        this->GetMatrixTransformToNode(this, transformToNode);
        }
      }
    else if (lnode->GetMatrixTransformToParent())
      {
      vtkMatrix4x4 *xform = vtkMatrix4x4::New();
      xform->DeepCopy(transformToNode);
      vtkMatrix4x4::Multiply4x4(lnode->GetMatrixTransformToParent(), xform, transformToNode);
      xform->Delete();
      }
    }
  else 
    {
    this->GetMatrixTransformToWorld(transformToNode);
    vtkMatrix4x4* transformToWorld2 = vtkMatrix4x4::New();
    transformToWorld2->Identity();
    
    node->GetMatrixTransformToWorld(transformToWorld2);
    transformToWorld2->Invert();
    
    vtkMatrix4x4 *xform = vtkMatrix4x4::New();
    xform->DeepCopy(transformToNode);
    vtkMatrix4x4::Multiply4x4(transformToWorld2, xform, transformToNode);
    xform->Delete();
    transformToWorld2->Delete();
    }
  // TODO: what does this return code mean?
  return 1;
}


//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::SetAndObserveMatrixTransformToParent(vtkMatrix4x4 *matrix)
{
  if (this->MatrixTransformToParent == matrix)
    {
    return;
    }

  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldTransformModify=this->StartTransformModify();
  int oldModify=this->StartModify();

  // Set the specific transform
  vtkSetAndObserveMRMLObjectMacro(this->MatrixTransformToParent, matrix);
  if (matrix==NULL)
    {
    // the matrix is cleared, make sure the inverse is cleared, too to avoid
    // computation of the current transform from the inverse
    vtkSetAndObserveMRMLObjectMacro(this->MatrixTransformFromParent, NULL);
    }

  // Update the generic transform
  if (matrix!=NULL)
    {
    // Update TransformToParent
    this->TransformToParent->Identity();
    this->TransformToParent->Concatenate(this->MatrixTransformToParent);
    this->TransformToParentComputedFromInverseMTime=0;
    }
  else
    {
    // Clear TransformFromParent and TransformToParent
    this->TransformToParent->Identity();
    this->TransformToParentComputedFromInverseMTime=0;
    this->TransformFromParent->Identity();
    this->TransformFromParentComputedFromInverseMTime=0;
    }

  this->StorableModifiedTime.Modified();
  this->TransformModified();

  this->EndModify(oldModify);
  this->EndTransformModify(oldTransformModify);
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::SetAndObserveMatrixTransformFromParent(vtkMatrix4x4 *matrix)
{
  if (this->MatrixTransformFromParent == matrix)
    {
    return;
    }

  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldTransformModify=this->StartTransformModify();
  int oldModify=this->StartModify();

  // Set the specific transform
  vtkSetAndObserveMRMLObjectMacro(this->MatrixTransformFromParent, matrix);
  if (matrix==NULL)
    {
    // the matrix is cleared, make sure the inverse is cleared, too to avoid
    // computation of the current transform from the inverse
    vtkSetAndObserveMRMLObjectMacro(this->MatrixTransformToParent, NULL);
    }

  // Update the generic transform
  if (matrix!=NULL)
    {
    // Update TransformFromParent
    this->TransformFromParent->Identity();
    this->TransformFromParent->Concatenate(this->MatrixTransformFromParent);
    this->TransformFromParentComputedFromInverseMTime=0;
    }
  else
    {
    // Clear TransformToParent and TransformFromParent
    this->TransformFromParent->Identity();
    this->TransformFromParentComputedFromInverseMTime=0;
    this->TransformToParent->Identity();
    this->TransformToParentComputedFromInverseMTime=0;
    }

  this->StorableModifiedTime.Modified();
  this->TransformModified();

  this->EndModify(oldModify);
  this->EndTransformModify(oldTransformModify);
}

//---------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::ProcessMRMLEvents ( vtkObject *caller,
                                                    unsigned long event, 
                                                    void *callData )
{
  Superclass::ProcessMRMLEvents ( caller, event, callData );

  if (this->MatrixTransformToParent != NULL &&
      this->MatrixTransformToParent == vtkMatrix4x4::SafeDownCast(caller) &&
      event ==  vtkCommand::ModifiedEvent)
    {
    this->StorableModifiedTime.Modified();
    this->TransformModified();
    }
  else if (this->MatrixTransformFromParent != NULL &&
      this->MatrixTransformFromParent == vtkMatrix4x4::SafeDownCast(caller) &&
      event ==  vtkCommand::ModifiedEvent)
    {
    this->StorableModifiedTime.Modified();
    this->TransformModified();
    }
}

//----------------------------------------------------------------------------
bool vtkMRMLLinearTransformNode::CanApplyNonLinearTransforms()const
{
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::ApplyTransformMatrix(vtkMatrix4x4* transformMatrix)
{
  // vtkMatrix4x4::Multiply4x4 computes the output in an internal buffer and then
  // copies the result to the output matrix, therefore it is safe to use
  // one of the input matrices as output
  vtkMatrix4x4* matrixToParent=this->GetMatrixTransformToParent();
  vtkMatrix4x4::Multiply4x4(transformMatrix, matrixToParent, matrixToParent);
}

// End
