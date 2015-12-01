/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

/// Slicer logic includes
#include "vtkSlicerColorLogic.h"
#include "vtkSlicerModelsLogic.h"
#include "vtkMRMLSliceLogic.h"

/// MRML includes
#include <vtkCacheManager.h>
#include <vtkMRMLClipModelsNode.h>
#include <vtkMRMLFreeSurferModelOverlayStorageNode.h>
#include <vtkMRMLFreeSurferModelStorageNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformNode.h>

/// VTK includes
#include <vtkAlgorithmOutput.h>
#include <vtkGeneralTransform.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkTagTable.h>

/// ITK includes
#include <itksys/Directory.hxx>
#include <itksys/SystemTools.hxx>

/// STD includes
#include <cassert>

vtkStandardNewMacro(vtkSlicerModelsLogic);
vtkCxxSetObjectMacro(vtkSlicerModelsLogic, ColorLogic, vtkMRMLColorLogic);

//----------------------------------------------------------------------------
vtkSlicerModelsLogic::vtkSlicerModelsLogic()
{
  this->ActiveModelNode = NULL;
  this->ColorLogic = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerModelsLogic::~vtkSlicerModelsLogic()
{
  if (this->ActiveModelNode != NULL)
    {
    this->ActiveModelNode->Delete();
    this->ActiveModelNode = NULL;
    }
  if (this->ColorLogic != NULL)
    {
    this->ColorLogic->Delete();
    this->ColorLogic = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> sceneEvents;
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, sceneEvents.GetPointer());
}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::ObserveMRMLScene()
{
  if (this->GetMRMLScene() &&
      this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLClipModelsNode") == 0)
    {
    // vtkMRMLClipModelsNode is a singleton
    this->GetMRMLScene()->AddNode(vtkSmartPointer<vtkMRMLClipModelsNode>::New());
    }
  this->Superclass::ObserveMRMLScene();
}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::SetActiveModelNode(vtkMRMLModelNode *activeNode)
{
  vtkSetMRMLNodeMacro(this->ActiveModelNode, activeNode );
  this->Modified();
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkSlicerModelsLogic::AddModel(vtkPolyData* polyData)
{
  if (this->GetMRMLScene() == 0)
    {
    return 0;
    }

  vtkNew<vtkMRMLModelDisplayNode> display;
  this->GetMRMLScene()->AddNode(display.GetPointer());

  vtkNew<vtkMRMLModelNode> model;
  model->SetAndObservePolyData(polyData);
  model->SetAndObserveDisplayNodeID(display->GetID());
  this->GetMRMLScene()->AddNode(model.GetPointer());

  return model.GetPointer();
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkSlicerModelsLogic::AddModel(vtkAlgorithmOutput* polyData)
{
  if (this->GetMRMLScene() == 0)
    {
    return 0;
    }

  vtkNew<vtkMRMLModelDisplayNode> display;
  this->GetMRMLScene()->AddNode(display.GetPointer());

  vtkNew<vtkMRMLModelNode> model;
  model->SetPolyDataConnection(polyData);
  model->SetAndObserveDisplayNodeID(display->GetID());
  this->GetMRMLScene()->AddNode(model.GetPointer());

  return model.GetPointer();
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
  if (this->GetMRMLScene() == 0 ||
      filename == 0)
    {
    return 0;
    }
  vtkNew<vtkMRMLModelNode> modelNode;
  vtkNew<vtkMRMLModelDisplayNode> displayNode;
  vtkNew<vtkMRMLModelStorageNode> mStorageNode;
  vtkNew<vtkMRMLFreeSurferModelStorageNode> fsmStorageNode;
  fsmStorageNode->SetUseStripper(0);  // turn off stripping by default (breaks some pickers)
  vtkSmartPointer<vtkMRMLStorageNode> storageNode;

  // check for local or remote files
  int useURI = 0; // false;
  if (this->GetMRMLScene()->GetCacheManager() != NULL)
    {
    useURI = this->GetMRMLScene()->GetCacheManager()->IsRemoteReference(filename);
    vtkDebugMacro("AddModel: file name is remote: " << filename);
    }
  const char *localFile=0;
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
  const std::string fname(localFile?localFile:"");
  // the model name is based on the file name (itksys call should work even if
  // file is not on disk yet)
  std::string name = itksys::SystemTools::GetFilenameName(fname);
  vtkDebugMacro("AddModel: got model name = " << name.c_str());

  // check to see which node can read this type of file
  if (mStorageNode->SupportedFileType(name.c_str()))
    {
    storageNode = mStorageNode.GetPointer();
    }
  else if (fsmStorageNode->SupportedFileType(name.c_str()))
    {
    vtkDebugMacro("AddModel: have a freesurfer type model file.");
    storageNode = fsmStorageNode.GetPointer();
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
    std::string baseName = itksys::SystemTools::GetFilenameWithoutExtension(fname);
    std::string uname( this->GetMRMLScene()->GetUniqueNameByString(baseName.c_str()));
    modelNode->SetName(uname.c_str());

    this->GetMRMLScene()->SaveStateForUndo();

    this->GetMRMLScene()->AddNode(storageNode.GetPointer());
    this->GetMRMLScene()->AddNode(displayNode.GetPointer());

    // Set the scene so that SetAndObserve[Display|Storage]NodeID can find the
    // node in the scene (so that DisplayNodes return something not empty)
    modelNode->SetScene(this->GetMRMLScene());
    modelNode->SetAndObserveStorageNodeID(storageNode->GetID());
    modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());

    this->GetMRMLScene()->AddNode(modelNode.GetPointer());

    // now set up the reading
    vtkDebugMacro("AddModel: calling read on the storage node");
    int retval = storageNode->ReadData(modelNode.GetPointer());
    if (retval != 1)
      {
      vtkErrorMacro("AddModel: error reading " << filename);
      this->GetMRMLScene()->RemoveNode(modelNode.GetPointer());
      }
    }
  else
    {
    vtkErrorMacro("Couldn't read file: " << filename);
    return 0;
    }

  return modelNode.GetPointer();
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
  if (this->ColorLogic)
    {
    os << indent << "ColorLogic: ";
    this->ColorLogic->PrintSelf(os, indent);
    }
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
  if (this->GetMRMLScene() &&
      this->GetMRMLScene()->GetCacheManager() != NULL)
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
    vtkMRMLColorNode *colorNode = displayNode->GetColorNode();
    if (colorNode == NULL && this->ColorLogic != NULL)
      {
      displayNode->SetAndObserveColorNodeID(
        this->ColorLogic->GetDefaultModelColorNodeID());
      }
    }

  if (storageNode != NULL)
    {
    this->GetMRMLScene()->SaveStateForUndo();
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
      return NULL;
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

  vtkNew<vtkPolyData> poly;
  modelOut->SetAndObservePolyData(poly.GetPointer());

  poly->DeepCopy(modelNode->GetPolyData());

  vtkMRMLTransformNode *mtnode = modelNode->GetParentTransformNode();

  vtkAbstractTransform *transform = tnode->GetTransformToParent();
  modelOut->ApplyTransform(transform);

  if (transformNormals)
    {
    // fix normals
    //--- NOTE: This filter recomputes normals for polygons and
    //--- triangle strips only. Normals are not computed for lines or vertices.
    //--- Triangle strips are broken up into triangle polygons.
    //--- Polygons are not automatically re-stripped.
    vtkNew<vtkPolyDataNormals> normals;
    normals->SetInputData(poly.GetPointer());
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
    modelOut->SetPolyDataConnection(normals->GetOutputPort());
   }

  modelOut->SetAndObserveTransformNodeID(mtnode == NULL ? NULL : mtnode->GetID());

  return;
}

//----------------------------------------------------------------------------
void vtkSlicerModelsLogic::SetAllModelsVisibility(int flag)
{
  if (this->GetMRMLScene() == 0)
    {
    return;
    }

  std::vector<vtkMRMLNode *> selectionNodes;
  vtkMRMLSelectionNode *selectionNode = 0;
  if (this->GetMRMLScene())
    {
    this->GetMRMLScene()->GetNodesByClass("vtkMRMLSelectionNode", selectionNodes);
    }

  if (selectionNodes.size() > 0)
    {
    selectionNode = vtkMRMLSelectionNode::SafeDownCast(selectionNodes[0]);
    }

  std::map<std::string, std::string> displayNodeClasses =
    selectionNode->GetModelHierarchyDisplayNodeClassNames();
  std::map<std::string, std::string>::iterator it;

  int numModels = this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLModelNode");

  // go into batch processing mode
  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState);
  for (int i = 0; i < numModels; i++)
    {
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNthNodeByClass(i, "vtkMRMLModelNode");
    // Exclude volume slice model nodes.
    // Exclude vtkMRMLModelNode subclasses by comparing classname.
    // Doing so will avoid updating annotation and fiber bundle node
    // visibility since they derive from vtkMRMLModelNode
    // See http://www.na-mic.org/Bug/view.php?id=2576
    if (mrmlNode != NULL
        && !vtkMRMLSliceLogic::IsSliceModelNode(mrmlNode)
        && strcmp(mrmlNode->GetClassName(), "vtkMRMLModelNode") == 0)
      {
      vtkMRMLModelNode *modelNode = vtkMRMLModelNode::SafeDownCast(mrmlNode);
      if (modelNode)
        {
        // have a "real" model node, set the display visibility
        modelNode->SetDisplayVisibility(flag);
        }
      }

    if (flag != 2 && mrmlNode != NULL
        && !vtkMRMLSliceLogic::IsSliceModelNode(mrmlNode) )
      {
      vtkMRMLModelNode *modelNode = vtkMRMLModelNode::SafeDownCast(mrmlNode);
      int ndnodes = modelNode->GetNumberOfDisplayNodes();
      for (int i=0; i<ndnodes; i++)
        {
        vtkMRMLDisplayNode *displayNode = modelNode->GetNthDisplayNode(i);
        if (displayNode)
          {
          for (it = displayNodeClasses.begin(); it != displayNodeClasses.end(); it++)
            {
            if (!strcmp(displayNode->GetClassName(), it->second.c_str()))
              {
              displayNode->SetVisibility(flag);
              }
            }
          }
        }
      }
    }
  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);
}
