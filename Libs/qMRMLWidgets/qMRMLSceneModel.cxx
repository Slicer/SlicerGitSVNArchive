/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>

// qMRML includes
#include "qMRMLSceneModel_p.h"

// MRML includes
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLScene.h>

// VTK includes

// STD includes

//------------------------------------------------------------------------------
qMRMLSceneModelPrivate::qMRMLSceneModelPrivate(qMRMLSceneModel& object)
  : q_ptr(&object)
{
  this->CallBack = vtkSmartPointer<vtkCallbackCommand>::New();
  this->ListenNodeModifiedEvent = false;
  this->PendingItemModified = -1; // -1 means not updating
  
  this->NameColumn = -1;
  this->IDColumn = -1;
  this->CheckableColumn = -1;
  this->VisibilityColumn = -1;

  this->HiddenIcon = QIcon(":Icons/VisibleOff.png");
  this->VisibleIcon = QIcon(":Icons/VisibleOn.png");
  this->PartiallyVisibleIcon = QIcon(":Icons/VisiblePartially.png");

  this->MRMLScene = 0;

  qRegisterMetaType<QStandardItem* >("QStandardItem*");
}

//------------------------------------------------------------------------------
qMRMLSceneModelPrivate::~qMRMLSceneModelPrivate()
{
  if (this->MRMLScene)
    {
    this->MRMLScene->RemoveObserver(this->CallBack);
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneModelPrivate::init()
{
  Q_Q(qMRMLSceneModel);
  this->CallBack->SetClientData(q);
  this->CallBack->SetCallback(qMRMLSceneModel::onMRMLSceneEvent);
  
  this->NameColumn = qMRMLSceneModel::NameColumn;
  this->IDColumn = qMRMLSceneModel::IDColumn;
  
  q->setColumnCount(2);
  q->setHorizontalHeaderLabels(QStringList() << "Nodes" << "Ids");
  QObject::connect(q, SIGNAL(itemChanged(QStandardItem*)),
                   q, SLOT(onItemChanged(QStandardItem*)));
}

//------------------------------------------------------------------------------
void qMRMLSceneModelPrivate::listenNodeModifiedEvent()
{
  Q_Q(qMRMLSceneModel);
  q->qvtkDisconnect(0, vtkCommand::ModifiedEvent, q, SLOT(onMRMLNodeModified(vtkObject*)));
  if (!this->ListenNodeModifiedEvent)
    {
    return;
    }
  QModelIndex sceneIndex = q->mrmlSceneIndex();
  const int count = q->rowCount(sceneIndex);
  for (int i = 0; i < count; ++i)
    {
    q->qvtkConnect(q->mrmlNodeFromIndex(sceneIndex.child(i,0)),vtkCommand::ModifiedEvent,
                   q, SLOT(onMRMLNodeModified(vtkObject*)));
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneModelPrivate::insertExtraItem(int row, QStandardItem* parent,
                                             const QString& text,
                                             const QString& extraType,
                                             const Qt::ItemFlags& flags)
{
  Q_ASSERT(parent);

  QStandardItem* item = new QStandardItem;
  item->setData(extraType, qMRMLSceneModel::UIDRole);
  if (text == "separator")
    {
    item->setData("separator", Qt::AccessibleDescriptionRole);
    }
  else
    {
    item->setText(text);
    }
  item->setFlags(flags);
  QList<QStandardItem*> items;
  items << item;
  items << new QStandardItem;
  items[1]->setFlags(0);
  parent->insertRow(row, items);

  // update extra item cache info (for faster retrieval)
  QMap<QString, QVariant> extraItems = parent->data(qMRMLSceneModel::ExtraItemsRole).toMap();
  extraItems[extraType] = extraItems[extraType].toStringList() << text;
  parent->setData(extraItems, qMRMLSceneModel::ExtraItemsRole );
}


//------------------------------------------------------------------------------
QStringList qMRMLSceneModelPrivate::extraItems(QStandardItem* parent, const QString& extraType)const
{
  QStringList res;
  if (parent == 0)
    {
    //parent = q->invisibleRootItem();
    return res;
    }
  // It is expensive to search, cache the extra items.
  res = parent->data(qMRMLSceneModel::ExtraItemsRole).toMap()[extraType].toStringList();
  /*
  const int rowCount = parent->rowCount();
  for (int i = 0; i < rowCount; ++i)
    {
    QStandardItem* child = parent->child(i);
    if (child && child->data(qMRMLSceneModel::UIDRole).toString() == extraType)
      {
      if (child->data(Qt::AccessibleDescriptionRole) == "separator")
        {
        res << "separator";
        }
      else
        {
        res << child->text();
        }
      }
    }
  */
  return res;
}

//------------------------------------------------------------------------------
void qMRMLSceneModelPrivate::removeAllExtraItems(QStandardItem* parent, const QString extraType)
{
  Q_Q(qMRMLSceneModel);
  Q_ASSERT(parent);
  QMap<QString, QVariant> extraItems =
    parent->data(qMRMLSceneModel::ExtraItemsRole).toMap();
  if (extraItems[extraType].toStringList().size() == 0)
    {
    return;
    }
  QModelIndex start = parent ? parent->index().child(0,0) : QModelIndex().child(0,0);
  QModelIndexList indexes =
    q->match(start, qMRMLSceneModel::UIDRole, extraType, 1, Qt::MatchExactly);
  while (start != QModelIndex() && indexes.size())
    {
    QModelIndex parentIndex = indexes[0].parent();
    int row = indexes[0].row();
    q->removeRow(row, parentIndex);
    // don't start the whole search from scratch, only from where we ended it
    start = parentIndex.child(row,0);
    indexes = q->match(start, qMRMLSceneModel::UIDRole, extraType, 1, Qt::MatchExactly);
    }
  extraItems[extraType] = QStringList();
  parent->setData(extraItems, qMRMLSceneModel::ExtraItemsRole);
}

//------------------------------------------------------------------------------
bool qMRMLSceneModelPrivate::isExtraItem(const QStandardItem* item)const
{
  QString uid =
    item ? item->data(qMRMLSceneModel::UIDRole).toString() : QString();
  return uid == "preItem" || uid == "postItem";
}

//------------------------------------------------------------------------------
void qMRMLSceneModelPrivate::reparentItems(
  QList<QStandardItem*>& children, int newIndex, QStandardItem* newParentItem)
{
  Q_Q(qMRMLSceneModel);
  int min = q->preItems(newParentItem).count();
  int max = newParentItem->rowCount() - q->postItems(newParentItem).count();
  int pos = qMin(min + newIndex, max);
  newParentItem->insertRow(pos, children);
}

//------------------------------------------------------------------------------
// qMRMLSceneModel
//------------------------------------------------------------------------------
qMRMLSceneModel::qMRMLSceneModel(QObject *_parent)
  :QStandardItemModel(_parent)
  , d_ptr(new qMRMLSceneModelPrivate(*this))
{
  Q_D(qMRMLSceneModel);
  d->init(/*new qMRMLSceneModelItemHelperFactory*/);
}

//------------------------------------------------------------------------------
qMRMLSceneModel::qMRMLSceneModel(qMRMLSceneModelPrivate* pimpl, QObject *parentObject)
  :QStandardItemModel(parentObject)
  , d_ptr(pimpl)
{
  Q_D(qMRMLSceneModel);
  d->init(/*factory*/);
}

//------------------------------------------------------------------------------
qMRMLSceneModel::~qMRMLSceneModel()
{
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setPreItems(const QStringList& extraItems, QStandardItem* parent)
{
  Q_D(qMRMLSceneModel);

  if (parent == 0)
    {
    return;
    }

  d->removeAllExtraItems(parent, "preItem");

  int row = 0;
  foreach(QString extraItem, extraItems)
    {
    d->insertExtraItem(row++, parent, extraItem, "preItem", Qt::ItemIsEnabled  | Qt::ItemIsSelectable);
    }
}

//------------------------------------------------------------------------------
QStringList qMRMLSceneModel::preItems(QStandardItem* parent)const
{
  Q_D(const qMRMLSceneModel);
  return d->extraItems(parent, "preItem");
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setPostItems(const QStringList& extraItems, QStandardItem* parent)
{
  Q_D(qMRMLSceneModel);

  if (parent == 0)
    {
    return;
    }

  d->removeAllExtraItems(parent, "postItem");
  foreach(QString extraItem, extraItems)
    {
    d->insertExtraItem(parent->rowCount(), parent, extraItem, "postItem", Qt::ItemIsEnabled);
    }
}

//------------------------------------------------------------------------------
QStringList qMRMLSceneModel::postItems(QStandardItem* parent)const
{
  Q_D(const qMRMLSceneModel);
  return d->extraItems(parent, "postItem");
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qMRMLSceneModel);
  /// it could go wrong if you try to set the same scene (specially because
  /// while updating the scene your signals/slots might call setMRMLScene again
  if (scene == d->MRMLScene)
    {
    return;
    }

  if (d->MRMLScene)
    {
    d->MRMLScene->RemoveObserver(d->CallBack);
    }
  if (scene)
    {
    scene->AddObserver(vtkMRMLScene::NodeAboutToBeAddedEvent, d->CallBack, -10.);
    scene->AddObserver(vtkMRMLScene::NodeAddedEvent, d->CallBack, 10.);
    scene->AddObserver(vtkMRMLScene::NodeAboutToBeRemovedEvent, d->CallBack, -10.);
    scene->AddObserver(vtkMRMLScene::NodeRemovedEvent, d->CallBack, 10.);
    scene->AddObserver(vtkCommand::DeleteEvent, d->CallBack);
    scene->AddObserver(vtkMRMLScene::SceneAboutToBeClosedEvent, d->CallBack);
    scene->AddObserver(vtkMRMLScene::SceneClosedEvent, d->CallBack);
    scene->AddObserver(vtkMRMLScene::SceneAboutToBeImportedEvent, d->CallBack);
    scene->AddObserver(vtkMRMLScene::SceneImportedEvent, d->CallBack);
    }
  d->MRMLScene = scene;
  this->updateScene();
}

//------------------------------------------------------------------------------
vtkMRMLScene* qMRMLSceneModel::mrmlScene()const
{
  Q_D(const qMRMLSceneModel);
  return d->MRMLScene;
}

//------------------------------------------------------------------------------
QStandardItem* qMRMLSceneModel::mrmlSceneItem()const
{
  Q_D(const qMRMLSceneModel);
  if (d->MRMLScene == 0)
    {
    return 0;
    }
  int count = this->invisibleRootItem()->rowCount();
  for (int i = 0; i < count; ++i)
    {
    QStandardItem* child = this->invisibleRootItem()->child(i);
    QVariant uid = child->data(qMRMLSceneModel::UIDRole);
    if (uid.type() == QVariant::String &&
        uid.toString() == "scene")
      {
      return child;
      }
    }
  return 0;
}

//------------------------------------------------------------------------------
QModelIndex qMRMLSceneModel::mrmlSceneIndex()const
{
  QStandardItem* scene = this->mrmlSceneItem();
  if (scene == 0)
    {
    return QModelIndex();
    }
  return scene ? scene->index() : QModelIndex();
}

//------------------------------------------------------------------------------
vtkMRMLNode* qMRMLSceneModel::mrmlNodeFromItem(QStandardItem* nodeItem)const
{
  Q_D(const qMRMLSceneModel);
  // TODO: fasten by saving the pointer into the data
  if (d->MRMLScene == 0 || nodeItem == 0)
    {
    return 0;
    }
  QVariant nodePointer = nodeItem->data(qMRMLSceneModel::PointerRole);
  if (!nodePointer.isValid() || nodeItem->data(qMRMLSceneModel::UIDRole).toString() == "scene")
    {
    return 0;
    }
  //return nodeItem ? d->MRMLScene->GetNodeByID(
  //  nodeItem->data(qMRMLSceneModel::UIDRole).toString().toLatin1()) : 0;
  vtkMRMLNode* node = static_cast<vtkMRMLNode*>(
    reinterpret_cast<void *>(
      nodePointer.toLongLong()));
  Q_ASSERT(node);
  return node;
}
//------------------------------------------------------------------------------
QStandardItem* qMRMLSceneModel::itemFromNode(vtkMRMLNode* node, int column)const
{
  QModelIndex nodeIndex = this->indexFromNode(node, column);
  QStandardItem* nodeItem = this->itemFromIndex(nodeIndex);
  return nodeItem;
}

//------------------------------------------------------------------------------
QModelIndex qMRMLSceneModel::indexFromNode(vtkMRMLNode* node, int column)const
{
  if (node == 0)
    {
    return QModelIndex();
    }
  // QAbstractItemModel::match doesn't browse through columns
  // we need to do it manually
  QModelIndexList nodeIndexes = this->match(
    this->mrmlSceneIndex(), qMRMLSceneModel::UIDRole, QString(node->GetID()),
    1, Qt::MatchExactly | Qt::MatchRecursive);
  Q_ASSERT(nodeIndexes.size() <= 1); // we know for sure it won't be more than 1
  if (nodeIndexes.size() == 0)
    {
    // maybe the node hasn't been added to the scene yet...
    // (if it's called from populateScene/inserteNode)
    return QModelIndex();
    }
  if (column == 0)
    {
    // QAbstractItemModel::match only search through the first column
    // (because scene is in the first column)
    Q_ASSERT(nodeIndexes[0].isValid());
    return nodeIndexes[0];
    }
  // Add the QModelIndexes from the other columns
  const int row = nodeIndexes[0].row();
  QModelIndex nodeParentIndex = nodeIndexes[0].parent();
  Q_ASSERT( column < this->columnCount(nodeParentIndex) );
  return nodeParentIndex.child(row, column);
}

//------------------------------------------------------------------------------
QModelIndexList qMRMLSceneModel::indexes(vtkMRMLNode* node)const
{
  QModelIndex scene = this->mrmlSceneIndex();
  if (scene == QModelIndex())
    {
    return QModelIndexList();
    }
  // QAbstractItemModel::match doesn't browse through columns
  // we need to do it manually
  QModelIndexList nodeIndexes = this->match(
    scene, qMRMLSceneModel::UIDRole, QString(node->GetID()),
    1, Qt::MatchExactly | Qt::MatchRecursive);
  Q_ASSERT(nodeIndexes.size() <= 1); // we know for sure it won't be more than 1
  if (nodeIndexes.size() == 0)
    {
    return nodeIndexes;
    }
  // Add the QModelIndexes from the other columns
  const int row = nodeIndexes[0].row();
  QModelIndex nodeParentIndex = nodeIndexes[0].parent();
  const int sceneColumnCount = this->columnCount(nodeParentIndex);
  for (int j = 1; j < sceneColumnCount; ++j)
    {
    nodeIndexes << nodeParentIndex.child(row, j);
    }
  return nodeIndexes;
}


//------------------------------------------------------------------------------
vtkMRMLNode* qMRMLSceneModel::parentNode(vtkMRMLNode* node)const
{
  Q_UNUSED(node);
  return 0;
}

//------------------------------------------------------------------------------
int qMRMLSceneModel::nodeIndex(vtkMRMLNode* node)const
{
  Q_D(const qMRMLSceneModel);
  if (!d->MRMLScene)
    {
    return -1;
    }
  const char* nodeId = node ? node->GetID() : 0;
  if (nodeId == 0)
    {
    return -1;
    }
  const char* nId = 0;
  int index = -1;
  vtkMRMLNode* parent = this->parentNode(node);

  // otherwise, iterate through the scene
  vtkCollection* sceneCollection = d->MRMLScene->GetCurrentScene();
  vtkMRMLNode* n = 0;
  vtkCollectionSimpleIterator it;
  for (sceneCollection->InitTraversal(it);
       (n = (vtkMRMLNode*)sceneCollection->GetNextItemAsObject(it)) ;)
    {
    // note: parent can be NULL, it means that the scene is the parent
    if (parent == this->parentNode(n))
      {
      ++index;
      nId = n->GetID();
      if (nId && !strcmp(nodeId, nId))
        {
        return index;
        }
      }
    }
  return -1;
}

//------------------------------------------------------------------------------
bool qMRMLSceneModel::canBeAChild(vtkMRMLNode* node)const
{
  Q_UNUSED(node);
  return false;
}

//------------------------------------------------------------------------------
bool qMRMLSceneModel::canBeAParent(vtkMRMLNode* node)const
{
  Q_UNUSED(node);
  return false;
}

//------------------------------------------------------------------------------
bool qMRMLSceneModel::reparent(vtkMRMLNode* node, vtkMRMLNode* newParent)
{
  Q_UNUSED(node);
  Q_UNUSED(newParent);
  return false;
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setListenNodeModifiedEvent(bool listen)
{
  Q_D(qMRMLSceneModel);
  if (d->ListenNodeModifiedEvent == listen)
    {
    return;
    }
  d->ListenNodeModifiedEvent = listen;
  d->listenNodeModifiedEvent();
}

//------------------------------------------------------------------------------
bool qMRMLSceneModel::listenNodeModifiedEvent()const
{
  Q_D(const qMRMLSceneModel);
  return d->ListenNodeModifiedEvent;
}

//------------------------------------------------------------------------------
QMimeData* qMRMLSceneModel::mimeData(const QModelIndexList& indexes)const
{
  Q_D(const qMRMLSceneModel);
  if (!indexes.size())
    {
    return 0;
    }
  QModelIndex parent = indexes[0].parent();
  QModelIndexList allColumnsIndexes;
  foreach(const QModelIndex& index, indexes)
    {
    QModelIndex parent = index.parent();
    for (int column = 0; column < this->columnCount(parent); ++column)
      {
      allColumnsIndexes << this->index(index.row(), column, parent);
      }
    d->DraggedNodes << this->mrmlNodeFromIndex(index);
    }
  // Remove duplicates
  allColumnsIndexes = allColumnsIndexes.toSet().toList();
  return this->QStandardItemModel::mimeData(allColumnsIndexes);
}

//------------------------------------------------------------------------------
bool qMRMLSceneModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent)
{
  Q_D(qMRMLSceneModel);
  Q_UNUSED(column);
  // We want to do drag&drop only into the first item of a line (and not on a
  // randomn column.
  bool res = this->Superclass::dropMimeData(
    data, action, row, 0, parent.sibling(parent.row(), 0));
  d->DraggedNodes.clear();
  return res;
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::updateScene()
{
  Q_D(qMRMLSceneModel);
  // save extra items
  QStringList oldPreItems = this->preItems(0);
  QStringList oldPostItems = this->postItems(0);

  QStringList oldScenePreItems, oldScenePostItems;
  QList<QStandardItem*> oldSceneItem = this->findItems("Scene");
  if (oldSceneItem.size())
    {
    oldScenePreItems = this->preItems(oldSceneItem[0]);
    oldScenePostItems = this->postItems(oldSceneItem[0]);
    }
  // Stop listening to all the nodes before we remove them (setRowCount) as some
  // weird behavior could arise when removing the nodes (e.g onMRMLNodeModified
  // could be called ...)
  qvtkDisconnect(0, vtkCommand::ModifiedEvent,
                 this, SLOT(onMRMLNodeModified(vtkObject*)));
  // TBD: Because we don't call clear, I don't think restoring the column count
  // is necessary because it shouldn't be changed.
  int oldColumnCount = this->columnCount();

  this->setRowCount(0);
  this->invisibleRootItem()->setFlags(Qt::ItemIsEnabled);
  this->setColumnCount(oldColumnCount);

  // restore extra items
  this->setPreItems(oldPreItems, 0);
  this->setPostItems(oldPostItems, 0);
  if (d->MRMLScene == 0)
    {
    return;
    }

  // Add scene item
  QList<QStandardItem*> sceneItems;
  QStandardItem* sceneItem = new QStandardItem;
  sceneItem->setFlags(Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
  sceneItem->setText("Scene");
  sceneItem->setData("scene", qMRMLSceneModel::UIDRole);
  sceneItem->setData(QVariant::fromValue(reinterpret_cast<long long>(d->MRMLScene)), qMRMLSceneModel::PointerRole);
  sceneItems << sceneItem;
  for (int i = 1; i < this->columnCount(); ++i)
    {
    QStandardItem* sceneOtherColumn = new QStandardItem;
    sceneOtherColumn->setFlags(0);
    sceneItems << sceneOtherColumn;
    }
  this->insertRow(oldPreItems.count(), sceneItems);
  this->setPreItems(oldScenePreItems, sceneItem);
  this->setPostItems(oldScenePostItems, sceneItem);

  // Populate scene with nodes
  this->populateScene();
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::populateScene()
{
  Q_D(qMRMLSceneModel);
  Q_ASSERT(d->MRMLScene);
  // Add nodes
  vtkMRMLNode *node = 0;
  vtkCollectionSimpleIterator it;
  d->MisplacedNodes.clear();
  for (d->MRMLScene->GetCurrentScene()->InitTraversal(it);
       (node = (vtkMRMLNode*)d->MRMLScene->GetCurrentScene()->GetNextItemAsObject(it)) ;)
    {
    this->insertNode(node);
    }
  foreach(vtkMRMLNode* misplacedNode, d->MisplacedNodes)
    {
    this->onMRMLNodeModified(misplacedNode);
    }
}

//------------------------------------------------------------------------------
QStandardItem* qMRMLSceneModel::insertNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSceneModel);
  QStandardItem* nodeItem = this->itemFromNode(node);
  if (nodeItem != 0)
    {
    // It is possible that the node has been already added if it is the parent
    // of a child node already inserted.
    return nodeItem;
    }
  vtkMRMLNode* parentNode = this->parentNode(node);
  QStandardItem* parentItem =
    parentNode ? this->itemFromNode(parentNode) : this->mrmlSceneItem();
  if (!parentItem)
    {
    Q_ASSERT(parentNode);
    parentItem = this->insertNode(parentNode);
    Q_ASSERT(parentItem);
    }
  int min = this->preItems(parentItem).count();
  int max = parentItem->rowCount() - this->postItems(parentItem).count();
  int row = this->nodeIndex(node);
  if (min + row > max)
    {
    d->MisplacedNodes << node;
    row = max;
    }
  nodeItem = this->insertNode(node, parentItem, row);
  Q_ASSERT(this->itemFromNode(node) == nodeItem);
  return nodeItem;
}

//------------------------------------------------------------------------------
QStandardItem* qMRMLSceneModel::insertNode(vtkMRMLNode* node, QStandardItem* parent, int row)
{
  Q_D(qMRMLSceneModel);
  Q_ASSERT(vtkMRMLNode::SafeDownCast(node));

  if (node->GetName() && QString(node->GetName()) == "Fat")
    {
    qDebug() << "Row: " << row;
    }

  QList<QStandardItem*> items;
  for (int i= 0; i < this->columnCount(); ++i)
    {
    QStandardItem* newNodeItem = new QStandardItem();
    this->updateItemFromNode(newNodeItem, node, i);
    items.append(newNodeItem);
    }
  if (parent)
    {
    parent->insertRow(row, items);
    //Q_ASSERT(parent->columnCount() == 2);
    }
  else
    {
    this->insertRow(row,items);
    }
  // TODO: don't listen to nodes that are hidden from editors ?
  if (d->ListenNodeModifiedEvent)
    {
    qvtkConnect(node, vtkCommand::ModifiedEvent,
                this, SLOT(onMRMLNodeModified(vtkObject*)));
    }
  return items[0];
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::updateItemFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_D(qMRMLSceneModel);
  // We are going to make potentially multiple changes to the item. We want to
  // refresh the node only once, so we "block" the updates in onItemChanged().
  d->PendingItemModified = 0;
  item->setFlags(this->nodeFlags(node, column));
  // set UIDRole and set PointerRole need to be atomic
  bool blocked  = this->blockSignals(true);
  item->setData(QString(node->GetID()), qMRMLSceneModel::UIDRole);
  item->setData(QVariant::fromValue(reinterpret_cast<long long>(node)), qMRMLSceneModel::PointerRole);
  this->blockSignals(blocked);
  this->updateItemDataFromNode(item, node, column);
  
  bool itemChanged = (d->PendingItemModified > 0);
  d->PendingItemModified = -1;
  
  if (this->canBeAChild(node))
    {
    QStandardItem* parentItem = item->parent();
    QStandardItem* newParentItem = this->itemFromNode(this->parentNode(node));
    if (newParentItem == 0)
      {
      newParentItem = this->mrmlSceneItem();
      }
    // If the item has no parent, then it means it hasn't been put into the scene yet.
    // and it will do it automatically.
    if (parentItem)
      {
      int newIndex = this->nodeIndex(node);
      if (parentItem != newParentItem ||
          newIndex != item->row())
        {
        QList<QStandardItem*> children = parentItem->takeRow(item->row());
        d->reparentItems(children, newIndex, newParentItem);
        }
      }
    }
  if (itemChanged)
    {
    this->onItemChanged(item);
    }
}

//------------------------------------------------------------------------------
QFlags<Qt::ItemFlag> qMRMLSceneModel::nodeFlags(vtkMRMLNode* node, int column)const
{
  Q_D(const qMRMLSceneModel);
  QFlags<Qt::ItemFlag> flags = Qt::ItemIsEnabled
                             | Qt::ItemIsSelectable;
  if (column == d->CheckableColumn && node->GetSelectable())
    {
    flags = flags | Qt::ItemIsUserCheckable;
    }
  if (column == d->NameColumn)
    {
    flags = flags | Qt::ItemIsEditable;
    }
  if (this->canBeAChild(node))
    {
    flags = flags | Qt::ItemIsDragEnabled;
    }
  if (this->canBeAParent(node))
    {
    flags = flags | Qt::ItemIsDropEnabled;
    }

  return flags;
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::updateItemDataFromNode(
  QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_D(qMRMLSceneModel);
  if (column == d->NameColumn)
    {
    item->setText(QString(node->GetName()));
    item->setToolTip(node->GetNodeTagName());
    }
  if (column == d->ToolTipNameColumn)
    {
    item->setToolTip(QString(node->GetName()));
    }
  if (column == d->IDColumn)
    {
    item->setText(QString(node->GetID()));
    }
  if (column == d->CheckableColumn)
    {
    item->setCheckState(node->GetSelected() ? Qt::Checked : Qt::Unchecked);
    }
  if (column == d->VisibilityColumn)
    {
    vtkMRMLDisplayNode* displayNode = vtkMRMLDisplayNode::SafeDownCast(node);
    vtkMRMLDisplayableNode* displayableNode =
      vtkMRMLDisplayableNode::SafeDownCast(node);
    int visible = -1;
    if (displayNode)
      {
      visible = displayNode->GetVisibility();
      }
    else if (displayableNode)
      {
      visible = displayableNode->GetDisplayVisibility();
      }
    switch (visible)
      {
      case 0:
        // It should be fine to set the icon even if it is the same, but due
        // to a bug in Qt (http://bugreports.qt.nokia.com/browse/QTBUG-20248),
        // it would fire a superflous itemChanged() signal.
        if (item->icon().cacheKey() != d->HiddenIcon.cacheKey())
          {
          item->setIcon(d->HiddenIcon);
          }
        break;
      case 1:
        // It should be fine to set the icon even if it is the same, but due
        // to a bug in Qt (http://bugreports.qt.nokia.com/browse/QTBUG-20248),
        // it would fire a superflous itemChanged() signal.
        if (item->icon().cacheKey() != d->VisibleIcon.cacheKey())
          {
          item->setIcon(d->VisibleIcon);
          }
        break;
      case 2:
        // It should be fine to set the icon even if it is the same, but due
        // to a bug in Qt (http://bugreports.qt.nokia.com/browse/QTBUG-20248),
        // it would fire a superflous itemChanged() signal.
        if (item->icon().cacheKey() != d->PartiallyVisibleIcon.cacheKey())
          {
          item->setIcon(d->PartiallyVisibleIcon);
          }
        break;
      default:
        break;
      }
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::updateNodeFromItem(vtkMRMLNode* node, QStandardItem* item)
{
  int wasModifying = node->StartModify();
  this->updateNodeFromItemData(node, item);
  node->EndModify(wasModifying);
  
  // the following only applies to tree hierarchies
  if (!this->canBeAChild(node))
    {
    return;
    }
    
 Q_ASSERT(node != this->mrmlNodeFromItem(item->parent()));
  
  QStandardItem* parentItem = item->parent();
  
  // Don't do the following if the row is not complete (reparenting an
  // incomplete row might lead to errors). (if there is no child yet for a given
  // column, it will get there next time updateNodeFromItem is called).
  // updateNodeFromItem() is called for every item drag&dropped (we insure that
  // all the indexes of the row are reparented when entering the d&d function
  for (int i = 0; i < parentItem->columnCount(); ++i)
    {
    if (parentItem->child(item->row(), i) == 0)
      {
      return;
      }
    }

  vtkMRMLNode* parent = this->mrmlNodeFromItem(parentItem);
  int desiredNodeIndex = -1;
  if (this->parentNode(node) != parent)
    {
    this->reparent(node, parent);
    }
  else if ((desiredNodeIndex = this->nodeIndex(node)) != item->row())
    {
    QStandardItem* parentItem = item->parent();
    if (parentItem && desiredNodeIndex <
          (parentItem->rowCount() - this->postItems(parentItem).count()))
      {
      this->updateItemFromNode(item, node, item->column());
      }
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  Q_D(qMRMLSceneModel);
  if (item->column() == d->NameColumn)
    {
    node->SetName(item->text().toLatin1());
    }
  // ToolTip can't be edited, don't change the node
  // if (item->column() == d->ToolTipNameColumn)
  // {
  // }
  if (item->column() == d->IDColumn)
    {
    // Too dangerous
    //node->SetName(item->text().toLatin1());
    }
  if (item->column() == d->CheckableColumn)
    {
    node->SetSelected(item->checkState() == Qt::Checked ? 1 : 0);
    }
  if (item->column() == d->VisibilityColumn)
    {
    vtkMRMLDisplayNode* displayNode = vtkMRMLDisplayNode::SafeDownCast(node);
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(node);
    int visible = -1;
    if (item->icon().cacheKey() == d->HiddenIcon.cacheKey())
      {
      visible = 0;
      }
    else if (item->icon().cacheKey() == d->VisibleIcon.cacheKey())
      {
      visible = 1;
      }
    else if (item->icon().cacheKey() == d->PartiallyVisibleIcon.cacheKey())
      {
      visible = 2;
      }
    if (displayNode)
      {
      displayNode->SetVisibility(visible);
      }
    else if (displayableNode)
      {
      displayableNode->SetDisplayVisibility(visible);
      }
    }
}

//-----------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneEvent(vtkObject* vtk_obj, unsigned long event,
                                        void* client_data, void* call_data)
{
  vtkMRMLScene* scene = reinterpret_cast<vtkMRMLScene*>(vtk_obj);
  qMRMLSceneModel* sceneModel = reinterpret_cast<qMRMLSceneModel*>(client_data);
  vtkMRMLNode* node = reinterpret_cast<vtkMRMLNode*>(call_data);
  Q_ASSERT(scene);
  Q_ASSERT(sceneModel);
  switch(event)
    {
    case vtkMRMLScene::NodeAboutToBeAddedEvent:
      Q_ASSERT(node);
      sceneModel->onMRMLSceneNodeAboutToBeAdded(scene, node);
      break;
    case vtkMRMLScene::NodeAddedEvent:
      Q_ASSERT(node);
      sceneModel->onMRMLSceneNodeAdded(scene, node);
      break;
    case vtkMRMLScene::NodeAboutToBeRemovedEvent:
      Q_ASSERT(node);
      sceneModel->onMRMLSceneNodeAboutToBeRemoved(scene, node);
      break;
    case vtkMRMLScene::NodeRemovedEvent:
      Q_ASSERT(node);
      sceneModel->onMRMLSceneNodeRemoved(scene, node);
      break;
    case vtkCommand::DeleteEvent:
      sceneModel->onMRMLSceneDeleted(scene);
      break;
    case vtkMRMLScene::SceneAboutToBeClosedEvent:
      sceneModel->onMRMLSceneAboutToBeClosed(scene);
      break;
    case vtkMRMLScene::SceneClosedEvent:
      sceneModel->onMRMLSceneClosed(scene);
      break;
    case vtkMRMLScene::SceneAboutToBeImportedEvent:
      sceneModel->onMRMLSceneAboutToBeImported(scene);
      break;
    case vtkMRMLScene::SceneImportedEvent:
      sceneModel->onMRMLSceneImported(scene);
      break;
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneNodeAboutToBeAdded(vtkMRMLScene* scene, vtkMRMLNode* node)
{
  Q_UNUSED(scene);
  Q_UNUSED(node);
#ifndef QT_NO_DEBUG
  Q_D(qMRMLSceneModel);
  Q_ASSERT(scene != 0);
  Q_ASSERT(scene == d->MRMLScene);
#endif
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneNodeAdded(vtkMRMLScene* scene, vtkMRMLNode* node)
{
  Q_D(qMRMLSceneModel);
  Q_UNUSED(d);
  Q_UNUSED(scene);
  Q_ASSERT(scene == d->MRMLScene);
  Q_ASSERT(vtkMRMLNode::SafeDownCast(node));

  this->insertNode(node);
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneNodeAboutToBeRemoved(vtkMRMLScene* scene, vtkMRMLNode* node)
{
  Q_D(qMRMLSceneModel);
  Q_UNUSED(d);
  Q_UNUSED(scene);
  Q_ASSERT(scene == d->MRMLScene);

  int connectionsRemoved =
    qvtkDisconnect(node, vtkCommand::ModifiedEvent,
                   this, SLOT(onMRMLNodeModified(vtkObject*)));

  Q_ASSERT_X((!d->ListenNodeModifiedEvent && connectionsRemoved == 0) ||
             (d->ListenNodeModifiedEvent && connectionsRemoved == 1),
             "qMRMLSceneModel::onMRMLSceneNodeAboutToBeRemoved()",
             "A node has been removed from the scene but the scene model has "
             "never been notified it has been added in the first place. Maybe"
             " vtkMRMLScene::AddNodeNoNotify() has been used instead of "
             "vtkMRMLScene::AddNode");
  Q_UNUSED(connectionsRemoved);

  // TODO: can be fasten by browsing the tree only once
  QModelIndexList indexes = this->match(this->mrmlSceneIndex(), qMRMLSceneModel::UIDRole,
                                        QString(node->GetID()), 1,
                                        Qt::MatchExactly | Qt::MatchRecursive);
  if (indexes.count())
    {
    QStandardItem* item = this->itemFromIndex(indexes[0].sibling(indexes[0].row(),0));
    // The children may be lost if not reparented, we ensure they got reparented.
    while (item->rowCount())
      {
      // we need to remove the children from the node to remove because they 
      // would be automatically deleted in QStandardItemModel::removeRow()
      d->Orphans.push_back(item->takeRow(0));
      }
    // Remove the item from any orphan list if it exist as we don't want to
    // add it back later in onMRMLSceneNodeRemoved
    foreach(QList<QStandardItem*> orphans, d->Orphans)
      {
      if (orphans.contains(item))
        {
        d->Orphans.removeAll(orphans);
        }
      }
    this->removeRow(indexes[0].row(), indexes[0].parent());
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneNodeRemoved(vtkMRMLScene* scene, vtkMRMLNode* node)
{
  Q_D(qMRMLSceneModel);
  Q_UNUSED(scene);
  Q_UNUSED(node);
  // The removed node may had children, if they haven't been updated, they
  // are likely to be lost (not reachable when browsing the model), we need
  // to reparent them.
  foreach(QList<QStandardItem*> orphans, d->Orphans)
    {
    QStandardItem* orphan = orphans[0];
    // Make sure that the orphans have not already been reparented.
    if (orphan->parent())
      {
      // Not sure how it is possible, but if it is, then we might want to
      // review the logic behind.
      Q_ASSERT(orphan->parent() == 0);
      continue;
      }
    vtkMRMLNode* node = this->mrmlNodeFromItem(orphan);
    int newIndex = this->nodeIndex(node);
    QStandardItem* newParentItem = this->itemFromNode(this->parentNode(node));
    if (newParentItem == 0)
      {
      newParentItem = this->mrmlSceneItem();
      }
    Q_ASSERT(newParentItem);
    d->reparentItems(orphans, newIndex, newParentItem);
    }
  d->Orphans.clear();
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneDeleted(vtkObject* scene)
{
  Q_UNUSED(scene);
#ifndef QT_NO_DEBUG
  Q_D(qMRMLSceneModel);
  Q_ASSERT(scene == d->MRMLScene);
#endif
  this->setMRMLScene(0);
}

//------------------------------------------------------------------------------
void printStandardItem(QStandardItem* item, const QString& offset)
{
  if (!item)
    {
    return;
    }
  qDebug() << offset << item << item->index() << item->text()
           << item->data(qMRMLSceneModel::UIDRole).toString() << item->row()
           << item->column() << item->rowCount() << item->columnCount();
  for(int i = 0; i < item->rowCount(); ++i )
    {
    for (int j = 0; j < item->columnCount(); ++j)
      {
      printStandardItem(item->child(i,j), offset + "   ");
      }
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLNodeModified(vtkObject* node)
{
  Q_D(qMRMLSceneModel);
  vtkMRMLNode* modifiedNode = vtkMRMLNode::SafeDownCast(node);
  Q_ASSERT(modifiedNode && modifiedNode->GetScene());
  //Q_ASSERT(modifiedNode->GetScene()->IsNodePresent(modifiedNode));
  QModelIndexList nodeIndexes = this->indexes(modifiedNode);
  Q_ASSERT(nodeIndexes.count());
  //qDebug() << "onMRMLNodeModified" << modifiedNode->GetID() << nodeIndexes;
  for (int i = 0; i < nodeIndexes.size(); ++i)
    {
    QModelIndex index = nodeIndexes[i];
    // The node has been modified because it's part of a drag&drop action
    // (reparenting). so it means QStandardItemModel has already reparented
    // the row, no need to update the items again.
    //if (d->DraggedNodes.contains(modifiedNode))
    //  {
    //  continue;
    //  }
    QStandardItem* item = this->itemFromIndex(index);
    int oldRow = item->row();
    QStandardItem* oldParent = item->parent();

    this->updateItemFromNode(item, modifiedNode, item->column());
    // maybe the item has been reparented, then we need to rescan the
    // indexes again as may are wrong.
    if (item->row() != oldRow || item->parent() != oldParent)
      {
      int oldSize = nodeIndexes.size();
      nodeIndexes = this->indexes(modifiedNode);
      int newSize = nodeIndexes.size();
      //the number of columns shouldn't change
      Q_ASSERT(oldSize == newSize);
      Q_UNUSED(oldSize);
      Q_UNUSED(newSize);
      }
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onItemChanged(QStandardItem * item)
{
  Q_D(qMRMLSceneModel);

  if (d->PendingItemModified >= 0)
    {
    ++d->PendingItemModified;
    return;
    }
  // when a dnd occurs, the order of the items called with onItemChanged is
  // random, it could be the item in column 1 then the item in column 0
  //qDebug() << "onItemChanged: " << item << item->row() << item->column();
  //printStandardItem(this->mrmlSceneItem(), "");
  //return;
  // check on the column is optional(no strong feeling), it is just there to be
  // faster though
  if (!this->isANode(item))
    {
    return;
    }

  if (d->DraggedNodes.count())
    {
    this->metaObject()->invokeMethod(
      this, "onItemChanged", Qt::QueuedConnection, Q_ARG(QStandardItem*, item));
    return;
    }

  // Only nodes can be changed, scene and extra items should be not editable
  vtkMRMLNode* mrmlNode = this->mrmlNodeFromItem(item);
  Q_ASSERT(mrmlNode);
  this->updateNodeFromItem(mrmlNode, item);
}

//------------------------------------------------------------------------------
bool qMRMLSceneModel::isANode(const QStandardItem * item)const
{
  Q_D(const qMRMLSceneModel);
  return item
    && item != this->mrmlSceneItem()
    && !d->isExtraItem(item);
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneAboutToBeImported(vtkMRMLScene* scene)
{
  Q_UNUSED(scene);
  //this->beginResetModel();
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneImported(vtkMRMLScene* scene)
{
  Q_UNUSED(scene);
  //this->endResetModel();
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneAboutToBeClosed(vtkMRMLScene* scene)
{
  Q_UNUSED(scene);
  //this->beginResetModel();
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::onMRMLSceneClosed(vtkMRMLScene* scene)
{
  Q_UNUSED(scene);
  //this->endResetModel();
}

//------------------------------------------------------------------------------
Qt::DropActions qMRMLSceneModel::supportedDropActions()const
{
  return Qt::IgnoreAction;
}

//------------------------------------------------------------------------------
int qMRMLSceneModel::nameColumn()const
{
  Q_D(const qMRMLSceneModel);
  return d->NameColumn;
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setNameColumn(int column)
{
  Q_D(qMRMLSceneModel);
  d->NameColumn = column;
  /// TODO: refresh the items
}

//------------------------------------------------------------------------------
int qMRMLSceneModel::idColumn()const
{
  Q_D(const qMRMLSceneModel);
  return d->IDColumn;
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setIDColumn(int column)
{
  Q_D(qMRMLSceneModel);
  d->IDColumn = column;
  /// TODO: refresh the items
}

//------------------------------------------------------------------------------
int qMRMLSceneModel::checkableColumn()const
{
  Q_D(const qMRMLSceneModel);
  return d->CheckableColumn;
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setCheckableColumn(int column)
{
  Q_D(qMRMLSceneModel);
  d->CheckableColumn = column;
  /// TODO: refresh the items
}

//------------------------------------------------------------------------------
int qMRMLSceneModel::visibilityColumn()const
{
  Q_D(const qMRMLSceneModel);
  return d->VisibilityColumn;
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setVisibilityColumn(int column)
{
  Q_D(qMRMLSceneModel);
  d->VisibilityColumn = column;
  /// TODO: refresh the items
}

//------------------------------------------------------------------------------
int qMRMLSceneModel::toolTipNameColumn()const
{
  Q_D(const qMRMLSceneModel);
  return d->ToolTipNameColumn;
}

//------------------------------------------------------------------------------
void qMRMLSceneModel::setToolTipNameColumn(int column)
{
  Q_D(qMRMLSceneModel);
  d->ToolTipNameColumn = column;
  /// TODO: refresh the items
}
