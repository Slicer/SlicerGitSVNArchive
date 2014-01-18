// SlicerLogic includes
#include "vtkSlicerSceneViewsModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSceneViewNode.h>
#include <vtkMRMLSceneViewStorageNode.h>
#include <vtkMRMLHierarchyNode.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSlicerSceneViewsModuleLogic, "$Revision: 1.0$")
vtkStandardNewMacro(vtkSlicerSceneViewsModuleLogic)

const char SCENE_VIEW_TOP_LEVEL_SINGLETON_TAG[] = "SceneViewTopLevel";

//-----------------------------------------------------------------------------
// vtkSlicerSceneViewsModuleLogic methods
//-----------------------------------------------------------------------------
vtkSlicerSceneViewsModuleLogic::vtkSlicerSceneViewsModuleLogic()
{
  this->m_LastAddedSceneViewNode = 0;
  this->ActiveHierarchyNodeID = NULL;
}

//-----------------------------------------------------------------------------
vtkSlicerSceneViewsModuleLogic::~vtkSlicerSceneViewsModuleLogic()
{
  // let go of pointer to the last added node
  if (this->m_LastAddedSceneViewNode)
    {
    this->m_LastAddedSceneViewNode = 0;
    }

  if (this->ActiveHierarchyNodeID != NULL)
    {
    delete [] this->ActiveHierarchyNodeID;
    this->ActiveHierarchyNodeID = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkDebugMacro("SetMRMLSceneInternal - listening to scene events");
  
  vtkIntArray *events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
//  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndRestoreEvent);
//  events->InsertNextValue(vtkMRMLScene::SceneAboutToBeRestoredEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events);
  events->Delete();
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::AddMissingHierarchyNodes()
{
  vtkDebugMacro("AddMissingHierarchyNodes");

  if (!this->GetMRMLScene())
    {
    return;
    }
  
  // don't do anything if the scene is still updating
  if (this->GetMRMLScene() &&
      this->GetMRMLScene()->IsBatchProcessing())
    {
    vtkDebugMacro("AddMissingHierarchyNodes: updating, returning");
    return;
    }

  vtkSmartPointer<vtkCollection> sceneViewNodes;
  sceneViewNodes.TakeReference(this->GetMRMLScene()->GetNodesByClass("vtkMRMLSceneViewNode"));
  unsigned int numNodes = sceneViewNodes->GetNumberOfItems();
  vtkDebugMacro("AddMissingHierarchyNodes: have " << numNodes << " scene view nodes");
  for (unsigned int n = 0; n < numNodes; n++)
    {
    vtkMRMLSceneViewNode *sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(sceneViewNodes->GetItemAsObject(n));
    vtkMRMLHierarchyNode *hierarchyNode =  NULL;
    hierarchyNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(sceneViewNode->GetScene(), sceneViewNode->GetID());
    if (hierarchyNode)
      {
      vtkMRMLHierarchyNode *topLevelNode = hierarchyNode->GetParentNode();
      if (topLevelNode != NULL)
        {
        if (topLevelNode->GetSingletonTag() == NULL)
          {
          // If the top-level hierarchy node is just read from the scene then it is not yet set up as a singleton.
          // The top-level node has to be a singleton, so its singleton tag now.
          topLevelNode->SetSingletonTag(SCENE_VIEW_TOP_LEVEL_SINGLETON_TAG);
          }
        }
      }
    else
      {
      vtkDebugMacro("AddMissingHierarchyNodes: missing a hierarchy node for scene view node " << sceneViewNode->GetID() << ", adding one");
      int retval = this->AddHierarchyNodeForNode(sceneViewNode);
      if (!retval)
        {
        vtkErrorMacro("AddMissingHierarchyNodes: failed to add a missing a hierarchy node for scene view node " << sceneViewNode->GetID());
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLSceneNodeAddedEvent");

  // don't do anything if the scene is still updating
  if (this->GetMRMLScene() &&
      this->GetMRMLScene()->IsBatchProcessing())
    {
    vtkDebugMacro("OnMRMLSceneNodeAddedEvent: updating, returning");
    return;
    }
  
  vtkMRMLSceneViewNode * sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(node);
  if (!sceneViewNode)
    {
    return;
    }

  // only deal with this special case: the master scene view that's created
  // when the scene is saved needs a hierarchy added for it
  if (!sceneViewNode->GetName() ||
      (sceneViewNode->GetName() &&
       strncmp(sceneViewNode->GetName(),"Master Scene View", 17) != 0))
    {
    vtkDebugMacro("OnMRMLSceneNodeAddedEvent: Not a master scene view node, node added is named " << sceneViewNode->GetName());
    return;
    }
  vtkDebugMacro("OnMRMLSceneNodeAddedEvent: master scene view added");
  
  int retval = this->AddHierarchyNodeForNode(sceneViewNode);
  vtkMRMLHierarchyNode* hierarchyNode = NULL;
  if (!retval)
    {
    vtkErrorMacro("OnMRMLSceneNodeAddedEvent: error adding a hierarchy node for scene view node");
    return;
    }
  hierarchyNode =  vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(sceneViewNode->GetScene(), sceneViewNode->GetID());
  if (!hierarchyNode)
    {
    vtkErrorMacro("OnMRMLSceneNodeAddedEvent: No hierarchyNode found.")
    return;
    }
  hierarchyNode->SetAssociatedNodeID(sceneViewNode->GetID());
  sceneViewNode->Modified();

  // we pass the hierarchy node along - it includes the pointer to the actual sceneViewNode
  this->AddNodeCompleted(hierarchyNode);
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLSceneEndImport()
{
  vtkDebugMacro("OnMRMLSceneEndImport");

  // this may have been an imported scene with old style snapshot nodes and no
  // hierarchies, so fill in some hierarchies
  this->AddMissingHierarchyNodes();
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLSceneEndRestore()
{
  vtkDebugMacro("OnMRMLSceneEndRestore");
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLNodeModified(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLNodeModifiedEvent " << node->GetID());

  if (this->GetMRMLScene() &&
      this->GetMRMLScene()->IsBatchProcessing())
    {
    vtkDebugMacro("OnMRMLNodeModifiedEvent: updating, returning");
    return;
    }
  
  vtkMRMLSceneViewNode * sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(node);
  if (!sceneViewNode)
    {
    return;
    }
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLSceneEndClose()
{
  this->m_LastAddedSceneViewNode = 0;

  // this is important: otherwise adding a new scene view node might end up
  // setting it's parent to itself
  this->SetActiveHierarchyNodeID(NULL);
}


//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::RegisterNodes()
{

  if (!this->GetMRMLScene())
    {
    std::cerr << "RegisterNodes: no scene on which to register nodes" << std::endl;
    return;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::New();
  this->GetMRMLScene()->RegisterNodeClass(viewNode);
  // SceneSnapshot backward compatibility
#if MRML_SUPPORT_VERSION < 0x040000
  this->GetMRMLScene()->RegisterNodeClass(viewNode, "SceneSnapshot");
#endif
  viewNode->Delete();

  vtkMRMLSceneViewStorageNode *storageNode = vtkMRMLSceneViewStorageNode::New();
  this->GetMRMLScene()->RegisterNodeClass ( storageNode );
  storageNode->Delete();
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::CreateSceneView(const char* name, const char* description, int screenshotType, vtkImageData* screenshot)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return;
    }

  if (!screenshot)
    {
    vtkErrorMacro("CreateSceneView: No screenshot was set.")
    return;
    }

  vtkStdString nameString = vtkStdString(name);

  vtkNew<vtkMRMLSceneViewNode> newSceneViewNode;
  newSceneViewNode->SetScene(this->GetMRMLScene());
  if (strcmp(nameString,""))
    {
    // a name was specified
    newSceneViewNode->SetName(nameString.c_str());
    }
  else
    {
    // if no name is specified, generate a new unique one
    newSceneViewNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("SceneView"));
    }

  vtkStdString descriptionString = vtkStdString(description);

  newSceneViewNode->SetSceneViewDescription(descriptionString);
  newSceneViewNode->SetScreenShotType(screenshotType);

  // make a new vtk image data, as the set macro is taking the pointer
  vtkNew<vtkImageData> copyScreenShot;
  copyScreenShot->DeepCopy(screenshot);
  newSceneViewNode->SetScreenShot(copyScreenShot.GetPointer());
  newSceneViewNode->StoreScene();
  //newSceneViewNode->HideFromEditorsOff();
  // mark it modified since read so that the screen shot will get saved to disk

  this->GetMRMLScene()->AddNode(newSceneViewNode.GetPointer());

  // put it in a hierarchy
  if (!this->AddHierarchyNodeForNode(newSceneViewNode.GetPointer()))
    {
    vtkErrorMacro("CreateSceneView: Error adding a hierarchy node for new scene view node " << newSceneViewNode->GetID());
    }
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::ModifySceneView(vtkStdString id, const char* name, const char* description, int screenshotType, vtkImageData* screenshot)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return;
    }

  if (!screenshot)
    {
    vtkErrorMacro("ModifySceneView: No screenshot was set.")
    return;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id.c_str()));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewName: Could not get sceneView node!")
    return;
    }

  vtkStdString nameString = vtkStdString(name);
  if (strcmp(nameString,""))
    {
    // a name was specified
    viewNode->SetName(nameString.c_str());
    }
  else
    {
    // if no name is specified, generate a new unique one
    viewNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("SceneView"));
    }

  vtkStdString descriptionString = vtkStdString(description);
  viewNode->SetSceneViewDescription(descriptionString);
  viewNode->SetScreenShotType(screenshotType);
  viewNode->SetScreenShot(screenshot);

  // TODO: Listen to the node directly, probably in OnMRMLSceneNodeAddedEvent
  this->OnMRMLNodeModified(viewNode);
}

//---------------------------------------------------------------------------
vtkStdString vtkSlicerSceneViewsModuleLogic::GetSceneViewName(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return 0;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewName: Could not get sceneView node!")
    return 0;
    }

  return vtkStdString(viewNode->GetName());
}

//---------------------------------------------------------------------------
vtkStdString vtkSlicerSceneViewsModuleLogic::GetSceneViewDescription(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return 0;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewDescription: Could not get sceneView node!")
    return 0;
    }

  return viewNode->GetSceneViewDescription();
}

//---------------------------------------------------------------------------
int vtkSlicerSceneViewsModuleLogic::GetSceneViewScreenshotType(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return -1;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewScreenshotType: Could not get sceneView node!")
    return -1;
    }

  return viewNode->GetScreenShotType();
}

//---------------------------------------------------------------------------
vtkImageData* vtkSlicerSceneViewsModuleLogic::GetSceneViewScreenshot(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return 0;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewScreenshot: Could not get sceneView node!")
    return 0;
    }

  return viewNode->GetScreenShot();
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::RestoreSceneView(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("RestoreSceneView: Could not get sceneView node!")
    return;
    }

  this->GetMRMLScene()->SaveStateForUndo();
  viewNode->RestoreScene();
}

//---------------------------------------------------------------------------
const char* vtkSlicerSceneViewsModuleLogic::MoveSceneViewUp(const char* id)
{
  // reset stringHolder
  this->m_StringHolder = "";

  if (!id)
    {
    return this->m_StringHolder.c_str();
    }

  this->m_StringHolder = id;

  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return this->m_StringHolder.c_str();
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("MoveSceneViewUp: Could not get sceneView node! (id = " << id << ")")
    return this->m_StringHolder.c_str();
    }

  // see if it's in a hierarchy
  vtkMRMLHierarchyNode *hNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), id);
  if (!hNode)
    {
    vtkWarningMacro("MoveSceneViewUp: did not find a hierarchy node for node with id " << id);
    return this->m_StringHolder.c_str();
    }
  // where is it in the parent's list?
  int currentIndex = hNode->GetIndexInParent();
  // now move it up one
  hNode->SetIndexInParent(currentIndex - 1);
  // if it succeeded, trigger a modified event on the node to get the GUI to
  // update
  if (hNode->GetIndexInParent() != currentIndex)
    {
    std::cout << "MoveSceneViewUp: calling mod on scene view node" << std::endl;
    viewNode->Modified();
    }
  // the id should be the same now
  this->m_StringHolder = viewNode->GetID();
  return this->m_StringHolder.c_str();
}

//---------------------------------------------------------------------------
const char* vtkSlicerSceneViewsModuleLogic::MoveSceneViewDown(const char* id)
{
  // reset stringHolder
  this->m_StringHolder = "";

  if (!id)
    {
    return this->m_StringHolder.c_str();
    }

  this->m_StringHolder = id;


  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return this->m_StringHolder.c_str();
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("MoveSceneViewDown: Could not get sceneView node!")
    return this->m_StringHolder.c_str();
    }

  // see if it's in a hierarchy
  vtkMRMLHierarchyNode *hNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), id);
  if (!hNode)
    {
    vtkWarningMacro("MoveSceneViewDown: Temporarily disabled (did not find a hierarchy node for node with id " << id << ")");
    return this->m_StringHolder.c_str();
    }
  // where is it in the parent's list?
  int currentIndex = hNode->GetIndexInParent();
  // now move it down one
  hNode->SetIndexInParent(currentIndex + 1);
  // the id should be the same now
  this->m_StringHolder = viewNode->GetID();
  return this->m_StringHolder.c_str();
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::RemoveSceneViewNode(vtkMRMLSceneViewNode *sceneViewNode)
{
  if (!sceneViewNode)
    {
    vtkErrorMacro("RemoveSceneViewNode: No node to remove");
    return;
    }

  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("RemoveSceneViewNode: No MRML Scene found from which to remove the node");
    return;
    }

  // remove the 1-1 IS-A hierarchy node first
  vtkMRMLHierarchyNode *hNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), sceneViewNode->GetID());
  if (hNode)
    {
    // there is a parent node
    this->GetMRMLScene()->RemoveNode(hNode);
    }

  this->GetMRMLScene()->RemoveNode(sceneViewNode);

}

//---------------------------------------------------------------------------
int vtkSlicerSceneViewsModuleLogic::AddHierarchyNodeForNode(vtkMRMLNode* node)
{
  // check that there isn't already a hierarchy node for this node
  if (node && node->GetScene() && node->GetID())
    {
    vtkMRMLHierarchyNode *hnode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(node->GetScene(), node->GetID());
    if (hnode != NULL)
      {
      vtkWarningMacro("AddHierarchyNodeForNode: scene view node " << node->GetID() << " already has a hierarchy node, returning.");
      return true;
      }
    }
  if (!this->GetActiveHierarchyNodeID())
    {
    vtkDebugMacro("AddHierarchyNodeForNode: no active hierarchy...");
    // no active hierarchy node, this means we create the new node directly under the top-level hierarchy node
    char * toplevelHierarchyNodeID = NULL;
    if (!node)
      {
      // we just add a new toplevel hierarchy node
      toplevelHierarchyNodeID = this->GetTopLevelHierarchyNodeID(0);
      }
    else
      {
      // we need to insert the new toplevel hierarchy before the given node
      toplevelHierarchyNodeID = this->GetTopLevelHierarchyNodeID(node);
      }

    if (!toplevelHierarchyNodeID)
      {
      vtkErrorMacro("AddHierarchyNodeForNode: Toplevel hierarchy node was NULL.")
      return 0;
      }

    this->SetActiveHierarchyNodeID(toplevelHierarchyNodeID);
    }

  // Create a hierarchy node
  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::New();
  if (hierarchyNode == NULL)
    {
    vtkErrorMacro("AddHierarchyNodeForNode: can't create a new hierarchy node to associate with scene view " << node->GetID());
    return 0;
    }

  hierarchyNode->SetParentNodeID(this->GetActiveHierarchyNodeID());
  hierarchyNode->SetScene(this->GetMRMLScene());

  if (!node)
    {
    // this is a user created hierarchy!

    // we want to see that!
    hierarchyNode->HideFromEditorsOff();

    hierarchyNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("SceneViewHierarchy"));
    hierarchyNode->SetAttribute("SceneViewHierarchy", "true");

    this->GetMRMLScene()->AddNode(hierarchyNode);

    // we want it to be the active hierarchy from now on
    this->SetActiveHierarchyNodeID(hierarchyNode->GetID());
    }
  else
    {
    // this is the 1-1 hierarchy node for a given node

    // we do not want to see that!
    hierarchyNode->HideFromEditorsOn();

    hierarchyNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("SceneViewHierarchy"));

    this->GetMRMLScene()->InsertBeforeNode(node,hierarchyNode);
    hierarchyNode->SetAssociatedNodeID(node->GetID());
    vtkDebugMacro("AddHierarchyNodeForNode: added hierarchy node, id = " << (hierarchyNode->GetID() ? hierarchyNode->GetID() : "null") << ", set associated node id on the hierarchy node of " << (hierarchyNode->GetAssociatedNodeID() ? hierarchyNode->GetAssociatedNodeID() : "null"));
    }
  
  hierarchyNode->Delete();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSlicerSceneViewsModuleLogic::AddHierarchy()
{
  return this->AddHierarchyNodeForNode(0);
}

//---------------------------------------------------------------------------
char * vtkSlicerSceneViewsModuleLogic::GetTopLevelHierarchyNodeID(vtkMRMLNode* node)
{

  if (this->GetMRMLScene() == NULL)
    {
    return NULL;
    }
  if (this->GetMRMLScene()->IsBatchProcessing() ||
      this->GetMRMLScene()->IsImporting() ||
      this->GetMRMLScene()->IsRestoring())
    {
    vtkDebugMacro("GetTopLevelHierarchyNodeID: Scene is busy, returning null");
    return NULL;
    }
  
  vtkMRMLNode *toplevelNode = this->GetMRMLScene()->GetSingletonNode(SCENE_VIEW_TOP_LEVEL_SINGLETON_TAG, "vtkMRMLHierarchyNode");
  if (!toplevelNode)
    {
    // there wasn't a top level node, create one
    vtkDebugMacro("GetTopLevelHierarchyNode: no top level node, making new" );
    vtkNew<vtkMRMLHierarchyNode> createdToplevelNode;
    createdToplevelNode->SetSingletonTag(SCENE_VIEW_TOP_LEVEL_SINGLETON_TAG);
    createdToplevelNode->HideFromEditorsOff();
    createdToplevelNode->SetName("Scene Views");
    createdToplevelNode->SetAttribute("SceneViewHierarchy", "true");
    if (!node)
      {
      toplevelNode = this->GetMRMLScene()->AddNode(createdToplevelNode.GetPointer());
      }
    else
      {
      toplevelNode = this->GetMRMLScene()->InsertBeforeNode(node,createdToplevelNode.GetPointer());
      }
    if (toplevelNode)
      {
      vtkDebugMacro("Added a new top level node with id " << toplevelNode->GetID()
        << " and name " << ( toplevelNode->GetName() ? toplevelNode->GetName() : "undefined" ) );
      }
    else
      {
      vtkErrorMacro("Failed to add top-level scene view node");
      }

    }
  char *toplevelNodeID = ( toplevelNode ? toplevelNode->GetID() : NULL );

  // sanity check
  this->FlattenSceneViewsHierarchy(toplevelNodeID);
  
  return toplevelNodeID;

}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::FlattenSceneViewsHierarchy(const char *toplevelNodeID)
{
  if (!this->GetMRMLScene() ||
      this->GetMRMLScene()->IsBatchProcessing())
    {
    return;
    }
  // a null top level id means make everything a child of the scene
  // sanity check: all scene view nodes should have hierarchies that point
  // to the top level one, enforcing a one level hierarchy
  vtkSmartPointer<vtkCollection> sceneViewCol;
  sceneViewCol.TakeReference(this->GetMRMLScene()->GetNodesByClass("vtkMRMLSceneViewNode"));
  int numSceneViews = sceneViewCol.GetPointer()->GetNumberOfItems();
  vtkDebugMacro("FlattenSceneViewsHierarchy: number of scene view nodes = " << numSceneViews );
  for (int s = 0; s <  numSceneViews; s++)
    {
    // get the hierarchy node
    vtkMRMLSceneViewNode *sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(sceneViewCol.GetPointer()->GetItemAsObject(s));
    vtkMRMLHierarchyNode *hNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), sceneViewNode->GetID());
    if (hNode)
      {
      if (hNode->GetParentNodeID() == NULL || toplevelNodeID == NULL ||
          (toplevelNodeID != NULL &&
           strcmp(hNode->GetParentNodeID(), toplevelNodeID) != 0))
        {
        vtkDebugMacro("\tWARNING: scene view node " << sceneViewNode->GetID() << "'s hierarchy node " << hNode->GetID() << " doesn't have a correct parent node id, resetting to the top level" );
        hNode->SetParentNodeID(toplevelNodeID);
        }
      }
    }
}

//--------------------------------------------------------------------------- 
vtkMRMLHierarchyNode * vtkSlicerSceneViewsModuleLogic::GetActiveHierarchyNode()
{
  if (!this->GetActiveHierarchyNodeID())
    {
    // there was no active hierarchy
    // we then use the toplevel hierarchyNode
    char* toplevelNodeID = this->GetTopLevelHierarchyNodeID();

    if (!toplevelNodeID)
      {
      vtkErrorMacro("SetActiveHierarchyNodeByID: Could not find or create any hierarchy.")
      return NULL;
      }

    this->SetActiveHierarchyNodeID(toplevelNodeID);
    }
  if (this->GetMRMLScene()->GetNodeByID(this->GetActiveHierarchyNodeID()) == NULL)
    {
    return NULL;
    }
  return vtkMRMLHierarchyNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->GetActiveHierarchyNodeID()));
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::AddNodeCompleted(vtkMRMLHierarchyNode* hierarchyNode)
{

  if (!hierarchyNode)
    {
    return;
    }

  vtkMRMLSceneViewNode* sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(hierarchyNode->GetAssociatedNode());

  if (!sceneViewNode)
    {
    vtkErrorMacro("AddNodeCompleted: Could not get scene view node.")
    return;
    }

  this->m_LastAddedSceneViewNode = sceneViewNode;

}
