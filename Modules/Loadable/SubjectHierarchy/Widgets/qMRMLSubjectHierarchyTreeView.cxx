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

// Qt includes
#include <QHeaderView>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QMouseEvent>

// SlicerQt includes
#include "qSlicerApplication.h"

// SubjectHierarchy includes
#include "qMRMLSubjectHierarchyTreeView.h"

#include "qMRMLSubjectHierarchyModel.h"
#include "qMRMLSortFilterSubjectHierarchyProxyModel.h"
#include "qMRMLTransformItemDelegate.h"

#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

// MRML includes
#include <vtkMRMLScene.h>

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy
class qMRMLSubjectHierarchyTreeViewPrivate : public qMRMLTreeViewPrivate
{
  Q_DECLARE_PUBLIC(qMRMLSubjectHierarchyTreeView);
public:
  typedef qMRMLTreeViewPrivate Superclass;
  qMRMLSubjectHierarchyTreeViewPrivate(qMRMLSubjectHierarchyTreeView& object);

  virtual void init();

  /// Setup all actions for tree view
  void setupActions();

  /// Save the current expansion state of child items
  void saveChildrenExpandState(QModelIndex& parentIndex);

public:
  qMRMLSubjectHierarchyModel* Model;
  qMRMLSortFilterSubjectHierarchyProxyModel* SortFilterModel;

  QMenu* NodeMenu;
  QAction* RenameAction;
  QAction* DeleteAction;
  QAction* EditAction;
  QList<QAction*> SelectPluginActions;
  QMenu* SelectPluginSubMenu;
  QActionGroup* SelectPluginActionGroup;
  QAction* ExpandToDepthAction;
  QMenu* SceneMenu;

  qMRMLTransformItemDelegate* TransformItemDelegate;

  /// Flag determining whether to highlight nodes referenced by DICOM. Storing DICOM references:
  ///   Referenced SOP instance UIDs (in attribute named vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName())
  ///   -> SH node instance UIDs (serialized string lists in subject hierarchy UID vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName())
  bool HighlightReferencedNodes;

  /// Cached list of highlighted nodes to speed up clearing highlight after new selection
  QList<vtkMRMLSubjectHierarchyNode*> HighlightedNodes;
};

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyTreeViewPrivate::qMRMLSubjectHierarchyTreeViewPrivate(qMRMLSubjectHierarchyTreeView& object)
  : qMRMLTreeViewPrivate(object)
{
  this->Model = NULL;
  this->SortFilterModel = NULL;

  this->RenameAction = NULL;
  this->DeleteAction = NULL;
  this->EditAction = NULL;
  this->ExpandToDepthAction = NULL;
  this->SelectPluginSubMenu = NULL;
  this->HighlightReferencedNodes = true;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeViewPrivate::init()
{
  Q_Q(qMRMLSubjectHierarchyTreeView);

  // Set up scene model and sort and proxy model
  this->Model = new qMRMLSubjectHierarchyModel(q);
  QObject::connect( this->Model, SIGNAL(saveTreeExpandState()), q, SLOT(saveTreeExpandState()) );
  QObject::connect( this->Model, SIGNAL(loadTreeExpandState()), q, SLOT(loadTreeExpandState()) );

  this->SortFilterModel = new qMRMLSortFilterSubjectHierarchyProxyModel(q);
  q->QTreeView::setModel(this->SortFilterModel);
  QObject::connect( q->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                    q, SLOT(onSelectionChanged(QItemSelection,QItemSelection)) );
  this->SortFilterModel->setParent(q);
  this->SortFilterModel->setSourceModel(this->Model);
  //TODO: Needed?
  // Resize the view if new rows are added/removed
  //QObject::connect( this->SortFilterModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
  //                  q, SLOT(onNumberOfVisibleIndexChanged()) );
  //QObject::connect( this->SortFilterModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
  //                  q, SLOT(onNumberOfVisibleIndexChanged()) );
  //q->onNumberOfVisibleIndexChanged();

  // Change item visibility
  q->setShowScene(true);
  //TODO: this would be desirable to set, but results in showing the scrollbar, which makes
  //      subject hierarchy much less usable (because there will be two scrollbars)
  //q->setUniformRowHeights(false);

  // Set up headers
  q->header()->setStretchLastSection(false);
  q->header()->setResizeMode(d->Model->nameColumn(), QHeaderView::Stretch);
  q->header()->setResizeMode(d->Model->visibilityColumn(), QHeaderView::ResizeToContents);
  q->header()->setResizeMode(d->Model->transformColumn(), QHeaderView::Interactive);
  q->header()->setResizeMode(d->Model->idColumn(), QHeaderView::ResizeToContents);

  // Create default menu actions
  this->NodeMenu = new QMenu(q);
  this->NodeMenu->setObjectName("nodeMenuTreeView");

  this->RenameAction = new QAction(tr("Rename"), this->NodeMenu);
  this->NodeMenu->addAction(this->RenameAction);
  QObject::connect(this->RenameAction, SIGNAL(triggered()), q, SLOT(renameCurrentNode()));

  this->DeleteAction = new QAction(tr("Delete"), this->NodeMenu);
  this->NodeMenu->addAction(this->DeleteAction);
  QObject::connect(this->DeleteAction, SIGNAL(triggered()), q, SLOT(deleteSelectedNodes()));

  this->EditAction = new QAction(tr("Edit properties..."), this->NodeMenu);
  this->NodeMenu->addAction(this->EditAction);
  QObject::connect(this->EditAction, SIGNAL(triggered()), q, SLOT(editCurrentSubjectHierarchyNode()));

  this->SceneMenu = new QMenu(q);
  this->SceneMenu->setObjectName("sceneMenuTreeView");

  // Set item delegate (that creates widgets for certain types of data)
  this->TransformItemDelegate = new qMRMLTransformItemDelegate(q);
  this->TransformItemDelegate->setFixedRowHeight(16);
  this->TransformItemDelegate->setMRMLScene(q->mrmlScene());
  q->setItemDelegateForColumn(d->Model->transformColumn(), this->TransformItemDelegate);
  QObject::connect(this->TransformItemDelegate, SIGNAL(removeTransformsFromBranchOfCurrentNode()),
    d->Model, SLOT(onRemoveTransformsFromBranchOfCurrentNode()));
  QObject::connect(this->TransformItemDelegate, SIGNAL(hardenTransformOnBranchOfCurrentNode()),
    d->Model, SLOT(onHardenTransformOnBranchOfCurrentNode()));

  // Connect invalidate filters
  QObject::connect( d->Model, SIGNAL(invalidateFilter()), d->SortFilterModel, SLOT(invalidate()) );

  // Set up scene and node actions for the tree view
  this->setupActions();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeViewPrivate::setupActions()
{
  Q_Q(qMRMLSubjectHierarchyTreeView);

  // Set up expand to level action and its menu
  this->ExpandToDepthAction = new QAction(qMRMLTreeView::tr("Expand tree to level..."), this->NodeMenu);
  this->SceneMenu->addAction(this->ExpandToDepthAction);

  QMenu* expandToDepthSubMenu = new QMenu();
  this->ExpandToDepthAction->setMenu(expandToDepthSubMenu);
  QAction* expandToDepth_1 = new QAction("1",q);
  QObject::connect(expandToDepth_1, SIGNAL(triggered()), q, SLOT(expandToDepthFromContextMenu()));
  expandToDepthSubMenu->addAction(expandToDepth_1);
  this->ExpandToDepthAction->setMenu(expandToDepthSubMenu);
  QAction* expandToDepth_2 = new QAction("2",q);
  QObject::connect(expandToDepth_2, SIGNAL(triggered()), q, SLOT(expandToDepthFromContextMenu()));
  expandToDepthSubMenu->addAction(expandToDepth_2);
  this->ExpandToDepthAction->setMenu(expandToDepthSubMenu);
  QAction* expandToDepth_3 = new QAction("3",q);
  QObject::connect(expandToDepth_3, SIGNAL(triggered()), q, SLOT(expandToDepthFromContextMenu()));
  expandToDepthSubMenu->addAction(expandToDepth_3);
  this->ExpandToDepthAction->setMenu(expandToDepthSubMenu);
  QAction* expandToDepth_4 = new QAction("4",q);
  QObject::connect(expandToDepth_4, SIGNAL(triggered()), q, SLOT(expandToDepthFromContextMenu()));
  expandToDepthSubMenu->addAction(expandToDepth_4);

  // Perform tasks needed for all plugins
  int index = 0; // Index used to insert actions before default tree actions
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
    {
    // Add node context menu actions
    foreach (QAction* action, plugin->nodeContextMenuActions())
      {
      this->NodeMenu->insertAction(this->NodeMenu->actions()[index++], action);
      }

    // Add scene context menu actions
    foreach (QAction* action, plugin->sceneContextMenuActions())
      {
      this->SceneMenu->addAction(action);
      }

    // Connect plugin events to be handled by the tree view
    QObject::connect( plugin, SIGNAL(requestExpandNode(vtkMRMLSubjectHierarchyNode*)),
      q, SLOT(expandNode(vtkMRMLSubjectHierarchyNode*)) );
    QObject::connect( plugin, SIGNAL(requestInvalidateFilter()), q->sceneModel(), SIGNAL(invalidateFilter()) );
    }

  // Create a plugin selection action for each plugin in a sub-menu
  this->SelectPluginSubMenu = this->NodeMenu->addMenu("Select role");
  this->SelectPluginActionGroup = new QActionGroup(q);
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
    {
    QAction* selectPluginAction = new QAction(plugin->name(),q);
    selectPluginAction->setCheckable(true);
    selectPluginAction->setActionGroup(this->SelectPluginActionGroup);
    selectPluginAction->setData(QVariant(plugin->name()));
    this->SelectPluginSubMenu->addAction(selectPluginAction);
    QObject::connect(selectPluginAction, SIGNAL(triggered()), q, SLOT(selectPluginForCurrentNode()));
    this->SelectPluginActions << selectPluginAction;
    }

  // Update actions in owner plugin sub-menu when opened
  QObject::connect( this->SelectPluginSubMenu, SIGNAL(aboutToShow()), q, SLOT(updateSelectPluginActions()) );
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeViewPrivate::saveChildrenExpandState(QModelIndex &parentIndex)
{
//TODO:
  //Q_Q(qMRMLTreeView);
  //vtkMRMLNode* parentNode = q->sortFilterProxyModel()->mrmlNodeFromIndex(parentIndex);

  //// Check if the node is currently present in the scene.
  //// When a node/hierarchy is being deleted from the vtkMRMLScene, there is
  //// some reference of the deleted node left dangling in the qMRMLSceneModel.
  //// As a result, mrmlNodeFromIndex returns a reference to a non-existent node.
  //// We do not need to save the tree hierarchy in such cases.
  //if (!parentNode ||
  //    !q->sortFilterProxyModel()->mrmlScene()->IsNodePresent(parentNode))
  //  {
  //  return;
  //  }

  //  if (q->isExpanded(parentIndex))
  //    {
  //    this->ExpandedNodes->AddItem(parentNode);
  //    }
  //  // Iterate over children nodes recursively to save their expansion state
  //  unsigned int numChildrenRows = q->sortFilterProxyModel()->rowCount(parentIndex);
  //  for(unsigned int row = 0; row < numChildrenRows; ++row)
  //    {
  //    QModelIndex childIndex = q->sortFilterProxyModel()->index(row, 0, parentIndex);
  //    this->saveChildrenExpandState(childIndex);
  //    }
}


//------------------------------------------------------------------------------
// qMRMLSubjectHierarchyTreeView
//------------------------------------------------------------------------------
qMRMLSubjectHierarchyTreeView::qMRMLSubjectHierarchyTreeView(QWidget *parent)
  : qMRMLTreeView(new qMRMLSubjectHierarchyTreeViewPrivate(*this), parent)
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyTreeView::~qMRMLSubjectHierarchyTreeView()
{
}

//------------------------------------------------------------------------------
vtkMRMLScene* qMRMLSubjectHierarchyTreeView::mrmlScene()const
{
  Q_D(const qMRMLSubjectHierarchyTreeView);
  return d->SceneModel ? d->SceneModel->mrmlScene() : 0;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::setMRMLScene(vtkMRMLScene* scene)
{
//TODO:
  //Q_D(qMRMLSubjectHierarchyTreeView);
  //Q_ASSERT(d->SortFilterModel);
  //vtkMRMLNode* rootNode = this->rootNode();
  //d->SceneModel->setMRMLScene(scene);
  //d->TransformItemDelegate->setMRMLScene(scene);
  //this->setRootNode(rootNode);
  //this->expandToDepth(4);
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::setShowScene(bool show)
{
//TODO:
  //Q_D(qMRMLSubjectHierarchyTreeView);
  //if (d->ShowScene == show)
  //  {
  //  return;
  //  }
  //vtkMRMLNode* oldRootNode = this->rootNode();
  //d->ShowScene = show;
  //this->setRootNode(oldRootNode);
}

//--------------------------------------------------------------------------
bool qMRMLSubjectHierarchyTreeView::showScene()const
{
  Q_D(const qMRMLSubjectHierarchyTreeView);
  return d->ShowScene;
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::setShowRootNode(bool show)
{
//TODO:
  //Q_D(qMRMLSubjectHierarchyTreeView);
  //if (d->ShowRootNode == show)
  //  {
  //  return;
  //  }
  //vtkMRMLNode* oldRootNode = this->rootNode();
  //d->ShowRootNode = show;
  //this->setRootNode(oldRootNode);
}

//--------------------------------------------------------------------------
bool qMRMLSubjectHierarchyTreeView::showRootNode()const
{
  Q_D(const qMRMLSubjectHierarchyTreeView);
  return d->ShowRootNode;
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::setRootNode(vtkMRMLNode* rootNode)
{
//TODO:
  //Q_D(qMRMLSubjectHierarchyTreeView);
  //// Need to reset the filter to be able to find indexes from nodes that
  //// could potentially be filtered out.
  //this->sortFilterProxyModel()->setHideNodesUnaffiliatedWithNodeID(QString());
  //QModelIndex treeRootIndex;
  //if (rootNode == 0)
  //  {
  //  if (!d->ShowScene)
  //    {
  //    treeRootIndex = this->sortFilterProxyModel()->mrmlSceneIndex();
  //    }
  //  }
  //else
  //  {
  //  treeRootIndex = this->sortFilterProxyModel()->indexFromMRMLNode(rootNode);
  //  if (d->ShowRootNode)
  //    {
  //    // Hide the siblings of the root node.
  //    this->sortFilterProxyModel()->setHideNodesUnaffiliatedWithNodeID(
  //      rootNode->GetID());
  //    // The parent of the root node becomes the root for QTreeView.
  //    treeRootIndex = treeRootIndex.parent();
  //    rootNode = this->sortFilterProxyModel()->mrmlNodeFromIndex(treeRootIndex);
  //    }
  //  }
  //qvtkReconnect(this->rootNode(), rootNode, vtkCommand::ModifiedEvent,
  //              this, SLOT(updateRootNode(vtkObject*)));
  //this->setRootIndex(treeRootIndex);
}

//--------------------------------------------------------------------------
vtkMRMLNode* qMRMLSubjectHierarchyTreeView::rootNode()const
{
//TODO:
  //Q_D(const qMRMLSubjectHierarchyTreeView);
  //vtkMRMLNode* treeRootNode =
  //  this->sortFilterProxyModel()->mrmlNodeFromIndex(this->rootIndex());
  //if (d->ShowRootNode &&
  //    this->mrmlScene() &&
  //    this->sortFilterProxyModel()->hideNodesUnaffiliatedWithNodeID()
  //      .isEmpty())
  //  {
  //  return this->mrmlScene()->GetNodeByID(
  //    this->sortFilterProxyModel()->hideNodesUnaffiliatedWithNodeID().toLatin1());
  //  }
  //return treeRootNode;
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::updateRootNode(vtkObject* node)
{
//TODO:
  //// Maybe the node has changed of QModelIndex, need to resync
  //this->setRootNode(vtkMRMLNode::SafeDownCast(node));
}

//--------------------------------------------------------------------------
bool qMRMLSubjectHierarchyTreeView::highlightReferencedNodes()const
{
  Q_D(const qMRMLSubjectHierarchyTreeView);
  return d->HighlightReferencedNodes;
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::setHighlightReferencedNodes(bool highlightOn)
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  d->HighlightReferencedNodes = highlightOn;
}

//------------------------------------------------------------------------------
bool qMRMLSubjectHierarchyTreeView::clickDecoration(const QModelIndex& index)
{
  //bool res = false;
  //QModelIndex sourceIndex = this->sortFilterProxyModel()->mapToSource(index);
  //if (!(sourceIndex.flags() & Qt::ItemIsEnabled))
  //  {
  //  res = false;
  //  }
  //else if (sourceIndex.column() == this->sceneModel()->visibilityColumn())
  //  {
  //  this->toggleVisibility(index);
  //  res = true;
  //  }

  //if (res)
  //  {
  //  emit decorationClicked(index);
  //  }
  //return res;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::toggleVisibility(const QModelIndex& index)
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  vtkMRMLNode* node = d->SortFilterModel->mrmlNodeFromIndex(index);
  if (!node)
    {
    return;
    }

  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);
  if (!subjectHierarchyNode)
    {
    vtkErrorWithObjectMacro(this->mrmlScene(),"toggleVisibility: Invalid node in subject hierarchy tree! Nodes must all be subject hierarchy nodes.");
    return;
    }
  qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin =
    qSlicerSubjectHierarchyPluginHandler::instance()->getOwnerPluginForSubjectHierarchyNode(subjectHierarchyNode);

  int visible = (ownerPlugin->getDisplayVisibility(subjectHierarchyNode) > 0 ? 0 : 1);
  ownerPlugin->setDisplayVisibility(subjectHierarchyNode, visible);
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::updateGeometries()
{
  // Don't update the geometries if it's not visible on screen
  // UpdateGeometries is for tree child widgets geometry
  if (!this->isVisible())
    {
    return;
    }
  this->QTreeView::updateGeometries();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::mousePressEvent(QMouseEvent* e)
{
  // Perform default mouse press event (make selections etc.)
  this->QTreeView::mousePressEvent(e);

  // Collect selected subject hierarchy nodes
  QList<vtkMRMLSubjectHierarchyNode*> selectedShNodes;
  QList<QModelIndex> selectedIndices = this->selectedIndexes();
  foreach(QModelIndex index, selectedIndices)
    {
    // Only consider the first column to avoid duplicates
    if (index.column() != 0)
      {
      continue;
      }
    vtkMRMLNode* node = this->sortFilterProxyModel()->mrmlNodeFromIndex(index);
    vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);
    if (shNode)
      {
      selectedShNodes.append(shNode);
      }
    }
  // Set current node(s) to plugin handler
  qSlicerSubjectHierarchyPluginHandler::instance()->setCurrentNodes(selectedShNodes);

  // Highlight nodes referenced by DICOM in case of single-selection
  //   Referenced SOP instance UIDs (in attribute named vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName())
  //   -> SH node instance UIDs (serialized string lists in subject hierarchy UID vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName())
  if (this->highlightReferencedNodes())
    {
    this->applyReferenceHighlightForNode(selectedShNodes);
    }

  // Not the right button clicked, handle events the default way
  if (e->button() == Qt::RightButton)
    {
    // Make sure the shown context menu is up-to-date
    this->populateContextMenuForCurrentNode();

    // Show context menu
    this->qMRMLTreeView::mousePressEvent(e);
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::mouseReleaseEvent(QMouseEvent* e)
{
//TODO:
  //if (e->button() == Qt::LeftButton)
  //  {
  //  // get the index of the current column
  //  QModelIndex index = this->indexAt(e->pos());
  //  QStyleOptionViewItemV4 opt = this->viewOptions();
  //  opt.rect = this->visualRect(index);
  //  qobject_cast<qMRMLItemDelegate*>(this->itemDelegate())->initStyleOption(&opt,index);
  //  QRect decorationElement =
  //    this->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, this);
  //  //decorationElement.translate(this->visualRect(index).topLeft());
  //  if (decorationElement.contains(e->pos()))
  //    {
  //    if (this->clickDecoration(index))
  //      {
  //      return;
  //      }
  //    }
  //  }

  //this->QTreeView::mouseReleaseEvent(e);
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
//TODO:
  //Q_UNUSED(deselected);
  //Q_D(qMRMLTreeView);
  //vtkMRMLNode* newCurrentNode = 0;
  //if (selected.indexes().count() > 0)
  //  {
  //  newCurrentNode = d->SortFilterModel->mrmlNodeFromIndex(selected.indexes()[0]);
  //  }
  //emit currentNodeChanged(newCurrentNode);
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::saveTreeExpandState()
{
//TODO:
  //Q_D(qMRMLSubjectHierarchyTreeView);
  //// Check if there is a scene loaded
  //QStandardItem* sceneItem = this->sceneModel()->mrmlSceneItem();
  //if (!sceneItem)
  //  {
  //  return;
  //  }
  //// Erase previous tree expand state
  //d->ExpandedNodes->RemoveAllItems();
  //QModelIndex sceneIndex = this->sortFilterProxyModel()->mrmlSceneIndex();

  //// First pass for the scene node
  //vtkMRMLNode* sceneNode = this->sortFilterProxyModel()->mrmlNodeFromIndex(sceneIndex);
  //if (this->isExpanded(sceneIndex))
  //  {
  //  if (sceneNode && this->sortFilterProxyModel()->mrmlScene()->IsNodePresent(sceneNode))
  //    d->ExpandedNodes->AddItem(sceneNode);
  //  }
  //unsigned int numChildrenRows = this->sortFilterProxyModel()->rowCount(sceneIndex);
  //for(unsigned int row = 0; row < numChildrenRows; ++row)
  //  {
  //  QModelIndex childIndex = this->sortFilterProxyModel()->index(row, 0, sceneIndex);
  //  d->saveChildrenExpandState(childIndex);
  //  }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::loadTreeExpandState()
{
//TODO:
  //Q_D(qMRMLSubjectHierarchyTreeView);
  //// Check if there is a scene loaded
  //QStandardItem* sceneItem = this->sceneModel()->mrmlSceneItem();
  //if (!sceneItem)
  //  {
  //  return;
  //  }
  //// Iterate over the vtkCollection of expanded nodes
  //vtkCollectionIterator* iter = d->ExpandedNodes->NewIterator();
  //for(iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  //  {
  //  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(iter->GetCurrentObject());
  //  // Check if the node is currently present in the scene.
  //  if (node && this->sortFilterProxyModel()->mrmlScene()->IsNodePresent(node))
  //    {
  //    // Expand the node
  //    QModelIndex nodeIndex = this->sortFilterProxyModel()->indexFromMRMLNode(node);
  //    this->expand(nodeIndex);
  //    }
  //  }
  //// Clear the vtkCollection now
  //d->ExpandedNodes->RemoveAllItems();
  //iter->Delete();
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::populateContextMenuForCurrentNode()
{
  Q_D(qMRMLSubjectHierarchyTreeView);

  // Get current node(s)
  QList<vtkMRMLSubjectHierarchyNode*> currentNodes = qSlicerSubjectHierarchyPluginHandler::instance()->currentNodes();
  if (currentNodes.size() > 1)
    {
    // Multi-selection: only show delete action
    d->EditAction->setVisible(false);
    d->RenameAction->setVisible(false);
    d->SelectPluginSubMenu->menuAction()->setVisible(false);

    // Hide all plugin context menu items
    foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
      {
      plugin->hideAllContextMenuActions();
      }

    return;
    }

  // Single selection
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
    {
    // Don't show certain actions for non-subject hierarchy nodes (i.e. filtering is turned off)
    d->EditAction->setVisible(false);
    d->SelectPluginSubMenu->menuAction()->setVisible(false);
    }
  else
    {
    // Show basic actions for all subject hierarchy nodes
    d->EditAction->setVisible(true);
    d->SelectPluginSubMenu->menuAction()->setVisible(true);
    }

  // Have all plugins show context menu items for current node
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
    {
    plugin->showContextMenuActionsForNode(currentNode);
    }
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::expandNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  if (node)
    {
    QModelIndex nodeIndex = d->SortFilterModel->indexFromMRMLNode(node);
    this->expand(nodeIndex);
    }
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::selectPluginForCurrentNode()
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid current node for manually selecting owner plugin!";
    return;
    }
  QString selectedPluginName = d->SelectPluginActionGroup->checkedAction()->data().toString();
  if (selectedPluginName.isEmpty())
    {
    qCritical() << Q_FUNC_INFO << ": No owner plugin found for node " << currentNode->GetName();
    return;
    }
  else if (!selectedPluginName.compare(currentNode->GetOwnerPluginName()))
    {
    // Do nothing if the owner plugin stays the same
    return;
    }

  // Check if the user is setting the plugin that would otherwise be chosen automatically
  qSlicerSubjectHierarchyAbstractPlugin* mostSuitablePluginByConfidenceNumbers =
    qSlicerSubjectHierarchyPluginHandler::instance()->findOwnerPluginForSubjectHierarchyNode(currentNode);
  bool mostSuitablePluginByConfidenceNumbersSelected =
    !mostSuitablePluginByConfidenceNumbers->name().compare(selectedPluginName);
  // Set owner plugin auto search flag to false if the user manually selected a plugin other
  // than the most suitable one by confidence numbers
  currentNode->SetOwnerPluginAutoSearch(mostSuitablePluginByConfidenceNumbersSelected);

  // Set new owner plugin
  currentNode->SetOwnerPluginName(selectedPluginName.toLatin1().constData());
  //qDebug() << Q_FUNC_INFO << ": Owner plugin of subject hierarchy node '"
  //  << currentNode->GetName() << "' has been manually changed to '" << d->SelectPluginActionGroup->checkedAction()->data().toString() << "'";
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::updateSelectPluginActions()
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid current node!";
    return;
    }
  QString ownerPluginName = QString(currentNode->GetOwnerPluginName());

  foreach (QAction* currentSelectPluginAction, d->SelectPluginActions)
    {
    // Check select plugin action if it's the owner
    bool isOwner = !(currentSelectPluginAction->data().toString().compare(ownerPluginName));

    // Get confidence numbers and show the plugins with non-zero confidence
    qSlicerSubjectHierarchyAbstractPlugin* currentPlugin =
      qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName( currentSelectPluginAction->data().toString() );
    double confidenceNumber = currentPlugin->canOwnSubjectHierarchyNode(currentNode);

    if (confidenceNumber <= 0.0 && !isOwner)
      {
      currentSelectPluginAction->setVisible(false);
      }
    else
      {
      // Set text to display for the role
      QString role = currentPlugin->roleForPlugin();
      QString currentSelectPluginActionText = QString("%1: '%2', (%3%)").arg(
        role).arg(currentPlugin->displayedNodeName(currentNode)).arg(confidenceNumber*100.0, 0, 'f', 0);
      currentSelectPluginAction->setText(currentSelectPluginActionText);
      currentSelectPluginAction->setVisible(true);
      }

    currentSelectPluginAction->setChecked(isOwner);
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::renameCurrentNode()
{
//TODO:
  //if (!this->currentNode())
  //  {
  //  Q_ASSERT(this->currentNode());
  //  return;
  //  }
  //// pop up an entry box for the new name, with the old name as default
  //QString oldName = this->currentNode()->GetName();

  //bool ok = false;
  //QString newName = QInputDialog::getText(
  //  this, "Rename " + oldName, "New name:",
  //  QLineEdit::Normal, oldName, &ok);
  //if (!ok)
  //  {
  //  return;
  //  }
  //this->currentNode()->SetName(newName.toLatin1());
  //emit currentNodeRenamed(newName);
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::editCurrentSubjectHierarchyNode()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid current node!";
    return;
    }

  qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin =
    qSlicerSubjectHierarchyPluginHandler::instance()->getOwnerPluginForSubjectHierarchyNode(currentNode);
  ownerPlugin->editProperties(currentNode);
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::deleteSelectedNodes()
{
  QList<vtkMRMLSubjectHierarchyNode*> currentNodes = qSlicerSubjectHierarchyPluginHandler::instance()->currentNodes();
  foreach(vtkMRMLSubjectHierarchyNode* node, currentNodes)
  {
    this->mrmlScene()->RemoveNode(node);
  }
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::expandToDepthFromContextMenu()
{
  QAction* senderAction = qobject_cast<QAction*>(this->sender());
  if (!senderAction)
    {
    qCritical() << Q_FUNC_INFO << ": Unable to get sender action!";
    return;
    }

  int depth = senderAction->text().toInt();
  this->expandToDepth(depth);
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::applyReferenceHighlightForNode(QList<vtkMRMLSubjectHierarchyNode*> nodes)
{
  Q_D(qMRMLSubjectHierarchyTreeView);

  // Get scene model and column to highlight
  qMRMLSubjectHierarchyModel* sceneModel = qobject_cast<qMRMLSubjectHierarchyModel*>(this->sceneModel());
  int nameColumn = sceneModel->nameColumn();

  // Clear highlight for previously highlighted nodes
  foreach(vtkMRMLSubjectHierarchyNode* highlightedNode, d->HighlightedNodes)
    {
    QStandardItem* item = sceneModel->itemFromNode(highlightedNode, nameColumn);
    if (item)
      {
      item->setBackground(Qt::transparent);
      }
    }
  d->HighlightedNodes.clear();

  // Go through all selected nodes
  foreach(vtkMRMLSubjectHierarchyNode* node, nodes)
    {
    // Get nodes referenced by argument node by DICOM
    std::vector<vtkMRMLSubjectHierarchyNode*> referencedNodes = node->GetSubjectHierarchyNodesReferencedByDICOM();

    // Highlight referenced nodes
    std::vector<vtkMRMLSubjectHierarchyNode*>::iterator nodeIt;
    for (nodeIt = referencedNodes.begin(); nodeIt != referencedNodes.end(); ++nodeIt)
      {
      vtkMRMLSubjectHierarchyNode* referencedNode = (*nodeIt);
      QStandardItem* item = sceneModel->itemFromNode(referencedNode, nameColumn);
      if (item && !d->HighlightedNodes.contains(referencedNode))
        {
        item->setBackground(Qt::yellow);
        d->HighlightedNodes.append(referencedNode);
        }
      }
    }
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::setMultiSelection(bool multiSelectionOn)
{
  if (multiSelectionOn)
    {
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
  else
    {
    this->setSelectionMode(QAbstractItemView::SingleSelection);
    }
}

//TODO: Snippet for asking whether whole branch is to be deleted
  //QMessageBox::StandardButton answer = QMessageBox::Yes;
  //if (!d->AutoDeleteSubjectHierarchyChildren)
  //  {
  //  answer =
  //    QMessageBox::question(NULL, tr("Delete subject hierarchy branch?"),
  //    tr("The deleted subject hierarchy node has children. "
  //        "Do you want to remove those too?\n\n"
  //        "If you choose yes, the whole branch will be deleted, including all children.\n"
  //        "If you choose Yes to All, this question never appears again, and all subject hierarchy children "
  //        "are automatically deleted. This can be later changed in Application Settings."),
  //    QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll,
  //    QMessageBox::No);
  //  }
  //// Delete branch if the user chose yes
  //if (answer == QMessageBox::Yes || answer == QMessageBox::YesToAll)
  //  {
  //  d->DeleteBranchInProgress = true;
  //  for (std::vector<vtkMRMLHierarchyNode*>::iterator childIt = nonVirtualChildNodes.begin();
  //    childIt != nonVirtualChildNodes.end(); ++childIt)
  //    {
  //    scene->RemoveNode(*childIt);
  //    }
  //  d->DeleteBranchInProgress = false;
  //  }
  //// Save auto-creation flag in settings
  //if (answer == QMessageBox::YesToAll)
  //  {
  //  d->AutoDeleteSubjectHierarchyChildren = true;
  //  QSettings *settings = qSlicerApplication::application()->settingsDialog()->settings();
  //  settings->setValue("SubjectHierarchy/AutoDeleteSubjectHierarchyChildren", "true");
  //  }
