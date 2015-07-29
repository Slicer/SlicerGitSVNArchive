/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women\"s Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLVolumeDisplayNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLVolumeDisplayNode.h"
#include "vtkMRMLVolumeNode.h"

// VTK includes
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkImageStencilData.h>
#include <vtkTrivialProducer.h>
#include <vtkStringArray.h>
#include <vtkNew.h>

// Initialize static member that controls resampling --
// old comment: "This offset will be changed to 0.5 from 0.0 per 2/8/2002 Slicer
// development meeting, to move ijk coordinates to voxel centers."

//----------------------------------------------------------------------------
vtkMRMLVolumeDisplayNode::vtkMRMLVolumeDisplayNode()
{
  // try setting a default greyscale color map
  //this->SetDefaultColorMap(0);
  this->SpaceQuantities = 0;
  this->Space = 0;
  this->SetSpaceQuantities("length;length;length");
  this->SetSpace("RAS");
  this->Tokens = vtkStringArray::New();
  this->Tokens->SetNumberOfComponents(1);
  this->Tokens->SetName("tokens");
}

//----------------------------------------------------------------------------
vtkMRMLVolumeDisplayNode::~vtkMRMLVolumeDisplayNode()
{
  if (this->SpaceQuantities)
    {
    delete [] this->SpaceQuantities;
    }
  if (this->Space)
    {
    delete [] this->Space;
    }
  if (this->Tokens)
    {
    this->Tokens->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeDisplayNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);

  of << indent << " SpaceQuantities=\"" << (this->SpaceQuantities ? this->SpaceQuantities : "") << "\"";
  of << indent << " Space=\"" << (this->Space ? this->Space : "") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeDisplayNode::ReadXMLAttributes(const char** atts)
{

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;

  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "SpaceQuantities"))
      {
      this->SetSpaceQuantities(attValue);
      continue;
      }

    if (!strcmp(attName, "Space"))
      {
      this->SetSpace(attValue);
      continue;
      }
    }

  this->WriteXML(std::cout,0);
}

//----------------------------------------------------------------------------
// Copy the node\"s attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLVolumeDisplayNode::Copy(vtkMRMLNode *anode)
{
  bool wasModifying = this->StartModify();
  this->Superclass::Copy(anode);
#if VTK_MAJOR_VERSION > 5
  vtkMRMLVolumeDisplayNode *node =
      vtkMRMLVolumeDisplayNode::SafeDownCast(anode);
  if (node)
    {
    this->SetInputImageDataConnection(node->GetInputImageDataConnection());
    this->SetSpaceQuantities(node->GetSpaceQuantities());
    this->SetSpace(node->GetSpace());
    }
  this->UpdateImageDataPipeline();
#endif

  this->EndModify(wasModifying);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeDisplayNode::ProcessMRMLEvents(vtkObject *caller,
                                                 unsigned long event,
                                                 void *callData)
{
  if (event ==  vtkCommand::ModifiedEvent)
    {
    this->UpdateImageDataPipeline();
    }
  this->Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SpaceQuantities: " <<
    (this->SpaceQuantities ? this->SpaceQuantities : "(none)") << "\n";
  os << indent << "Space: " <<
    (this->Space ? this->Space : "(none)") << "\n";
}

//-----------------------------------------------------------
void vtkMRMLVolumeDisplayNode::UpdateScene(vtkMRMLScene *scene)
{
  this->Superclass::UpdateScene(scene);
}

//-----------------------------------------------------------
void vtkMRMLVolumeDisplayNode::UpdateReferences()
{
  this->Superclass::UpdateReferences();
}

//---------------------------------------------------------------------------
#if (VTK_MAJOR_VERSION <= 5)
vtkImageData* vtkMRMLVolumeDisplayNode::GetImageData()
{
  if (!this->GetInputImageData())
    {
    return 0;
    }
  this->UpdateImageDataPipeline();
  return this->GetOutputImageData();
}
#else
vtkAlgorithmOutput* vtkMRMLVolumeDisplayNode::GetImageDataConnection()
{
/*
  if (!this->GetInputImageData())
    {
    return 0;
    }
  this->UpdateImageDataPipeline();
*/
  return this->GetOutputImageDataConnection();
}
#endif

//----------------------------------------------------------------------------
#if (VTK_MAJOR_VERSION <= 5)
void vtkMRMLVolumeDisplayNode
::SetInputImageData(vtkImageData *imageData)
{
  if (this->GetInputImageData() == imageData)
    {
    return;
    }
  this->SetInputToImageDataPipeline(imageData);
  this->Modified();
}
#else
void vtkMRMLVolumeDisplayNode
::SetInputImageDataConnection(vtkAlgorithmOutput *imageDataConnection)
{
  if (this->GetInputImageDataConnection() == imageDataConnection)
    {
    return;
    }
  this->SetInputToImageDataPipeline(imageDataConnection);
  this->Modified();
}
//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMRMLVolumeDisplayNode
::GetInputImageDataConnection()
{
  return 0;
}
#endif

//----------------------------------------------------------------------------
#if (VTK_MAJOR_VERSION <= 5)
void vtkMRMLVolumeDisplayNode::SetInputToImageDataPipeline(vtkImageData *vtkNotUsed(imageData))
{
}
#else
void vtkMRMLVolumeDisplayNode::SetInputToImageDataPipeline(vtkAlgorithmOutput *vtkNotUsed(imageDataConnection))
{
}
#endif

//----------------------------------------------------------------------------
vtkImageData* vtkMRMLVolumeDisplayNode::GetInputImageData()
{
#if (VTK_MAJOR_VERSION <= 5)
  return NULL;
#else
  vtkAlgorithmOutput* imageConnection = this->GetInputImageDataConnection();
  vtkAlgorithm* producer = imageConnection ? imageConnection->GetProducer() : 0;
  return vtkImageData::SafeDownCast(
    producer ? producer->GetOutputDataObject(0) : 0);
#endif
}

//----------------------------------------------------------------------------
#if (VTK_MAJOR_VERSION <= 5)
void vtkMRMLVolumeDisplayNode::SetBackgroundImageStencilData(vtkImageStencilData* vtkNotUsed(imageData))
{
}
#else
void vtkMRMLVolumeDisplayNode::SetBackgroundImageStencilDataConnection(vtkAlgorithmOutput* vtkNotUsed(imageDataConnection))
{
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMRMLVolumeDisplayNode::GetBackgroundImageStencilDataConnection()
{
  return 0;
}
#endif

//----------------------------------------------------------------------------
vtkImageStencilData* vtkMRMLVolumeDisplayNode::GetBackgroundImageStencilData()
{
#if (VTK_MAJOR_VERSION <= 5)
  return NULL;
}
#else
  vtkAlgorithmOutput* imageConnection = this->GetBackgroundImageStencilDataConnection();
  vtkAlgorithm* producer = imageConnection ? imageConnection->GetProducer() : 0;
  return vtkImageStencilData::SafeDownCast(
    producer ? producer->GetOutputDataObject(0) : 0);
}
#endif

//----------------------------------------------------------------------------
vtkImageData* vtkMRMLVolumeDisplayNode::GetOutputImageData()
{
#if (VTK_MAJOR_VERSION <= 5)
  return NULL;
}
#else
  vtkAlgorithmOutput* imageConnection = this->GetOutputImageDataConnection();
  vtkAlgorithm* producer = imageConnection ? imageConnection->GetProducer() : 0;
  return vtkImageData::SafeDownCast(
    producer ? producer->GetOutputDataObject(0) : 0);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMRMLVolumeDisplayNode::GetOutputImageDataConnection()
{
  return NULL;
}
#endif

//----------------------------------------------------------------------------
void vtkMRMLVolumeDisplayNode::UpdateImageDataPipeline()
{
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeDisplayNode::SetDefaultColorMap()
{
  this->SetAndObserveColorNodeID("vtkMRMLColorTableNodeGrey");
}

vtkStringArray* vtkMRMLVolumeDisplayNode::GetSpaceQuantitiesList()
{
  if (this->Tokens)
    {
    this->Tokens->Delete();
    this->Tokens = vtkStringArray::New();
    this->Tokens->SetNumberOfComponents(1);
    this->Tokens->SetName("tokens");
    }
  std::istringstream f(this->SpaceQuantities);
  std::string s;
  while (std::getline(f, s, ';'))
    {
    this->Tokens->InsertNextValue(s);
    }
  return this->Tokens;
}

//----------------------------------------------------------------------------
vtkMRMLVolumeNode* vtkMRMLVolumeDisplayNode::GetVolumeNode()
{
  return vtkMRMLVolumeNode::SafeDownCast(this->GetDisplayableNode());
}

//----------------------------------------------------------------------------
#if (VTK_MAJOR_VERSION <= 5)
vtkImageData* vtkMRMLVolumeDisplayNode::GetUpToDateImageData()
{
  vtkImageData* imageData = this->GetImageData();
  if (!imageData)
    {
    return NULL;
    }
  imageData->Update();
  return imageData;
}
#endif
