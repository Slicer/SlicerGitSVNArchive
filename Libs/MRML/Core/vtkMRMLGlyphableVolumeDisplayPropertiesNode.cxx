/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLGlyphableVolumeDisplayPropertiesNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.0 $

=========================================================================auto=*/
#include <sstream>

#include "vtkObjectFactory.h"
#include <vtkPolyData.h>

#include "vtkMRMLGlyphableVolumeDisplayPropertiesNode.h"

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMRMLGlyphableVolumeDisplayPropertiesNode, GlyphSource, vtkPolyData);

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLGlyphableVolumeDisplayPropertiesNode);

//----------------------------------------------------------------------------
vtkMRMLGlyphableVolumeDisplayPropertiesNode::vtkMRMLGlyphableVolumeDisplayPropertiesNode()
{

  // Glyph general parameters
  this->GlyphScaleFactor = 50;

  // VTK Objects
  this->GlyphSource = NULL;
  this->UpdateGlyphSource();

}

//----------------------------------------------------------------------------
vtkMRMLGlyphableVolumeDisplayPropertiesNode::~vtkMRMLGlyphableVolumeDisplayPropertiesNode()
{
  if ( this->GlyphSource != NULL )
    {
    this->GlyphSource->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayPropertiesNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults

  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  of << indent << " glyphScaleFactor=\"" << this->GlyphScaleFactor << "\"";

}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayPropertiesNode::ReadXMLAttributes(const char** atts)
{

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
      attName = *(atts++);
      attValue = *(atts++);
      if (!strcmp(attName, "glyphScaleFactor"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> GlyphScaleFactor;
      }

  }

}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLGlyphableVolumeDisplayPropertiesNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLGlyphableVolumeDisplayPropertiesNode *node = (vtkMRMLGlyphableVolumeDisplayPropertiesNode *) anode;

  this->SetGlyphScaleFactor(node->GlyphScaleFactor);

}

//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayPropertiesNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << indent << "GlyphScaleFactor:             " << this->GlyphScaleFactor << "\n";
}


//----------------------------------------------------------------------------
void vtkMRMLGlyphableVolumeDisplayPropertiesNode::UpdateGlyphSource ( )
{
  vtkErrorMacro("Shouldn't be calling this");
}

