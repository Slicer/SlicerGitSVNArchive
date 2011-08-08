/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

/// Slicer logic includes
#include "vtkSlicerColorLogic.h"
#include "vtkSlicerModelsLogic.h"

/// MRML includes
#include <vtkCacheManager.h>
#include <vtkMRMLClipModelsNode.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLFreeSurferModelOverlayStorageNode.h>
#include <vtkMRMLFreeSurferModelStorageNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLTransformNode.h>

/// VTK includes
#include <vtkGeneralTransform.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkTagTable.h>

/// ITK includes
#include <itksys/Directory.hxx>
#include <itksys/SystemTools.hxx>

/// STD includes
#include <cassert>

vtkCxxRevisionMacro(vtkSlicerModelsLogic, "$Revision$");
vtkStandardNewMacro(vtkSlicerModelsLogic);

//----------------------------------------------------------------------------
vtkSlicerModelsLogic::vtkSlicerModelsLogic()
{
  this->ActiveModelNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerModelsLogic::~vtkSlicerModelsLogic()
{
  if (this->ActiveModelNode != NULL)
    {
        this->ActiveModelNode->Delete();
        this->ActiveModelNode = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::ProcessMRMLEvents(vtkObject* caller,
                                            unsigned long event,
                                            void* callData)
{
  this->Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkSmartPointer<vtkIntArray> sceneEvents = vtkSmartPointer<vtkIntArray>::New();
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, sceneEvents);

  if (newScene && newScene->GetNthNodeByClass(0, "vtkMRMLClipModelsNode") == 0)
    {
    // vtkMRMLClipModelsNode is a singleton
    vtkMRMLClipModelsNode* clipNode = vtkMRMLClipModelsNode::New();
    newScene->AddNode(clipNode);
    clipNode->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::OnMRMLSceneNodeRemovedEvent(vtkMRMLNode* node)
{
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
  if (!modelNode || this->GetMRMLScene()->GetIsUpdating())
    {
    return;
    }
  std::vector<vtkMRMLDisplayNode*> displayNodes = modelNode->GetDisplayNodes();
  for (unsigned int i = 0; i < displayNodes.size(); ++i)
    {
    assert(displayNodes[i]);
    this->GetMRMLScene()->RemoveNode(displayNodes[i]);
    }
  std::vector<vtkMRMLStorageNode*> storageNodes = modelNode->GetStorageNodes();
  for (unsigned int i = 0; i < storageNodes.size(); ++i)
    {
    assert(storageNodes[i]);
    this->GetMRMLScene()->RemoveNode(storageNodes[i]);
    }

}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::SetActiveModelNode(vtkMRMLModelNode *activeNode)
{
  vtkSetMRMLNodeMacro(this->ActiveModelNode, activeNode );
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkSlicerModelsLogic::AddModels (const char* dirname, const char* suffix )
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
        if (this->AddModel(fullPath.c_str()) == NULL) 
          {
          res = 0;
          }
        }
      }
  }
  return res;
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkSlicerModelsLogic::AddModel (const char* filename)
{
  vtkMRMLModelNode *modelNode = vtkMRMLModelNode::New();
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  vtkSmartPointer<vtkMRMLModelStorageNode> mStorageNode = vtkSmartPointer<vtkMRMLModelStorageNode>::New();
  vtkSmartPointer<vtkMRMLFreeSurferModelStorageNode> fsmStorageNode = vtkSmartPointer<vtkMRMLFreeSurferModelStorageNode>::New();
  fsmStorageNode->SetUseStripper(0);  // turn off stripping by default (breaks some pickers)
  vtkSmartPointer<vtkMRMLStorageNode> storageNode = NULL;

  // check for local or remote files
  int useURI = 0; // false;
  if (this->GetMRMLScene()->GetCacheManager() != NULL)
    {
    useURI = this->GetMRMLScene()->GetCacheManager()->IsRemoteReference(filename);
    vtkDebugMacro("AddModel: file name is remote: " << filename);
    }
  
  itksys_stl::string name;
  const char *localFile;
  if (useURI)
    {
    mStorageNode->SetURI(filename);
    fsmStorageNode->SetURI(filename);
    // reset filename to the local file name
    localFile = ((this->GetMRMLScene())->GetCacheManager())->GetFilenameFromURI(filename);
    }
  else
    {
    mStorageNode->SetFileName(filename);
    fsmStorageNode->SetFileName(filename);
    localFile = filename;
    }
  const itksys_stl::string fname(localFile);
  // the model name is based on the file name (itksys call should work even if
  // file is not on disk yet)
  name = itksys::SystemTools::GetFilenameName(fname);
  vtkDebugMacro("AddModel: got model name = " << name.c_str());
  
  // check to see which node can read this type of file
  if (mStorageNode->SupportedFileType(name.c_str()))
    {
    storageNode = mStorageNode;
    }
  else if (fsmStorageNode->SupportedFileType(name.c_str()))
    {
    vtkDebugMacro("AddModel: have a freesurfer type model file.");
    storageNode = fsmStorageNode;
    }

  /* don't read just yet, need to add to the scene first for remote reading
  if (mStorageNode->ReadData(modelNode) != 0)
    {
    storageNode = mStorageNode;
    }
  else if (fsmStorageNode->ReadData(modelNode) != 0)
    {
    storageNode = fsmStorageNode;
    }
  */
  if (storageNode != NULL)
    {
    itksys_stl::string baseName = itksys::SystemTools::GetFilenameWithoutExtension(fname);
    std::string uname( this->GetMRMLScene()->GetUniqueNameByString(baseName.c_str()));
    modelNode->SetName(uname.c_str());

    this->GetMRMLScene()->SaveStateForUndo();

    modelNode->SetScene(this->GetMRMLScene());
    storageNode->SetScene(this->GetMRMLScene());
    displayNode->SetScene(this->GetMRMLScene()); 

    this->GetMRMLScene()->AddNode(storageNode);  
    this->GetMRMLScene()->AddNode(displayNode);
    modelNode->SetAndObserveStorageNodeID(storageNode->GetID());
    modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());  
    displayNode->SetPolyData(modelNode->GetPolyData());
    
    this->GetMRMLScene()->AddNode(modelNode);  

    //this->Modified();  

    // the scene points to it still
    modelNode->Delete();

    // now set up the reading
    vtkDebugMacro("AddModel: calling read on the storage node");
    int retval = storageNode->ReadData(modelNode);
    if (retval != 1)
      {
      vtkErrorMacro("AddModel: error reading " << filename);
      this->GetMRMLScene()->RemoveNode(modelNode);
      this->GetMRMLScene()->RemoveNode(storageNode);
      this->GetMRMLScene()->RemoveNode(displayNode);
      modelNode = NULL;
      }
    }
  else
    {
    vtkDebugMacro("Couldn't read file, returning null model node: " << filename);
    modelNode->Delete();
    modelNode = NULL;
    }

  return modelNode;  
}
//----------------------------------------------------------------------------
int vtkSlicerModelsLogic::SaveModel (const char* filename, vtkMRMLModelNode *modelNode)
{
   if (modelNode == NULL || filename == NULL)
    {
    vtkErrorMacro("SaveModel: unable to proceed, filename is " <<
                  (filename == NULL ? "null" : filename) <<
                  ", model node is " <<
                  (modelNode == NULL ? "null" : modelNode->GetID()));
    return 0;
    }
  
  vtkMRMLModelStorageNode *storageNode = NULL;
  vtkMRMLStorageNode *snode = modelNode->GetStorageNode();
  if (snode != NULL)
    {
    storageNode = vtkMRMLModelStorageNode::SafeDownCast(snode);
    }
  if (storageNode == NULL)
    {
    storageNode = vtkMRMLModelStorageNode::New();
    storageNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(storageNode);  
    modelNode->SetAndObserveStorageNodeID(storageNode->GetID());
    storageNode->Delete();
    }

  // check for a remote file
  if ((this->GetMRMLScene()->GetCacheManager() != NULL) &&
      this->GetMRMLScene()->GetCacheManager()->IsRemoteReference(filename))
    {
    storageNode->SetURI(filename);
    }
  else
    {
    storageNode->SetFileName(filename);
    }
  
  int res = storageNode->WriteData(modelNode);

  
  return res;

}


//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

  os << indent << "vtkSlicerModelsLogic:             " << this->GetClassName() << "\n";

  os << indent << "ActiveModelNode: " <<
    (this->ActiveModelNode ? this->ActiveModelNode->GetName() : "(none)") << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkSlicerModelsLogic::AddScalar(const char* filename, vtkMRMLModelNode *modelNode)
{
  if (modelNode == NULL ||
      filename == NULL)
    {
    vtkErrorMacro("Model node or file name are null.");
    return 0;
    }  

  vtkMRMLFreeSurferModelOverlayStorageNode *fsmoStorageNode = vtkMRMLFreeSurferModelOverlayStorageNode::New();
  vtkMRMLStorageNode *storageNode = NULL;
  
  // check for local or remote files
  int useURI = 0; //false ;
  if (this->GetMRMLScene()->GetCacheManager() != NULL)
    {
    useURI = this->GetMRMLScene()->GetCacheManager()->IsRemoteReference(filename);
    vtkDebugMacro("AddScalar: file name is remote: " << filename);
    }

  const char *localFile;
  if (useURI)
    {
    fsmoStorageNode->SetURI(filename);
    // add other overlay storage nodes here
    localFile = ((this->GetMRMLScene())->GetCacheManager())->GetFilenameFromURI(filename);
    }
  else
    {
    fsmoStorageNode->SetFileName(filename);
    // add other overlay storage nodes here
    localFile = filename;
    }

  // check to see if it can read it
  if (fsmoStorageNode->SupportedFileType(localFile))
    {
    storageNode = fsmoStorageNode;
    }

  // check to see if the model display node has a colour node already
  vtkMRMLModelDisplayNode *displayNode = modelNode->GetModelDisplayNode();
  if (displayNode == NULL)
    {
    vtkWarningMacro("Model " << modelNode->GetName() << "'s display node is null\n");
    }
  else
    {
    vtkMRMLColorNode *colorNode = vtkMRMLColorNode::SafeDownCast(displayNode->GetColorNode());
    if (colorNode == NULL)
      {
      vtkSlicerColorLogic *colorLogic = vtkSlicerColorLogic::New();
      displayNode->SetAndObserveColorNodeID(colorLogic->GetDefaultModelColorNodeID());
      colorLogic->Delete();
      }
    }

  if (storageNode != NULL)
    {
    this->GetMRMLScene()->SaveStateForUndo();
    storageNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(storageNode);
    // now add this as another storage node on the model
    modelNode->AddAndObserveStorageNodeID(storageNode->GetID());
    
    // now read, since all the id's are set up
    vtkDebugMacro("AddScalar: calling read data now.");
    if (this->GetDebug()) { storageNode->DebugOn(); }
    int retval = storageNode->ReadData(modelNode);
    if (retval == 0)
      {
      vtkErrorMacro("AddScalar: error adding scalar " << filename);
      this->GetMRMLScene()->RemoveNode(storageNode);
      fsmoStorageNode->Delete();
      return storageNode;
      }

    //--- informatics
    //--- tag for informatics and invoke event for anyone who cares.
    vtkTagTable *tt = modelNode->GetUserTagTable();
    if ( tt != NULL )
      {
      if ( storageNode->IsA("vtkMRMLFreeSurferModelOverlayStorageNode"))
        {
        modelNode->SetSlicerDataType ( "FreeSurferModelWithOverlay" );
        tt = modelNode->GetUserTagTable();
        tt->AddOrUpdateTag ( "SlicerDataType", modelNode->GetSlicerDataType() );
        }
      else if ( storageNode-IsA("vtkMRMLFreeSurferModelStorageNode"))
        {
        modelNode->SetSlicerDataType ( "FreeSurferModel" );
        tt = modelNode->GetUserTagTable();
        tt->AddOrUpdateTag ( "SlicerDataType", modelNode->GetSlicerDataType() );
        }
      }
    //--- end informatics

    }
  fsmoStorageNode->Delete();
  
  return storageNode;
}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::TransformModel(vtkMRMLTransformNode *tnode, 
                                          vtkMRMLModelNode *modelNode,
                                          int transformNormals,
                                          vtkMRMLModelNode *modelOut)
{
  if (!modelNode || !modelOut || !tnode)
    {
    return;
    }

  vtkPolyData *poly = vtkPolyData::New();
  modelOut->SetAndObservePolyData(poly);
  poly->Delete();

  poly->DeepCopy(modelNode->GetPolyData());

  vtkMRMLTransformNode *mtnode = modelNode->GetParentTransformNode();

  vtkGeneralTransform *transform = tnode->GetTransformToParent();
  modelOut->ApplyTransform(transform);

  if (transformNormals)
    {
    // fix normals
    //--- NOTE: This filter recomputes normals for polygons and
    //--- triangle strips only. Normals are not computed for lines or vertices.
    //--- Triangle strips are broken up into triangle polygons.
    //--- Polygons are not automatically re-stripped.
    vtkPolyDataNormals *normals = vtkPolyDataNormals::New();
    normals->SetInput ( poly );
    //--- NOTE: This assumes a completely closed surface
    //---(i.e. no boundary edges) and no non-manifold edges.
    //--- If these constraints do not hold, the AutoOrientNormals
    //--- is not guaranteed to work.
    normals->AutoOrientNormalsOn();
    //--- Flipping modifies both the normal direction
    //--- and the order of a cell's points.
    normals->FlipNormalsOn();
    normals->SplittingOff();
    //--- enforce consistent polygon ordering.
    normals->ConsistencyOn();

    normals->Update();
    modelOut->SetAndObservePolyData(normals->GetOutput());

    normals->Delete();
   }

  modelOut->SetAndObserveTransformNodeID(mtnode == NULL ? NULL : mtnode->GetID());

  return;
}

