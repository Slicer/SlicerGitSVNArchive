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
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLLinearTransformNode);

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode::vtkMRMLLinearTransformNode()
{
  this->MatrixTransformToParent = NULL;

  vtkMatrix4x4 *matrix  = vtkMatrix4x4::New();
  matrix->Identity();
  this->SetAndObserveMatrixTransformToParent(matrix);
  matrix->Delete();
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode::~vtkMRMLLinearTransformNode()
{
  if (this->MatrixTransformToParent) 
    {
    this->SetAndObserveMatrixTransformToParent(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  if (this->MatrixTransformToParent != NULL) 
    {
    std::stringstream ss;
    for (int row=0; row<4; row++) 
      {
      for (int col=0; col<4; col++) 
        {
        ss << this->MatrixTransformToParent->GetElement(row, col);
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
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLLinearTransformNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLLinearTransformNode *node = (vtkMRMLLinearTransformNode *) anode;
  for (int i=0; i<4; i++) 
    {
    for (int j=0; j<4; j++)
      {
      this->GetMatrixTransformToParent()->SetElement(i,j,(node->MatrixTransformToParent->GetElement(i,j)));
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLLinearTransformNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  if (this->MatrixTransformToParent != NULL) 
    {
    os << indent << "MatrixTransformToParent: " << "\n";
    for (int row=0; row<4; row++) 
      {
      for (int col=0; col<4; col++) 
        {
        os << this->MatrixTransformToParent->GetElement(row, col);
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
  this->TransformToParent->Identity();
  this->TransformToParent->Concatenate(this->MatrixTransformToParent);
  return this->TransformToParent;
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
  vtkMatrix4x4::Multiply4x4(this->MatrixTransformToParent, xform, transformToWorld);
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
      vtkMatrix4x4::Multiply4x4(this->MatrixTransformToParent, xform, transformToNode);
      xform->Delete();

      if (strcmp(parent->GetID(), node->GetID()) ) 
        {
        this->GetMatrixTransformToNode(node, transformToNode);
        }
      }
    else if (this->MatrixTransformToParent) 
      {
      vtkMatrix4x4 *xform = vtkMatrix4x4::New();
      xform->DeepCopy(transformToNode);
      vtkMatrix4x4::Multiply4x4(this->MatrixTransformToParent, xform, transformToNode);
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
      vtkMatrix4x4::Multiply4x4(lnode->MatrixTransformToParent, xform, transformToNode);
      xform->Delete();

      if (strcmp(parent->GetID(), this->GetID()) ) 
        {
        this->GetMatrixTransformToNode(this, transformToNode);
        }
      }
    else if (lnode->MatrixTransformToParent) 
      {
      vtkMatrix4x4 *xform = vtkMatrix4x4::New();
      xform->DeepCopy(transformToNode);
      vtkMatrix4x4::Multiply4x4(lnode->MatrixTransformToParent, xform, transformToNode);
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
  vtkSetAndObserveMRMLObjectMacro(this->MatrixTransformToParent, matrix);
  this->StorableModifiedTime.Modified();
  this->Modified();
  this->InvokeEvent(vtkMRMLTransformableNode::TransformModifiedEvent, NULL);
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
    this->InvokeEvent(vtkMRMLTransformableNode::TransformModifiedEvent, NULL);
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
  vtkMatrix4x4* matrixToParent = this->GetMatrixTransformToParent();
  vtkMatrix4x4* newMatrixToParent = vtkMatrix4x4::New();

  vtkMatrix4x4::Multiply4x4(transformMatrix, matrixToParent, newMatrixToParent);

  this->SetAndObserveMatrixTransformToParent(newMatrixToParent);

  newMatrixToParent->Delete();
}

// End
