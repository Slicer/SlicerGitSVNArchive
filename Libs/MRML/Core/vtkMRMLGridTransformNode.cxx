/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLGridTransformNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLGridTransformNode.h"

// VTK includes
#include <vtkGeneralTransform.h>
#include <vtkGridTransform.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLGridTransformNode);

//----------------------------------------------------------------------------
vtkMRMLGridTransformNode::vtkMRMLGridTransformNode()
{
  this->ReadWriteAsTransformToParent = 0;
  vtkNew<vtkGridTransform> warp;
  this->SetAndObserveTransformFromParent(warp.GetPointer());
}

//----------------------------------------------------------------------------
vtkMRMLGridTransformNode::~vtkMRMLGridTransformNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLGridTransformNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkGridTransform* grid=NULL;

  if (this->ReadWriteAsTransformToParent)
    {
    grid=vtkGridTransform::SafeDownCast(GetTransformToParentAs("vtkGridTransform"));
    }
  else
    {
    grid=vtkGridTransform::SafeDownCast(GetTransformFromParentAs("vtkGridTransform"));
    }

  if (grid != NULL)
    {
    // this transform should be a grid transform
    of << " interpolationMode=\"" << grid->GetInterpolationMode() << "\" ";
    of << " displacementScale=\"" << grid->GetDisplacementScale() << "\" ";
    of << " displacementShift=\"" << grid->GetDisplacementShift() << "\" ";
    }
  else
    {
    vtkErrorMacro("vtkMRMLGridTransformNode::WriteXML failed: the transform is not a vtkGridTransform");
    }

}

//----------------------------------------------------------------------------
void vtkMRMLGridTransformNode::ReadXMLAttributes(const char** atts)
{

  Superclass::ReadXMLAttributes(atts);

  vtkNew<vtkGridTransform> vtkgrid;

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "interpolationMode"))
      {
      std::stringstream ss;
      ss << attValue;
      int val;
      if( ss >> val )
        {
        vtkgrid->SetInterpolationMode(val);
        }
      else
        {
        vtkErrorMacro( "couldn't parse grid interpolationMode" );
        return;
        }
      }
    else if (!strcmp(attName, "displacementScale"))
      {
      double val;
      std::stringstream ss;
      ss << attValue;
      if( ss >> val )
        {
        vtkgrid->SetDisplacementScale(val);
        }
      else
        {
        vtkErrorMacro( "couldn't parse grid displacementScale" );
        return;
        }
      }
    else if (!strcmp(attName, "displacementShift"))
      {
      double val;
      std::stringstream ss;
      ss << attValue;
      if( ss >> val )
        {
        vtkgrid->SetDisplacementShift(val);
        }
      else
        {
        vtkErrorMacro( "couldn't parse grid displacementShift" );
        return;
        }
      }
    }

  if (this->ReadWriteAsTransformToParent)
    {
    this->SetAndObserveTransformToParent(vtkgrid.GetPointer());
    }
  else
    {
    this->SetAndObserveTransformFromParent(vtkgrid.GetPointer());
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLGridTransformNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}

//----------------------------------------------------------------------------
void vtkMRMLGridTransformNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
