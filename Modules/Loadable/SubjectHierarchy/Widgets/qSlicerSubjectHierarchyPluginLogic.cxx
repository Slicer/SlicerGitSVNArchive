/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Subject Hierarchy includes
#include "qSlicerSubjectHierarchyPluginLogic.h"

#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerSubjectHierarchyCloneNodePlugin.h"
#include "qSlicerSubjectHierarchyChartsPlugin.h"
#include "qSlicerSubjectHierarchyParseLocalDataPlugin.h"
#include "qSlicerSubjectHierarchyRegisterPlugin.h"
#include "qSlicerSubjectHierarchyFolderPlugin.h"

// SlicerQt includes
#include "qSlicerApplication.h"

// Qt includes
#include <QDebug>
#include <QString>
#include <QSettings>
#include <QMessageBox>

// MRML includes
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy
class qSlicerSubjectHierarchyPluginLogicPrivate
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyPluginLogic);
protected:
  qSlicerSubjectHierarchyPluginLogic* const q_ptr;
public:
  qSlicerSubjectHierarchyPluginLogicPrivate(qSlicerSubjectHierarchyPluginLogic& object);
  ~qSlicerSubjectHierarchyPluginLogicPrivate();
  void loadApplicationSettings();
public:
  /// Flag determining whether subject hierarchy children nodes are automatically
  /// deleted upon deleting a parent subject hierarchy node.
  bool AutoDeleteSubjectHierarchyChildren;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyPluginLogicPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyPluginLogicPrivate::qSlicerSubjectHierarchyPluginLogicPrivate(qSlicerSubjectHierarchyPluginLogic& object)
  : q_ptr(&object)
  , AutoDeleteSubjectHierarchyChildren(false)
{
  // Register vtkIdType for use in python for subject hierarchy item IDs
  qRegisterMetaType<vtkIdType>("vtkIdType");
  //qRegisterMetaType<QList<vtkIdType> >("QList<vtkIdType>"); //TODO: Allows returning it but cannot be used (e.g. pluginHandler->currentItems())
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyPluginLogicPrivate::~qSlicerSubjectHierarchyPluginLogicPrivate()
{
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogicPrivate::loadApplicationSettings()
{
  // Load settings
  QSettings* settings = qSlicerApplication::application()->settingsDialog()->settings();
  if (!settings)
    {
    qWarning() << Q_FUNC_INFO << ": Invalid application settings!";
    }
  else
    {
    if (settings->contains("SubjectHierarchy/AutoDeleteSubjectHierarchyChildren"))
      {
      this->AutoDeleteSubjectHierarchyChildren = (settings->value("SubjectHierarchy/AutoDeleteSubjectHierarchyChildren").toString().compare("true") == 0);
      }
    }
}

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyPluginLogic methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyPluginLogic::qSlicerSubjectHierarchyPluginLogic(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerSubjectHierarchyPluginLogicPrivate(*this) )
{
  Q_D(qSlicerSubjectHierarchyPluginLogic);
  d->loadApplicationSettings();

  // Register Subject Hierarchy core plugins
  this->registerCorePlugins();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyPluginLogic::~qSlicerSubjectHierarchyPluginLogic()
{
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::registerCorePlugins()
{
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(
    new qSlicerSubjectHierarchyFolderPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(
    new qSlicerSubjectHierarchyParseLocalDataPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(
    new qSlicerSubjectHierarchyCloneNodePlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(
    new qSlicerSubjectHierarchyChartsPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(
    new qSlicerSubjectHierarchyRegisterPlugin());
}

//-----------------------------------------------------------------------------
bool qSlicerSubjectHierarchyPluginLogic::autoDeleteSubjectHierarchyChildren()const
{
  Q_D(const qSlicerSubjectHierarchyPluginLogic);
  return d->AutoDeleteSubjectHierarchyChildren;
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::setAutoDeleteSubjectHierarchyChildren(bool flag)
{
  Q_D(qSlicerSubjectHierarchyPluginLogic);
  d->AutoDeleteSubjectHierarchyChildren = flag;
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyAbstractPlugin* qSlicerSubjectHierarchyPluginLogic::subjectHierarchyPluginByName(QString name)const
{
  return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName(name);
}

//-----------------------------------------------------------------------------
vtkIdType qSlicerSubjectHierarchyPluginLogic::currentSubjectHierarchyItem()const
{
  return qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::setCurrentSubjectHierarchyItem(vtkIdType itemID)
{
  qSlicerSubjectHierarchyPluginHandler::instance()->setCurrentItem(itemID);
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::setMRMLScene(vtkMRMLScene* scene)
{
  this->qSlicerObject::setMRMLScene(scene);

  // Set the new scene to the plugin handler
  qSlicerSubjectHierarchyPluginHandler::instance()->setMRMLScene(scene);

  // Connect scene node added event so that the new subject hierarchy nodes can be claimed by a plugin
  qvtkReconnect( scene, vtkMRMLScene::NodeAddedEvent, this, SLOT( onNodeAdded(vtkObject*,vtkObject*) ) );
  // Connect scene node added event so that the associated subject hierarchy node can be deleted too
  qvtkReconnect( scene, vtkMRMLScene::NodeAboutToBeRemovedEvent, this, SLOT( onNodeAboutToBeRemoved(vtkObject*,vtkObject*) ) );
  // Connect scene import ended event so that subject hierarchy nodes can be created for supported data nodes if missing (backwards compatibility)
  qvtkReconnect( scene, vtkMRMLScene::EndImportEvent, this, SLOT( onSceneImportEnded(vtkObject*) ) );
  // Connect scene close ended event so that subject hierarchy can be cleared
  qvtkReconnect( scene, vtkMRMLScene::EndCloseEvent, this, SLOT( onSceneCloseEnded(vtkObject*) ) );
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::onNodeAdded(vtkObject* sceneObject, vtkObject* nodeObject)
{
  Q_D(qSlicerSubjectHierarchyPluginLogic);

  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
    {
    return;
    }

  // If subject hierarchy node, then merge it with the already used subject hierarchy node (and remove the new one)
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nodeObject);
  if (subjectHierarchyNode)
    {
    // Calling this function makes sure that there is exactly one subject hierarchy node in the scene (performs the merge if more found)
    vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
    }
  // If data node, then add it to subject hierarchy
  else
    {
    vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(nodeObject);

    // Observe HierarchyModifiedEvent so that we can update subject hierarchy based on the other type of hierarchy the node is in
    qvtkConnect( node, vtkMRMLNode::HierarchyModifiedEvent, this, SLOT( onMRMLNodeHierarchyModified(vtkObject*) ) );

    // Add subject hierarchy node for the added data node
    // Don't add to subject hierarchy automatically one-by-one if importing scene, because the SH nodes may be stored in the scene and loaded
    // Also abort if invalid or hidden node or if explicitly excluded from subject hierarchy before even adding to the scene
    if ( scene->IsImporting()
      || !node
      || node->GetHideFromEditors()
      || node->GetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str()) )
      {
      return;
      }

    // If there is a plugin that can add the data node to subject hierarchy, then add
    QList<qSlicerSubjectHierarchyAbstractPlugin*> foundPlugins =
      qSlicerSubjectHierarchyPluginHandler::instance()->pluginsForAddingNodeToSubjectHierarchy(node);
    qSlicerSubjectHierarchyAbstractPlugin* selectedPlugin = NULL;
    if (foundPlugins.size() > 1)
      {
      // Let the user choose a plugin if more than one returned the same non-zero confidence value
      QString textToDisplay = QString("Equal confidence number found for more than one subject hierarchy plugin for adding new node to subject hierarchy.\n\nSelect plugin to add node named\n'%1'\n(type %2)").arg(node->GetName()).arg(node->GetNodeTagName());
      selectedPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->selectPluginFromDialog(textToDisplay, foundPlugins);
      }
     else if (foundPlugins.size() == 1)
      {
      selectedPlugin = foundPlugins[0];
      }
    // Have the selected plugin add the new node to subject hierarchy
    if (selectedPlugin)
      {
      // Get subject hierarchy node
      vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(node->GetScene());
      if (!shNode)
        {
        qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
        return;
        }
      // Add under subject hierarchy scene item
      bool successfullyAddedByPlugin = selectedPlugin->addNodeToSubjectHierarchy(node, shNode->GetSceneItemID());
      if (!successfullyAddedByPlugin)
        {
        qWarning() << Q_FUNC_INFO << ": Failed to add node " << node->GetName() <<
          " through plugin '" << selectedPlugin->name().toLatin1().constData() << "'";
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::onNodeAboutToBeRemoved(vtkObject* sceneObject, vtkObject* nodeObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene || scene->IsClosing())
    {
    // Do nothing if scene is closing
    return;
    }

  Q_D(qSlicerSubjectHierarchyPluginLogic);

  vtkMRMLNode* dataNode = vtkMRMLNode::SafeDownCast(nodeObject);
  if (!dataNode || dataNode->IsA("vtkMRMLSubjectHierarchyNode"))
    {
    return;
    }

  // Get subject hierarchy node
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(dataNode->GetScene());
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }

  // Remove associated subject hierarchy item if any
  vtkIdType itemID = shNode->GetItemByDataNode(dataNode);
  if (itemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    shNode->RemoveItem(itemID, false, false);
    }
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::onSceneImportEnded(vtkObject* sceneObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
    {
    return;
    }

  // Trigger merging the imported subject hierarchy node containing the unresolved items
  // into the singleton subject hierarchy node in the current scene. This would be done
  // when first accessing the subject hierarchy node, but it needs to be done so that the
  // addSupportedDataNodesToSubjectHierarchy call below only adds the nodes that were not
  // in the hierarchy stored by the imported scene
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);

  // Add data nodes that are supported (i.e. there is a plugin that can claim it) and were not
  // in the imported subject hierarchy node to subject hierarchy
  this->addSupportedDataNodesToSubjectHierarchy();
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::onSceneCloseEnded(vtkObject* sceneObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
    {
    return;
    }

  // Trigger creating new subject hierarchy node
  // (scene close removed the pseudo-singleton subject hierarchy node)
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": There must be a subject hierarchy node in the scene";
    return;
    }

  // Set subject hierarchy node to plugin handler
  qSlicerSubjectHierarchyPluginHandler::instance()->setSubjectHierarchyNode(shNode);
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::onMRMLNodeHierarchyModified(vtkObject* nodeObject)
{
  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(nodeObject);
  if (!node || node->IsA("vtkMRMLHierarchyNode"))
    // Only handle this event for non-hierarchy nodes, as the same event is fired for both
    // hierarchy and associated nodes, and also on other occasions, not just associations.
    {
    return;
    }
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(node->GetScene());
  if (!scene)
    {
    return;
    }

  //TODO: Use this function if real-time updating of SH is needed based on model hierarchy changes
  //  (one problem with it is what happens when the user moved the model in SH before that? That operation will be lost)

  //// Look for hierarchy nodes that are associated to the node in question
  //std::vector<vtkMRMLNode*> hierarchyNodes;
  //std::vector<vtkMRMLHierarchyNode*> associatedHierarchyNodes;
  //scene->GetNodesByClass("vtkMRMLHierarchyNode", hierarchyNodes);
  //for (std::vector<vtkMRMLNode*>::iterator hierarchyNodeIt = hierarchyNodes.begin(); hierarchyNodeIt != hierarchyNodes.end(); ++hierarchyNodeIt)
  //  {
  //  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(*hierarchyNodeIt);
  //  if ( hierarchyNode && hierarchyNode->GetAssociatedNodeID()
  //    && !strcmp(hierarchyNode->GetAssociatedNodeID(), node->GetID()) )
  //    {
  //    associatedHierarchyNodes.push_back(hierarchyNode);
  //    }
  //  }
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyPluginLogic::addSupportedDataNodesToSubjectHierarchy()
{
  // Get subject hierarchy node
  vtkMRMLScene* scene = this->mrmlScene();
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }

  // Traverse all storable nodes in the scene (those contain data that can be saved with the scene)
//TODO:
  // and all model hierarchy nodes
  std::vector<vtkMRMLNode*> storableNodes;
  scene->GetNodesByClass("vtkMRMLStorableNode", storableNodes);
  for (std::vector<vtkMRMLNode*>::iterator storableNodeIt = storableNodes.begin(); storableNodeIt != storableNodes.end(); ++storableNodeIt)
    {
    vtkMRMLNode* node = (*storableNodeIt);
    // Do not add into subject hierarchy if hidden, excluded, or already added
    if ( node->GetHideFromEditors()
      || node->GetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str())
      || shNode->GetItemByDataNode(node) != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID )
      {
      continue;
      }

    // If there is a plugin that can add the data node to subject hierarchy, then add
    QList<qSlicerSubjectHierarchyAbstractPlugin*> foundPlugins =
      qSlicerSubjectHierarchyPluginHandler::instance()->pluginsForAddingNodeToSubjectHierarchy(node, NULL);
    qSlicerSubjectHierarchyAbstractPlugin* selectedPlugin = NULL;
    if (foundPlugins.size() > 0)
      {
      // Choose first plugin in case of confidence equality not to annoy user (it can be changed later in subject hierarchy module)
      selectedPlugin = foundPlugins[0];
      }
    // Have the selected plugin add the new node to subject hierarchy
    if (selectedPlugin)
      {
      bool successfullyAddedByPlugin = selectedPlugin->addNodeToSubjectHierarchy(node, shNode->GetSceneItemID());
      if (!successfullyAddedByPlugin)
        {
        // Should never happen! If a plugin answers positively to the canOwn question (condition of
        // reaching this point), then it has to be able to add it.
        qCritical() << Q_FUNC_INFO << ": Failed to add node " << node->GetName()
          << " through plugin '" << selectedPlugin->name().toLatin1().constData() << "'";
        }
      }
    }
}
