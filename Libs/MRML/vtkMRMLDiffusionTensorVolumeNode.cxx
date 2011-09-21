/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLVolumeNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/


#include "vtkObjectFactory.h"

#include "vtkMRMLDiffusionTensorVolumeNode.h"
#include "vtkMRMLDiffusionTensorVolumeDisplayNode.h"
#include "vtkMRMLNRRDStorageNode.h"

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLDiffusionTensorVolumeNode);

//----------------------------------------------------------------------------
vtkMRMLDiffusionTensorVolumeNode::vtkMRMLDiffusionTensorVolumeNode()
{
  this->Order = 2; //Second order Tensor
}

//----------------------------------------------------------------------------
void vtkMRMLDiffusionTensorVolumeNode::SetAndObserveDisplayNodeID(const char *displayNodeID)
{
  this->Superclass::SetAndObserveDisplayNodeID(displayNodeID);
  // Make sure the node added is a DiffusionTensorVolumeDisplayNode
  vtkMRMLNode* displayNode =  this->GetDisplayNode();
  if (displayNode && !vtkMRMLDiffusionTensorVolumeDisplayNode::SafeDownCast(displayNode))
    {
    vtkWarningMacro("SetAndObserveDisplayNodeID: The node to display " << displayNodeID << " can not display diffusion tensors");
    }
}

//----------------------------------------------------------------------------
vtkMRMLDiffusionTensorVolumeNode::~vtkMRMLDiffusionTensorVolumeNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLDiffusionTensorVolumeNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
 
  vtkIndent indent(nIndent);

}

//----------------------------------------------------------------------------
void vtkMRMLDiffusionTensorVolumeNode::ReadXMLAttributes(const char** atts)
{

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
  }      

} 


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLDiffusionTensorVolumeNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  //vtkMRMLDiffusionTensorVolumeNode *node = (vtkMRMLDiffusionTensorVolumeNode *) anode;
}

//----------------------------------------------------------------------------
void vtkMRMLDiffusionTensorVolumeNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID,newID);
}

//-----------------------------------------------------------
void vtkMRMLDiffusionTensorVolumeNode::UpdateReferences()
{
  Superclass::UpdateReferences();
}

//---------------------------------------------------------------------------
void vtkMRMLDiffusionTensorVolumeNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event, 
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLDiffusionTensorVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkMRMLDiffusionTensorVolumeDisplayNode* vtkMRMLDiffusionTensorVolumeNode
::GetDiffusionTensorVolumeDisplayNode()
{
  return vtkMRMLDiffusionTensorVolumeDisplayNode::SafeDownCast(this->GetDisplayNode());
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLDiffusionTensorVolumeNode::CreateDefaultStorageNode()
{
  return vtkMRMLNRRDStorageNode::New();
}

 
