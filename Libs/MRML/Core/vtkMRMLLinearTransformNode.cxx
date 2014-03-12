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
#include <vtkMatrixToLinearTransform.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLLinearTransformNode);

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode::vtkMRMLLinearTransformNode()
{
  this->ReadWriteAsTransformToParent = 1;
  vtkNew<vtkMatrix4x4> matrix;
  matrix->Identity();
  this->SetMatrixTransformToParent(matrix.GetPointer());
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode::~vtkMRMLLinearTransformNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);


  vtkMatrix4x4* matrix=NULL;

  if (this->ReadWriteAsTransformToParent)
    {
    matrix=GetMatrixTransformToParent();
    }
  else
    {
    matrix=GetMatrixTransformFromParent();
    }

  if (matrix != NULL)
    {
    std::stringstream ss;
    for (int row=0; row<4; row++) 
      {
      for (int col=0; col<4; col++) 
        {
        ss << matrix->GetElement(row, col);
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
    if (this->ReadWriteAsTransformToParent)
      {
      of << indent << " matrixTransformToParent=\"" << ss.str() << "\"";
      }
    else
      {
      of << indent << " matrixTransformFromParent=\"" << ss.str() << "\"";
      }
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
      vtkNew<vtkMatrix4x4> matrix;
      matrix->Identity();
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
      this->SetMatrixTransformToParent(matrix.GetPointer());
      }
    if (!strcmp(attName, "matrixTransformFromParent"))
      {
      vtkNew<vtkMatrix4x4> matrix;
      matrix->Identity();
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
      this->SetMatrixTransformFromParent(matrix.GetPointer());
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
  Superclass::Copy(anode);
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
vtkMatrix4x4* vtkMRMLLinearTransformNode::GetMatrixTransformToParent()
{
  vtkMatrixToLinearTransform* transform=vtkMatrixToLinearTransform::SafeDownCast(GetTransformToParentAs("vtkMatrixToLinearTransform"));
  if (transform==NULL)
  {
    return NULL;
  }
  return transform->GetMatrix();
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkMRMLLinearTransformNode::GetMatrixTransformFromParent()
{
  vtkMatrixToLinearTransform* transform=vtkMatrixToLinearTransform::SafeDownCast(GetTransformFromParentAs("vtkMatrixToLinearTransform"));
  if (transform==NULL)
  {
    return NULL;
  }
  return transform->GetMatrix();
}

//----------------------------------------------------------------------------
int  vtkMRMLLinearTransformNode::GetMatrixTransformToWorld(vtkMatrix4x4* transformToWorld)
{
  if (this->IsTransformToWorldLinear() != 1)
    {
    vtkWarningMacro("Failed to retrieve matrix to world from transform, the requested transform is not linear");
    transformToWorld->Identity();
    return 0;
    }

  // vtkMatrix4x4::Multiply4x4 computes the result in a separate buffer, so it is safe to use the input as output as well
  vtkMatrix4x4* matrixTransformToParent=this->GetMatrixTransformToParent();
  if (matrixTransformToParent!=NULL)
    {
    vtkMatrix4x4::Multiply4x4(matrixTransformToParent, transformToWorld, transformToWorld);
    }
  else
    {
    vtkErrorMacro("Failed to retrieve matrix from linear transform");
    transformToWorld->Identity();
    return 0;
    }

  vtkMRMLTransformNode *parent = this->GetParentTransformNode();
  if (parent != NULL) 
    {
    vtkMRMLLinearTransformNode *lparent = vtkMRMLLinearTransformNode::SafeDownCast(parent);
    if (lparent)
      {
      return (lparent->GetMatrixTransformToWorld(transformToWorld));
      }
    else
      {
      vtkErrorMacro("vtkMRMLLinearTransformNode::GetMatrixTransformToWorld failed: expected parent linear transform");
      transformToWorld->Identity();
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int  vtkMRMLLinearTransformNode::GetMatrixTransformToNode(vtkMRMLTransformNode* node,
                                                          vtkMatrix4x4* transformToNode)
{
  if (node == NULL) 
    {
    return this->GetMatrixTransformToWorld(transformToNode);
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
void vtkMRMLLinearTransformNode::SetMatrixTransformToParent(vtkMatrix4x4 *matrix)
{
  vtkMatrixToLinearTransform* transform=vtkMatrixToLinearTransform::SafeDownCast(GetTransformToParentAs("vtkMatrixToLinearTransform"));
  vtkMatrix4x4* currentMatrix=NULL;
  if (transform!=NULL)
    {
    currentMatrix=transform->GetInput();
    }
  if (currentMatrix==matrix)
    {
    return;
    }
  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldTransformModify=this->StartTransformModify();
  int oldModify=this->StartModify();

  if (currentMatrix!=NULL)
    {
    if (matrix!=NULL)
      {
      currentMatrix->DeepCopy(matrix);
      }
    else
      {
      currentMatrix->Identity();
      }
    }
  else
    {
    vtkNew<vtkMatrixToLinearTransform> transform;
    vtkNew<vtkMatrix4x4> newMatrix;
    if (matrix!=NULL)
      {
      newMatrix->DeepCopy(matrix);
      }
    else
      {
      newMatrix->Identity();
      }
    transform->SetInput(newMatrix.GetPointer());
    this->SetAndObserveTransformToParent(transform.GetPointer());
    }
  this->TransformToParent->Modified();
  this->EndModify(oldModify);
  this->EndTransformModify(oldTransformModify);
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::SetMatrixTransformFromParent(vtkMatrix4x4 *matrix)
{
  vtkMatrixToLinearTransform* transform=vtkMatrixToLinearTransform::SafeDownCast(GetTransformFromParentAs("vtkMatrixToLinearTransform"));
  vtkMatrix4x4* currentMatrix=NULL;
  if (transform!=NULL)
    {
    currentMatrix=transform->GetInput();
    }
  if (currentMatrix==matrix)
    {
    return;
    }
  // Temporarily disable all Modified and TransformModified events to make sure that
  // the operations are performed without interruption.
  int oldTransformModify=this->StartTransformModify();
  int oldModify=this->StartModify();

  if (currentMatrix!=NULL)
    {
    if (matrix!=NULL)
      {
      currentMatrix->DeepCopy(matrix);
      }
    else
      {
      currentMatrix->Identity();
      }
    }
  else
    {
    vtkNew<vtkMatrixToLinearTransform> transform;
    vtkNew<vtkMatrix4x4> newMatrix;
    if (matrix!=NULL)
      {
      newMatrix->DeepCopy(matrix);
      }
    else
      {
      newMatrix->Identity();
      }
    transform->SetInput(newMatrix.GetPointer());
    this->SetAndObserveTransformFromParent(transform.GetPointer());
    }
  this->TransformFromParent->Modified();
  this->EndModify(oldModify);
  this->EndTransformModify(oldTransformModify);
}

//----------------------------------------------------------------------------
bool vtkMRMLLinearTransformNode::CanApplyNonLinearTransforms()const
{
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::ApplyTransformMatrix(vtkMatrix4x4* transformMatrix)
{
  if (transformMatrix==NULL)
    {
    vtkErrorMacro("vtkMRMLLinearTransformNode::ApplyTransformMatrix failed: input transform is invalid");
    return;
    }
  // vtkMatrix4x4::Multiply4x4 computes the output in an internal buffer and then
  // copies the result to the output matrix, therefore it is safe to use
  // one of the input matrices as output
  vtkMatrix4x4* matrixToParent=this->GetMatrixTransformToParent();
  if (matrixToParent!=NULL)
    {
    vtkMatrix4x4::Multiply4x4(transformMatrix, matrixToParent, matrixToParent);
    }
  else
    {
    SetMatrixTransformToParent(transformMatrix);
    }
}

// End
