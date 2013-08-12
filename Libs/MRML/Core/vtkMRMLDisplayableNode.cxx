/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLDisplayableNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLDisplayableNode.h"
#include "vtkMRMLDisplayNode.h"
#include "vtkMRMLScene.h"

// when change the display node, update the scalars
//#include "vtkMRMLVolumeNode.h"

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkMath.h>

// STD includes
#include <algorithm>
#include <cassert>
#include <sstream>

const char* vtkMRMLDisplayableNode::DisplayNodeReferenceRole = "display";
const char* vtkMRMLDisplayableNode::DisplayNodeReferenceMRMLAttributeName = "displayNodeRef";

//----------------------------------------------------------------------------
vtkMRMLDisplayableNode::vtkMRMLDisplayableNode()
{
  this->AddNodeReferenceRole(this->GetDisplayNodeReferenceRole(),
                             this->GetDisplayNodeReferenceMRMLAttributeName() );
}

//----------------------------------------------------------------------------
vtkMRMLDisplayableNode::~vtkMRMLDisplayableNode()
{
}

//----------------------------------------------------------------------------
const char* vtkMRMLDisplayableNode::GetDisplayNodeReferenceRole()
{
  return vtkMRMLDisplayableNode::DisplayNodeReferenceRole;
}

//----------------------------------------------------------------------------
const char* vtkMRMLDisplayableNode::GetDisplayNodeReferenceMRMLAttributeName()
{
  return vtkMRMLDisplayableNode::DisplayNodeReferenceMRMLAttributeName;
}

//----------------------------------------------------------------------------
void vtkMRMLDisplayableNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults

  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLDisplayableNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLDisplayableNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  this->Superclass::Copy(anode);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLDisplayableNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int numDisplayNodes = this->GetNumberOfNodeReferences(
    this->GetDisplayNodeReferenceRole());

  for (int i=0; i<numDisplayNodes; i++)
    {
    const char * id = this->GetNthNodeReferenceID(
      this->GetDisplayNodeReferenceRole(), i);
    os << indent << "DisplayNodeIDs[" << i << "]: " << id << "\n";
    }
}

//----------------------------------------------------------------------------
vtkMRMLDisplayNode* vtkMRMLDisplayableNode::GetNthDisplayNode(int n)
{
  return vtkMRMLDisplayNode::SafeDownCast(
    this->GetNthNodeReference(this->GetDisplayNodeReferenceRole(), n));
}

//---------------------------------------------------------------------------
void vtkMRMLDisplayableNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event,
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
  int numDisplayNodes = this->GetNumberOfNodeReferences(this->GetDisplayNodeReferenceRole());
  for (int i=0; i<numDisplayNodes; i++)
    {
    vtkMRMLDisplayNode *dnode = this->GetNthDisplayNode(i);
    if (dnode != NULL && dnode == vtkMRMLDisplayNode::SafeDownCast(caller) &&
      event ==  vtkCommand::ModifiedEvent)
      {
      this->InvokeEvent(vtkMRMLDisplayableNode::DisplayModifiedEvent, dnode);
      }
    }
  return;
}

//---------------------------------------------------------------------------
void vtkMRMLDisplayableNode::CreateDefaultDisplayNodes()
{
  // does nothing by default
}

//---------------------------------------------------------------------------
int vtkMRMLDisplayableNode::GetDisplayVisibility()
{
  int ndnodes = this->GetNumberOfDisplayNodes();
  if (ndnodes == 0 || this->GetNthDisplayNode(0) == 0)
    {
    return 0;
    }
  int visible = this->GetNthDisplayNode(0)->GetVisibility();
  if (visible == 2)
    {
    return 2;
    }

  for (int i=1; i<ndnodes; i++)
    {
    vtkMRMLDisplayNode *displayNode = this->GetNthDisplayNode(i);
    if ( displayNode && displayNode->GetVisibility() != visible)
      {
      return 2;
      }
    }
  return visible;
}

//---------------------------------------------------------------------------
void vtkMRMLDisplayableNode::SetDisplayVisibility(int visible)
{
  if (visible == 2)
    {
    return;
    }

  int ndnodes = this->GetNumberOfDisplayNodes();
  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLDisplayNode *displayNode = this->GetNthDisplayNode(i);
    if (displayNode)
      {
      displayNode->SetVisibility(visible);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLDisplayableNode::GetRASBounds(double bounds[6])
{
  vtkMath::UninitializeBounds(bounds);
}

