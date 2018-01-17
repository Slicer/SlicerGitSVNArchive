/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women\"s Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLGlyphableVolumeDisplayNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

#include "vtkMRMLColorNode.h"
#include "vtkMRMLGlyphableVolumeDisplayNode.h"
#include "vtkMRMLScene.h"

#include "vtk/Common/Core/vtkCallbackCommand.h"
#include "vtk/Common/Core/vtkObjectFactory.h"

#include <sstream>

// Initialize static member that controls resampling --
// old comment: "This offset will be changed to 0.5 from 0.0 per 2/8/2002 Slicer
// development meeting, to move ijk coordinates to voxel centers."
vtkCxxSetReferenceStringMacro(vtkMRMLGlyphableVolumeDisplayNode, GlyphColorNodeID);

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLGlyphableVolumeDisplayNode);

//----------------------------------------------------------------------------
vtkMRMLGlyphableVolumeDisplayNode::vtkMRMLGlyphableVolumeDisplayNode()
{
  // Strings

  this->GlyphColorNodeID = NULL;
  this->GlyphColorNode = NULL;
  this->VisualizationMode = vtkMRMLGlyphableVolumeDisplayNode::visModeScalar;
  // try setting a default greyscale color map
  //this->SetDefaultColorMap(0);
}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::SetDefaultColorMap(/*int isLabelMap*/)
{
  // set up a default color node
  // TODO: figure out if can use vtkSlicerColorLogic's helper methods
  /*if (isLabelMap)
    {
    this->SetAndObserveGlyphColorNodeID("vtkMRMLColorTableNodeLabels");
    }
  else
    {*/
  this->SetAndObserveGlyphColorNodeID("vtkMRMLColorTableNodeGrey");
  //  }
  if (this->GlyphColorNode == NULL)
    {
    vtkDebugMacro("vtkMRMLGlyphableVolumeDisplayNode: FAILED setting default  color node, it's still null\n")
    }
  else
    {
    vtkDebugMacro("vtkMRMLGlyphableVolumeDisplayNode: set up the default color node\n");
    }
}

//----------------------------------------------------------------------------
vtkMRMLGlyphableVolumeDisplayNode::~vtkMRMLGlyphableVolumeDisplayNode()
{
  this->SetAndObserveGlyphColorNodeID( NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  if (this->GlyphColorNodeID != NULL)
    {
    of << " glyphColorNodeRef=\"" << this->GlyphColorNodeID << "\"";
    }

  std::stringstream ss;
  ss << this->VisualizationMode;
  of << " visualizationMode=\"" << ss.str() << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID, newID);
  if (this->GlyphColorNodeID && !strcmp(oldID, this->GlyphColorNodeID))
    {
    this->SetGlyphColorNodeID(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "glyphColorNodeRef"))
      {
      this->SetGlyphColorNodeID(attValue);
      }
    if (!strcmp(attName, "visualizationMode"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->VisualizationMode;
      }

    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node\"s attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLGlyphableVolumeDisplayNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLGlyphableVolumeDisplayNode *node = (vtkMRMLGlyphableVolumeDisplayNode *) anode;

  this->SetGlyphColorNodeID(node->GlyphColorNodeID);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{

  Superclass::PrintSelf(os,indent);

 os << indent << "GlyphColorNodeID: " <<
    (this->GlyphColorNodeID ? this->GlyphColorNodeID : "(none)") << "\n";
 os << indent << "Visualization Mode:   " << this->VisualizationMode << "\n";
}

//-----------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::SetSceneReferences()
{
  this->Superclass::SetSceneReferences();
  this->Scene->AddReferencedNodeID(this->GlyphColorNodeID, this);
}

//-----------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::UpdateScene(vtkMRMLScene *scene)
{
   Superclass::UpdateScene(scene);

   this->SetAndObserveGlyphColorNodeID(this->GetGlyphColorNodeID());
}

//-----------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::UpdateReferences()
{
   Superclass::UpdateReferences();

  if (this->GlyphColorNodeID != NULL && this->Scene->GetNodeByID(this->GlyphColorNodeID) == NULL)
    {
    this->SetAndObserveGlyphColorNodeID(NULL);
    }
}

//----------------------------------------------------------------------------
vtkMRMLColorNode* vtkMRMLGlyphableVolumeDisplayNode::GetGlyphColorNode()
{
  vtkMRMLColorNode* node = NULL;
  if (this->GetScene() && this->GetGlyphColorNodeID() )
    {
    vtkMRMLNode* cnode = this->GetScene()->GetNodeByID(this->GlyphColorNodeID);
    node = vtkMRMLColorNode::SafeDownCast(cnode);
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::SetAndObserveGlyphColorNodeID(std::string glyphColorNodeID)
{
  this->SetAndObserveGlyphColorNodeID(glyphColorNodeID.c_str());
}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::SetAndObserveGlyphColorNodeID(const char *glyphColorNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->GlyphColorNode, NULL);

  this->SetGlyphColorNodeID(glyphColorNodeID);

  vtkMRMLColorNode *cnode = this->GetGlyphColorNode();
  if (cnode != NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->GlyphColorNode, cnode);
    }
  if (this->Scene)
    {
    this->Scene->AddReferencedNodeID(glyphColorNodeID, this);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event,
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  vtkMRMLColorNode *cnode = this->GetGlyphColorNode();
  if (cnode != NULL && cnode == vtkMRMLColorNode::SafeDownCast(caller) &&
      event ==  vtkCommand::ModifiedEvent)
    {
    this->InvokeEvent(vtkCommand::ModifiedEvent, NULL);
    }
  return;
}


