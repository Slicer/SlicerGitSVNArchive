/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLVolumeNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/
// MRML includes
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLVolumeArchetypeStorageNode.h"

// VTK includes
#include <vtkDataArray.h>
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkMatrix4x4.h>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLScalarVolumeNode);

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode::vtkMRMLScalarVolumeNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode::~vtkMRMLScalarVolumeNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  // For backward compatibility, we read the labelMap attribute and save it as a custom attribute.
  // This allows scene reader to detect that this node has to be converted to a segmentation node.
  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "labelMap"))
      {
      std::stringstream ss;
      int val;
      ss << attValue;
      ss >> val;
      if (val)
        {
        this->SetAttribute("LabelMap", "1");
        }
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLScalarVolumeNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}

//-----------------------------------------------------------
void vtkMRMLScalarVolumeNode::CreateNoneNode(vtkMRMLScene *scene)
{
  // Create a None volume RGBA of 0, 0, 0 so the filters won't complain
  // about missing input
  vtkNew<vtkImageData> id;
  id->SetDimensions(1, 1, 1);
#if (VTK_MAJOR_VERSION <= 5)
  id->SetNumberOfScalarComponents(4);
  id->AllocateScalars();
#else
  id->AllocateScalars(VTK_DOUBLE, 4);
#endif
  id->GetPointData()->GetScalars()->FillComponent(0, 0.0);
  id->GetPointData()->GetScalars()->FillComponent(1, 125.0);
  id->GetPointData()->GetScalars()->FillComponent(2, 0.0);
  id->GetPointData()->GetScalars()->FillComponent(3, 0.0);

  vtkNew<vtkMRMLScalarVolumeNode> n;
  n->SetName("None");
  // the scene will set the id
  n->SetAndObserveImageData(id.GetPointer());
  scene->AddNode(n.GetPointer());
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeDisplayNode* vtkMRMLScalarVolumeNode::GetScalarVolumeDisplayNode()
{
  return vtkMRMLScalarVolumeDisplayNode::SafeDownCast(this->GetDisplayNode());
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//---------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLScalarVolumeNode::CreateDefaultStorageNode()
{
  return vtkMRMLVolumeArchetypeStorageNode::New();
}

void vtkMRMLScalarVolumeNode::GetReferenceSpace(const double ijk[3], const char *Space, double SpaceCoordinates[3])
{
  if (Space != NULL)
    {

    if (!strcmp(Space, "IJK"))
      {
        for (int i=0; i<3; i++)
          {
          SpaceCoordinates[i] = ijk[i];
          }
        return;
      }

    vtkSmartPointer<vtkMatrix4x4> IJKtoRAS =
      vtkSmartPointer<vtkMatrix4x4>::New();
    this->GetIJKToRASMatrix(IJKtoRAS);

    vtkSmartPointer<vtkMatrix4x4> Rotation =
       vtkSmartPointer<vtkMatrix4x4>::New();
    Rotation->Identity();
    double ijkw[4] = {ijk[0], ijk[1], ijk[2], 1.0 };
    double SpaceCoordinatesw[4] = {0.0, 0.0, 0.0, 1.0};

    if (!strcmp(Space, "LAS"))
      {
      Rotation->SetElement(1, 1, -1);
      Rotation->SetElement(2, 2, -1);
      }
    if (!strcmp(Space, "LPS") || !strcmp(Space, "scanner-xyz"))
      {
      Rotation->SetElement(0, 0, -1);
      Rotation->SetElement(2, 2, -1);
    }

    Rotation->Multiply4x4(IJKtoRAS, Rotation, Rotation);
    Rotation->MultiplyPoint(ijkw, SpaceCoordinatesw);

    for (int i=0; i<3; i++)
      {
      SpaceCoordinates[i] = SpaceCoordinatesw[i];
      }
    }
}
