/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkSlicerFiberBundleLogic.cxx,v $
  Date:      $Date: 2006/01/06 17:56:48 $
  Version:   $Revision: 1.58 $

=========================================================================auto=*/

#include "vtkSlicerFiberBundleLogic.h"

// MRML includes
#include <vtkMRMLConfigure.h>
#ifdef MRML_USE_vtkTeem
#include "vtkMRMLDiffusionTensorDisplayPropertiesNode.h"
#include "vtkMRMLFiberBundleNode.h"
#include "vtkMRMLFiberBundleDisplayNode.h"
#include "vtkMRMLFiberBundleStorageNode.h"
#include "vtkMRMLFiberBundleLineDisplayNode.h"
#include "vtkMRMLFiberBundleTubeDisplayNode.h"
#include "vtkMRMLFiberBundleGlyphDisplayNode.h"
#endif
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>

#include <itksys/SystemTools.hxx>
#include <itksys/Directory.hxx>

vtkStandardNewMacro(vtkSlicerFiberBundleLogic);

//----------------------------------------------------------------------------
vtkSlicerFiberBundleLogic::vtkSlicerFiberBundleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerFiberBundleLogic::~vtkSlicerFiberBundleLogic()
{
}

//----------------------------------------------------------------------------
int vtkSlicerFiberBundleLogic::AddFiberBundles (const char* dirname, const char* suffix )
{
  std::string ssuf = suffix;
  itksys::Directory dir;
  dir.Load(dirname);

  int nfiles = dir.GetNumberOfFiles();
  int res = 1;
  for (int i=0; i<nfiles; i++) {
    const char* filename = dir.GetFile(i);
    std::string sname = filename;
    if (!itksys::SystemTools::FileIsDirectory(filename))
      {
      if ( sname.find(ssuf) != std::string::npos )
        {
        std::string fullPath = std::string(dir.GetPath())
            + "/" + filename;
        if (this->AddFiberBundle(fullPath.c_str()) == NULL)
          {
          res = 0;
          }
        }
      }
  }
  return res;
}

//----------------------------------------------------------------------------
int vtkSlicerFiberBundleLogic::AddFiberBundles (const char* dirname, std::vector< std::string > suffix )
{
  itksys::Directory dir;
  dir.Load(dirname);

  int nfiles = dir.GetNumberOfFiles();
  int res = 1;
  for (int i=0; i<nfiles; i++) {
    const char* filename = dir.GetFile(i);
    std::string sname = filename;
    if (!itksys::SystemTools::FileIsDirectory(filename))
      {
      for (unsigned int s=0; s<suffix.size(); s++)
        {
        std::string ssuf = suffix[s];
        if ( sname.find(ssuf) != std::string::npos )
          {
          std::string fullPath = std::string(dir.GetPath())
              + "/" + filename;
          if (this->AddFiberBundle(fullPath.c_str()) == NULL)
            {
            res = 0;
            }
          } //if (sname
        } // for (int s=0;
      }
  }
  return res;
}

//----------------------------------------------------------------------------
vtkMRMLFiberBundleNode* vtkSlicerFiberBundleLogic::AddFiberBundle (const char* filename)
{
  vtkDebugMacro("Adding fiber bundle from filename " << filename);

  vtkMRMLFiberBundleNode *fiberBundleNode = vtkMRMLFiberBundleNode::New();
  vtkMRMLFiberBundleLineDisplayNode *displayLineNode = vtkMRMLFiberBundleLineDisplayNode::New();
  vtkMRMLFiberBundleTubeDisplayNode *displayTubeNode = vtkMRMLFiberBundleTubeDisplayNode::New();
  vtkMRMLFiberBundleGlyphDisplayNode *displayGlyphNode = vtkMRMLFiberBundleGlyphDisplayNode::New();
  vtkMRMLFiberBundleStorageNode *storageNode = vtkMRMLFiberBundleStorageNode::New();

  vtkMRMLDiffusionTensorDisplayPropertiesNode *lineDTDPN = vtkMRMLDiffusionTensorDisplayPropertiesNode::New();
  vtkMRMLDiffusionTensorDisplayPropertiesNode *tubeDTDPN = vtkMRMLDiffusionTensorDisplayPropertiesNode::New();
  vtkMRMLDiffusionTensorDisplayPropertiesNode *glyphDTDPN = vtkMRMLDiffusionTensorDisplayPropertiesNode::New();
  glyphDTDPN->SetGlyphGeometry(vtkMRMLDiffusionTensorDisplayPropertiesNode::Ellipsoids);

  storageNode->SetFileName(filename);
  if (storageNode->ReadData(fiberBundleNode) != 0)
    {
    const std::string fname(filename);
    const std::string name = itksys::SystemTools::GetFilenameWithoutExtension(fname);
    const std::string uname( this->GetMRMLScene()->GetUniqueNameByString(name.c_str()));
    fiberBundleNode->SetName(uname.c_str());

    displayLineNode->SetVisibility(1);
    displayTubeNode->SetVisibility(0);
    displayGlyphNode->SetVisibility(0);

    fiberBundleNode->SetScene(this->GetMRMLScene());
    storageNode->SetScene(this->GetMRMLScene());
    displayLineNode->SetScene(this->GetMRMLScene());
    displayTubeNode->SetScene(this->GetMRMLScene());
    displayGlyphNode->SetScene(this->GetMRMLScene());

    this->GetMRMLScene()->SaveStateForUndo();
    this->GetMRMLScene()->AddNode(lineDTDPN);
    this->GetMRMLScene()->AddNode(tubeDTDPN);
    this->GetMRMLScene()->AddNode(glyphDTDPN);

    displayLineNode->SetAndObserveDiffusionTensorDisplayPropertiesNodeID(lineDTDPN->GetID());
    displayTubeNode->SetAndObserveDiffusionTensorDisplayPropertiesNodeID(tubeDTDPN->GetID());
    displayGlyphNode->SetAndObserveDiffusionTensorDisplayPropertiesNodeID(glyphDTDPN->GetID());

    this->GetMRMLScene()->AddNode(storageNode);
    this->GetMRMLScene()->AddNode(displayLineNode);
    this->GetMRMLScene()->AddNode(displayTubeNode);
    this->GetMRMLScene()->AddNode(displayGlyphNode);

    fiberBundleNode->SetAndObserveStorageNodeID(storageNode->GetID());
    displayLineNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    displayTubeNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    displayGlyphNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");

    fiberBundleNode->SetAndObserveDisplayNodeID(displayLineNode->GetID());
    fiberBundleNode->AddAndObserveDisplayNodeID(displayTubeNode->GetID());
    fiberBundleNode->AddAndObserveDisplayNodeID(displayGlyphNode->GetID());

    this->GetMRMLScene()->AddNode(fiberBundleNode);

    if (!this->SetPolyDataTensors(fiberBundleNode))
      {
      vtkErrorMacro("No Tensors found in: " << filename);
      }

    // Set up display logic and any other logic classes in future
    //this->InitializeLogicForFiberBundleNode(fiberBundleNode);

    //this->Modified();

    fiberBundleNode->Delete();
    }
  else
    {
    vtkErrorMacro("Couldn't read file, returning null fiberBundle node: " << filename);
    fiberBundleNode->Delete();
    fiberBundleNode = NULL;
    }
  storageNode->Delete();
  displayLineNode->Delete();
  displayTubeNode->Delete();
  displayGlyphNode->Delete();

  //displayLogic->Delete();

  lineDTDPN->Delete();
  tubeDTDPN->Delete();
  glyphDTDPN->Delete();

  return fiberBundleNode;
}
//----------------------------------------------------------------------------
int vtkSlicerFiberBundleLogic::SaveFiberBundle (const char* filename, vtkMRMLFiberBundleNode *fiberBundleNode)
{
   if (fiberBundleNode == NULL || filename == NULL)
    {
    return 0;
    }

  vtkMRMLFiberBundleStorageNode *storageNode = NULL;
  vtkMRMLStorageNode *snode = fiberBundleNode->GetStorageNode();
  if (snode != NULL)
    {
    storageNode = vtkMRMLFiberBundleStorageNode::SafeDownCast(snode);
    }
  if (storageNode == NULL)
    {
    storageNode = vtkMRMLFiberBundleStorageNode::New();
    storageNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(storageNode);
    fiberBundleNode->SetAndObserveStorageNodeID(storageNode->GetID());
    storageNode->Delete();
    }

  //storageNode->SetAbsoluteFileName(true);
  storageNode->SetFileName(filename);

  int res = storageNode->WriteData(fiberBundleNode);

  return res;
}

//----------------------------------------------------------------------------
void vtkSlicerFiberBundleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

  os << indent << "vtkSlicerFiberBundleLogic:             " << this->GetClassName() << "\n";
}

//-----------------------------------------------------------------------------
void vtkSlicerFiberBundleLogic::RegisterNodes()
{
  if(!this->GetMRMLScene())
    {
    return;
    }
#ifdef MRML_USE_vtkTeem
  this->GetMRMLScene()->RegisterNodeClass(vtkNew<vtkMRMLFiberBundleNode>().GetPointer());
  this->GetMRMLScene()->RegisterNodeClass(vtkNew<vtkMRMLFiberBundleLineDisplayNode>().GetPointer());
  this->GetMRMLScene()->RegisterNodeClass(vtkNew<vtkMRMLFiberBundleTubeDisplayNode>().GetPointer());
  this->GetMRMLScene()->RegisterNodeClass(vtkNew<vtkMRMLFiberBundleGlyphDisplayNode>().GetPointer());
  this->GetMRMLScene()->RegisterNodeClass(vtkNew<vtkMRMLFiberBundleStorageNode>().GetPointer());
#endif
}

//------------------------------------------------------------------------------
bool vtkSlicerFiberBundleLogic::SetPolyDataTensors(vtkMRMLFiberBundleNode *fiberBundleNode)
{
  vtkPolyData *polyData = fiberBundleNode->GetPolyData();
  bool hasTensors = false;
  vtkDataArray *tensors = 0;
  if (polyData && polyData->GetPointData()->GetTensors() )
    {
    tensors = polyData->GetPointData()->GetTensors();
    hasTensors = true;
    }
  else if (polyData && polyData->GetPointData())
    {
    for (int i=0; i<polyData->GetPointData()->GetNumberOfArrays(); i++)
      {
      if (polyData->GetPointData()->GetArray(i)->GetNumberOfComponents() == 9)
        {
        if (polyData->GetPointData()->GetArray(i)->GetName() == 0)
          {
          std::stringstream ss;
          ss << "Tensor_" << i;
          polyData->GetPointData()->GetArray(i)->SetName(ss.str().c_str());
          }
        if (!hasTensors)
          {
          tensors = polyData->GetPointData()->GetArray(i);
          polyData->GetPointData()->SetTensors(tensors);
          hasTensors = true;
          }
        }
      }
    }

  if (tensors)
    {
    if (polyData->GetCellData())
      {
      polyData->GetCellData()->SetScalars(0);
      }
    vtkMRMLFiberBundleDisplayNode *dnode = fiberBundleNode->GetLineDisplayNode();
    if (dnode)
      {
      vtkDataArray *da = dnode->GetInputPolyData()->GetPointData()->GetArray(tensors->GetName());
      dnode->GetInputPolyData()->GetPointData()->SetTensors(da);

      dnode->SetActiveTensorName(tensors->GetName());
      }
    dnode = fiberBundleNode->GetTubeDisplayNode();
    if (dnode)
      {
      vtkDataArray *da = dnode->GetInputPolyData()->GetPointData()->GetArray(tensors->GetName());
      dnode->GetInputPolyData()->GetPointData()->SetTensors(da);

      dnode->SetActiveTensorName(tensors->GetName());
      }
    dnode = fiberBundleNode->GetGlyphDisplayNode();
    if (dnode)
      {
      vtkDataArray *da = dnode->GetInputPolyData()->GetPointData()->GetArray(tensors->GetName());
      dnode->GetInputPolyData()->GetPointData()->SetTensors(da);

      dnode->SetActiveTensorName(tensors->GetName());
      }
    }
  return hasTensors;
}
