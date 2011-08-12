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
#include <QApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QListView>

// CTK includes
#include <ctkComboBox.h>

// qMRMLWidgets includes
#include "qMRMLNodeComboBox_p.h"
#include "qMRMLNodeFactory.h"
#include "qMRMLSceneModel.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>

// -----------------------------------------------------------------------------
qMRMLNodeComboBoxPrivate::qMRMLNodeComboBoxPrivate(qMRMLNodeComboBox& object)
  : q_ptr(&object)
{
  this->ComboBox = 0;
  this->MRMLNodeFactory = 0;
  this->MRMLSceneModel = 0;
  this->SelectNodeUponCreation = true;
  this->NoneEnabled = false;
  this->AddEnabled = true;
  this->RemoveEnabled = true;
  this->EditEnabled = false;
  this->RenameEnabled = false;
  this->AutoDefaultText = true;
}

// -----------------------------------------------------------------------------
void qMRMLNodeComboBoxPrivate::init(QAbstractItemModel* model)
{
  Q_Q(qMRMLNodeComboBox);
  Q_ASSERT(this->MRMLNodeFactory == 0);

  q->setLayout(new QHBoxLayout);
  q->layout()->setContentsMargins(0,0,0,0);
  q->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                               QSizePolicy::Fixed,
                               QSizePolicy::ComboBox));

  if (this->ComboBox == 0)
    {
    ctkComboBox* comboBox = new ctkComboBox(q);
    comboBox->setElideMode(Qt::ElideMiddle);
    q->setComboBox(comboBox);
    }
  else
    {
    QComboBox* comboBox = this->ComboBox;
    this->ComboBox = 0;
    q->setComboBox(comboBox);
    }

  this->MRMLNodeFactory = new qMRMLNodeFactory(q);

  QAbstractItemModel* rootModel = model;
  while (qobject_cast<QAbstractProxyModel*>(rootModel) &&
         qobject_cast<QAbstractProxyModel*>(rootModel)->sourceModel())
    {
    rootModel = qobject_cast<QAbstractProxyModel*>(rootModel)->sourceModel();
    }
  this->MRMLSceneModel = qobject_cast<qMRMLSceneModel*>(rootModel);
  this->MRMLSceneModel->setListenNodeModifiedEvent(true);
  Q_ASSERT(this->MRMLSceneModel);
  // no need to reset the root model index here as the model is not yet set
  this->updateNoneItem(false);
  this->updateActionItems(false);

  qMRMLSortFilterProxyModel* sortFilterModel = new qMRMLSortFilterProxyModel(q);
  sortFilterModel->setSourceModel(model);
  this->setModel(sortFilterModel);

  // nodeTypeLabel() works only when the model is set.
  this->updateDefaultText();

  q->setEnabled(q->mrmlScene() != 0);
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBoxPrivate::setModel(QAbstractItemModel* model)
{
  Q_Q(qMRMLNodeComboBox);
  if (model == 0)
    {// it's invalid to set a null model to a combobox
    return;
    }
  if (this->ComboBox->model() != model)
    {
    this->ComboBox->setModel(model);
    }
  q->connect(model, SIGNAL(rowsInserted(const QModelIndex&, int,int)),
             q, SLOT(emitNodesAdded(const QModelIndex&, int, int)));
  q->connect(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int,int)),
             q, SLOT(emitNodesAboutToBeRemoved(const QModelIndex&, int, int)));
  q->connect(model, SIGNAL(rowsRemoved(const QModelIndex&, int,int)),
             q, SLOT(refreshIfCurrentNodeHidden()));
  q->connect(model, SIGNAL(modelReset()), q, SLOT(refreshIfCurrentNodeHidden()));
  q->connect(model, SIGNAL(layoutChanged()), q, SLOT(refreshIfCurrentNodeHidden()));
}

// --------------------------------------------------------------------------
vtkMRMLNode* qMRMLNodeComboBoxPrivate::mrmlNode(int row)const
{
  QModelIndex modelIndex;
  if (qobject_cast<QListView*>(this->ComboBox->view()))
    {
    modelIndex  = this->ComboBox->model()->index(
      row, this->ComboBox->modelColumn(), this->ComboBox->rootModelIndex());
    }
  else
    {// special case where the view can handle a tree... currentIndex could be
    // from any parent, not only a top level..
    modelIndex = this->ComboBox->view()->currentIndex();
    modelIndex = this->ComboBox->model()->index(
      row, this->ComboBox->modelColumn(), modelIndex.parent());
    }
  /*
  Q_Q(const qMRMLNodeComboBox);
  QString nodeId =
    this->ComboBox->itemData(index, qMRMLSceneModel::UIDRole).toString();
  if (nodeId.isEmpty())
    {
    return 0;
    }
  vtkMRMLScene* scene = q->mrmlScene();
  return scene ? scene->GetNodeByID(nodeId.toLatin1().data()) : 0;
  */
  return this->mrmlNodeFromIndex(modelIndex);
}

// --------------------------------------------------------------------------
vtkMRMLNode* qMRMLNodeComboBoxPrivate::mrmlNodeFromIndex(const QModelIndex& index)const
{
  Q_Q(const qMRMLNodeComboBox);
  Q_ASSERT(q->model());
  QString nodeId =
    this->ComboBox->model()->data(index, qMRMLSceneModel::UIDRole).toString();
  if (nodeId.isEmpty())
    {
    return 0;
    }
  vtkMRMLScene* scene = q->mrmlScene();
  return scene ? scene->GetNodeByID(nodeId.toLatin1().data()) : 0;
}

// --------------------------------------------------------------------------
QModelIndexList qMRMLNodeComboBoxPrivate::indexesFromMRMLNodeID(const QString& nodeID)const
{
  return this->ComboBox->model()->match(
    this->ComboBox->model()->index(0, 0), qMRMLSceneModel::UIDRole, nodeID, 1,
    Qt::MatchRecursive | Qt::MatchExactly | Qt::MatchWrap);
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBoxPrivate::updateDefaultText()
{
  if (!this->AutoDefaultText)
    {
    return;
    }
  ctkComboBox* cb = qobject_cast<ctkComboBox*>(this->ComboBox);
  if (cb)
    {
    cb->setDefaultText(QObject::tr("Select a ") + this->nodeTypeLabel());
    }
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBoxPrivate::updateNoneItem(bool resetRootIndex)
{
  Q_UNUSED(resetRootIndex);
  //Q_Q(qMRMLNodeComboBox);
  QStringList noneItem;
  if (this->NoneEnabled)
    {
    noneItem.append("None");
    }
  //QVariant currentNode =
  //  this->ComboBox->itemData(this->ComboBox->currentIndex(), qMRMLSceneModel::UIDRole);
  //qDebug() << "updateNoneItem: " << this->MRMLSceneModel->mrmlSceneItem();
  if (this->MRMLSceneModel->mrmlSceneItem())
    {
    this->MRMLSceneModel->setPreItems(noneItem, this->MRMLSceneModel->mrmlSceneItem());
    }
/*  if (resetRootIndex)
    {
    this->ComboBox->setRootModelIndex(q->model()->index(0, 0));
    // setting the rootmodel index looses the current item
    // try to set the current item back
    q->setCurrentNode(currentNode.toString());
    }
*/
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBoxPrivate::updateActionItems(bool resetRootIndex)
{
  Q_Q(qMRMLNodeComboBox);
  Q_UNUSED(resetRootIndex);

  QVariant currentNode =
    this->ComboBox->itemData(this->ComboBox->currentIndex(), qMRMLSceneModel::UIDRole);

  QStringList extraItems;
  if (q->mrmlScene())
    {
    if (this->AddEnabled || this->RemoveEnabled || this->EditEnabled || this->RenameEnabled)
      {
      extraItems.append("separator");
      }
    if (this->RenameEnabled)
      {
      extraItems.append(QObject::tr("Rename current ")  + this->nodeTypeLabel());
      }
    if (this->EditEnabled)
      {
      extraItems.append(QObject::tr("Edit current ")  + this->nodeTypeLabel());
      }
    if (this->AddEnabled)
      {
      extraItems.append(QObject::tr("Create new ") + this->nodeTypeLabel());
      }
    if (this->RemoveEnabled)
      {
      extraItems.append(QObject::tr("Delete current ")  + this->nodeTypeLabel());
      }
    }
  this->MRMLSceneModel->setPostItems(extraItems, this->MRMLSceneModel->mrmlSceneItem());
  QObject::connect(this->ComboBox->view(), SIGNAL(clicked(const QModelIndex& )),
                   q, SLOT(activateExtraItem(const QModelIndex& )),
                   Qt::UniqueConnection);
  /*
  if (resetRootIndex)
    {
    this->ComboBox->setRootModelIndex(q->model()->index(0, 0));
    // setting the rootmodel index looses the current item
    // try to set the current item back
    q->setCurrentNode(currentNode.toString());
    }
  */
}

// --------------------------------------------------------------------------
QString qMRMLNodeComboBoxPrivate::nodeTypeLabel()const
{
  Q_Q(const qMRMLNodeComboBox);
  QStringList nodeTypes = q->nodeTypes();
  QString label;
  if (q->mrmlScene() && !nodeTypes.isEmpty())
    {
    label = q->mrmlScene()->GetTagByClassName(nodeTypes[0].toLatin1());
    if (label.isEmpty() && nodeTypes[0] == "vtkMRMLVolumeNode")
      {
      label = QObject::tr("Volume");
      }
    }
  if (label.isEmpty())
    {
    label = QObject::tr("Node");
    }
  return label;
}

// --------------------------------------------------------------------------
// qMRMLNodeComboBox

// --------------------------------------------------------------------------
qMRMLNodeComboBox::qMRMLNodeComboBox(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new qMRMLNodeComboBoxPrivate(*this))
{
  Q_D(qMRMLNodeComboBox);
  d->init(new qMRMLSceneModel(this));
}

// --------------------------------------------------------------------------
qMRMLNodeComboBox::qMRMLNodeComboBox(QAbstractItemModel* sceneModel, QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new qMRMLNodeComboBoxPrivate(*this))
{
  Q_D(qMRMLNodeComboBox);
  d->init(sceneModel);
}

// --------------------------------------------------------------------------
qMRMLNodeComboBox::qMRMLNodeComboBox(qMRMLNodeComboBoxPrivate* pimpl, QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(pimpl)
{
  Q_D(qMRMLNodeComboBox);
  d->init(new qMRMLSceneModel(this));
}

// --------------------------------------------------------------------------
qMRMLNodeComboBox::~qMRMLNodeComboBox()
{
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::activateExtraItem(const QModelIndex& index)
{
  Q_D(qMRMLNodeComboBox);
  // FIXME: check the type of the item on a different role instead of the display role
  QString data = this->model()->data(index, Qt::DisplayRole).toString();
  if (data.startsWith(QObject::tr("Create new ")))
    {
    d->ComboBox->hidePopup();
    this->addNode();
    }
  else if (data.startsWith(QObject::tr("Delete current ")))
    {
    d->ComboBox->hidePopup();
    this->removeCurrentNode();
    }
  else if (data.startsWith(QObject::tr("Edit current ")))
    {
    d->ComboBox->hidePopup();
    this->editCurrentNode();
    }
  else if (data.startsWith(QObject::tr("Rename current ")))
    {
    d->ComboBox->hidePopup();
    this->renameCurrentNode();
    }
}

//-----------------------------------------------------------------------------
void qMRMLNodeComboBox::addAttribute(const QString& nodeType,
                                     const QString& attributeName,
                                     const QVariant& attributeValue)
{
  Q_D(qMRMLNodeComboBox);
  d->MRMLNodeFactory->addAttribute(attributeName, attributeValue.toString());
  this->sortFilterProxyModel()->addAttribute(nodeType, attributeName, attributeValue);
}

//-----------------------------------------------------------------------------
void qMRMLNodeComboBox::setBaseName(const QString& baseName)
{
  Q_D(qMRMLNodeComboBox);
  QStringList nodeClasses = this->nodeTypes();
  if (nodeClasses.isEmpty())
    {
    return;
    }
  d->MRMLNodeFactory->setBaseName(nodeClasses[0], baseName);
}

//-----------------------------------------------------------------------------
QString qMRMLNodeComboBox::baseName()const
{
  Q_D(const qMRMLNodeComboBox);
  QStringList nodeClasses = this->nodeTypes();
  if (nodeClasses.isEmpty())
    {
    return QString();
    }
  return d->MRMLNodeFactory->baseName(nodeClasses[0]);
}

// --------------------------------------------------------------------------
vtkMRMLNode* qMRMLNodeComboBox::addNode()
{
  Q_D(qMRMLNodeComboBox);
  // Create the MRML node via the MRML Scene
  // FIXME, for the moment we create only nodes of the first type, but we should
  // be able to add a node of any type in NodeTypes
  vtkMRMLNode * newNode =
    d->MRMLNodeFactory->createNode(this->nodeTypes()[0]);
  // The created node is appended at the bottom of the current list
  Q_ASSERT(newNode);
  if (newNode && this->selectNodeUponCreation())
    {// select the created node.
    this->setCurrentNode(newNode);
    }
  emit this->nodeAddedByUser(newNode);
  return newNode;
}

// --------------------------------------------------------------------------
vtkMRMLNode* qMRMLNodeComboBox::currentNode()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->mrmlNode(d->ComboBox->currentIndex());
}

// --------------------------------------------------------------------------
QString qMRMLNodeComboBox::currentNodeId()const
{
  vtkMRMLNode* node = this->currentNode();
  return node ? node->GetID() : "";
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::editCurrentNode()
{
  //Q_D(const qMRMLNodeComboBox);
  //FIXME
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::renameCurrentNode()
{
  Q_D(qMRMLNodeComboBox);
  vtkMRMLNode* node = this->currentNode();
  if (!node)
    {
    return;
    }
  bool ok = false;
  QString newName = QInputDialog::getText(
    this, "Rename " + d->nodeTypeLabel(), "New name:",
    QLineEdit::Normal, node->GetName(), &ok);
  if (!ok)
    {
    return;
    }
  node->SetName(newName.toLatin1());
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::emitCurrentNodeChanged(int currentIndex)
{
  Q_D(qMRMLNodeComboBox);
  vtkMRMLNode*  node = d->mrmlNode(currentIndex);
  if (!node && ((!d->NoneEnabled &&currentIndex != -1) || (d->NoneEnabled && currentIndex != 0)) )
    {
    this->setCurrentNode(this->nodeFromIndex(this->nodeCount()-1));
    }
  else
    {
    emit currentNodeChanged(node);
    emit currentNodeChanged(node != 0);
    }
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::emitNodeActivated(int currentIndex)
{
  Q_D(qMRMLNodeComboBox);
  vtkMRMLNode*  node = d->mrmlNode(currentIndex);
  // Fire only if the user clicked on a node or "None", don't fire the signal
  // if the user clicked on an "action" (post item) like "Add Node".
  if (node || (d->NoneEnabled && currentIndex == 0))
    {
    emit nodeActivated(node);
    }
}
// --------------------------------------------------------------------------
vtkMRMLScene* qMRMLNodeComboBox::mrmlScene()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->MRMLSceneModel->mrmlScene();
}

// --------------------------------------------------------------------------
int qMRMLNodeComboBox::nodeCount()const
{
  Q_D(const qMRMLNodeComboBox);
  int extraItemsCount =
    d->MRMLSceneModel->preItems(d->MRMLSceneModel->mrmlSceneItem()).count()
    + d->MRMLSceneModel->postItems(d->MRMLSceneModel->mrmlSceneItem()).count();
  //qDebug() << d->MRMLSceneModel->invisibleRootItem() << d->MRMLSceneModel->mrmlSceneItem() << d->ComboBox->count() <<extraItemsCount;
  //printStandardItem(d->MRMLSceneModel->invisibleRootItem(), "  ");
  //qDebug() << d->ComboBox->rootModelIndex();
  return this->mrmlScene() ? d->ComboBox->count() - extraItemsCount : 0;
}

// --------------------------------------------------------------------------
vtkMRMLNode* qMRMLNodeComboBox::nodeFromIndex(int index)const
{
  Q_D(const qMRMLNodeComboBox);
  return d->mrmlNode(d->NoneEnabled ? index + 1 : index);
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::removeCurrentNode()
{
  this->mrmlScene()->RemoveNode(this->currentNode());
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qMRMLNodeComboBox);

  // Be careful when commenting that out. you really need a good reason for
  // forcing a new set. You should probably expose 
  // qMRMLSceneModel::UpdateScene() and make sure there is no nested calls
  if (d->MRMLSceneModel->mrmlScene() == scene)
    {
    return ;
    }

  // The Add button is valid only if the scene is non-empty
  //this->setAddEnabled(scene != 0);
  QString oldCurrentNode = d->ComboBox->itemData(d->ComboBox->currentIndex(), qMRMLSceneModel::UIDRole).toString();
  bool oldNodeCount = this->nodeCount();

  // Update factory
  d->MRMLNodeFactory->setMRMLScene(scene);
  d->MRMLSceneModel->setMRMLScene(scene);
  d->updateDefaultText();
  d->updateNoneItem(false);
  d->updateActionItems(false);

  //qDebug()<< "setMRMLScene:" << this->model()->index(0, 0);
  // updating the action items reset the root model index. Set it back
  // setting the rootmodel index looses the current item
  d->ComboBox->setRootModelIndex(this->model()->index(0, 0));

  // try to set the current item back
  // if there was no node in the scene (or scene not set), then the
  // oldCurrentNode was not meaningful and we probably don't want to
  // set it back. Please consider make it a behavior property if it doesn't fit
  // your need, as this behavior is currently wanted for some cases (
  // vtkMRMLClipModels selector in the Models module)
  if (oldNodeCount)
    {
    this->setCurrentNode(oldCurrentNode);
    }
  // if the new nodeCount is 0, then let's make sure to select 'invalid' node
  // (None(0) or -1). we can't do nothing otherwise the Scene index (rootmodelIndex)
  // would be selected and "Scene" would be displayed (see vtkMRMLNodeComboboxTest5)
  else
    {
    this->setCurrentNode(this->currentNode());
    }

  this->setEnabled(scene != 0);
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::setCurrentNode(vtkMRMLNode* newCurrentNode)
{
  this->setCurrentNode(newCurrentNode ? newCurrentNode->GetID() : "");
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::setCurrentNode(const QString& nodeID)
{
  Q_D(qMRMLNodeComboBox);
  // A straight forward implementation of setCurrentNode would be:
  //    int index = !nodeID.isEmpty() ? d->ComboBox->findData(nodeID, qMRMLSceneModel::UIDRole) : -1;
  //    if (index == -1 && d->NoneEnabled)
  //      {
  //      index = 0;
  //      }
  //    d->ComboBox->setCurrentIndex(index);
  // However it doesn't work for custom comboxboxes that display non-flat lists
  // (typically if it is a tree model/view)
  // let's use a more generic one
  QModelIndexList indexes = d->indexesFromMRMLNodeID(nodeID);
  if (indexes.size() == 0)
    {
    QModelIndex sceneIndex = d->ComboBox->model()->index(0, 0);
    d->ComboBox->setRootModelIndex(sceneIndex);
    // The combobox updates the current index of the view only when he needs
    // it (in popup()), however we want the view to be always synchronized
    // with the currentIndex as we use it to know if it has changed. This is 
    // why we set it here.
    QModelIndex noneIndex = sceneIndex.child(0, d->ComboBox->modelColumn());
    d->ComboBox->view()->setCurrentIndex(
      d->NoneEnabled ? noneIndex : sceneIndex);
    d->ComboBox->setCurrentIndex(d->NoneEnabled ? 0 : -1);
    return;
    }
  //d->ComboBox->setRootModelIndex(indexes[0].parent());
  //d->ComboBox->setCurrentIndex(indexes[0].row());
  QModelIndex oldIndex = d->ComboBox->view()->currentIndex();
  if (oldIndex != indexes[0])
    {
    d->ComboBox->view()->setCurrentIndex(indexes[0]);
    QKeyEvent event(QEvent::ShortcutOverride, Qt::Key_Enter, Qt::NoModifier);
    // here we conditionally send the event, otherwise, nodeActivated would be
    // fired even if the user didn't manually select the node.
    // Warning: please note that sending a KeyEvent will close the popup menu
    // of the combobox if it is open.
    QApplication::sendEvent(d->ComboBox->view(), &event);
    }
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::setCurrentNodeIndex(int index)
{
  Q_D(qMRMLNodeComboBox);
  if (index >= this->nodeCount())
    {
    index = -1;
    }
  if (d->NoneEnabled)
    {
    // If the "None" extra item is present, shift all the indexes
    ++index;
    }
  d->ComboBox->setCurrentIndex(index);
}

//--------------------------------------------------------------------------
CTK_SET_CPP(qMRMLNodeComboBox, bool, setSelectNodeUponCreation, SelectNodeUponCreation);
CTK_GET_CPP(qMRMLNodeComboBox, bool, selectNodeUponCreation, SelectNodeUponCreation);

// --------------------------------------------------------------------------
QStringList qMRMLNodeComboBox::nodeTypes()const
{
  qMRMLSortFilterProxyModel* m = this->sortFilterProxyModel();
  return m ? m->nodeTypes() : QStringList();
}

// --------------------------------------------------------------------------
void qMRMLNodeComboBox::setNodeTypes(const QStringList& _nodeTypes)
{
  Q_D(qMRMLNodeComboBox);
  this->sortFilterProxyModel()->setNodeTypes(_nodeTypes);
  d->updateDefaultText();
  d->updateActionItems();
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::setNoneEnabled(bool enable)
{
  Q_D(qMRMLNodeComboBox);
  if (d->NoneEnabled == enable)
    {
    return;
    }
  d->NoneEnabled = enable;
  d->updateNoneItem();
}

//--------------------------------------------------------------------------
bool qMRMLNodeComboBox::noneEnabled()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->NoneEnabled;
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::setAddEnabled(bool enable)
{
  Q_D(qMRMLNodeComboBox);
  if (d->AddEnabled == enable)
    {
    return;
    }
  d->AddEnabled = enable;
  d->updateActionItems();
}

//--------------------------------------------------------------------------
bool qMRMLNodeComboBox::addEnabled()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->AddEnabled;
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::setRemoveEnabled(bool enable)
{
  Q_D(qMRMLNodeComboBox);
  if (d->RemoveEnabled == enable)
    {
    return;
    }
  d->RemoveEnabled = enable;
  d->updateActionItems();
}

//--------------------------------------------------------------------------
bool qMRMLNodeComboBox::removeEnabled()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->RemoveEnabled;
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::setEditEnabled(bool enable)
{
  Q_D(qMRMLNodeComboBox);
  if (d->EditEnabled == enable)
    {
    return;
    }
  d->EditEnabled = enable;
  d->updateActionItems();
}

//--------------------------------------------------------------------------
bool qMRMLNodeComboBox::editEnabled()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->EditEnabled;
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::setRenameEnabled(bool enable)
{
  Q_D(qMRMLNodeComboBox);
  if (d->RenameEnabled == enable)
    {
    return;
    }
  d->RenameEnabled = enable;
  d->updateActionItems();
}

//--------------------------------------------------------------------------
bool qMRMLNodeComboBox::renameEnabled()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->RenameEnabled;
}

//--------------------------------------------------------------------------
qMRMLSortFilterProxyModel* qMRMLNodeComboBox::sortFilterProxyModel()const
{
  Q_ASSERT(qobject_cast<qMRMLSortFilterProxyModel*>(this->model()));
  return qobject_cast<qMRMLSortFilterProxyModel*>(this->model());
}

//--------------------------------------------------------------------------
QAbstractItemModel* qMRMLNodeComboBox::model()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->ComboBox ? d->ComboBox->model() : 0;
}

//--------------------------------------------------------------------------
QAbstractItemModel* qMRMLNodeComboBox::rootModel()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->MRMLSceneModel;
}

//--------------------------------------------------------------------------
qMRMLNodeFactory* qMRMLNodeComboBox::nodeFactory()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->MRMLNodeFactory;
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::setComboBox(QComboBox* comboBox)
{
  Q_D(qMRMLNodeComboBox);
  if (comboBox == d->ComboBox)
    {
    return;
    }

  QAbstractItemModel* oldModel = this->model();
  QComboBox* oldComboBox = d->ComboBox;

  this->layout()->addWidget(comboBox);
  d->ComboBox = comboBox;
  d->setModel(oldModel);

  connect(d->ComboBox, SIGNAL(currentIndexChanged(int)),
          this, SLOT(emitCurrentNodeChanged(int)));
  connect(d->ComboBox, SIGNAL(activated(int)),
          this, SLOT(emitNodeActivated(int)));
  d->ComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                         QSizePolicy::Expanding,
                                         QSizePolicy::DefaultType));
  delete oldComboBox;
}

//--------------------------------------------------------------------------
QComboBox* qMRMLNodeComboBox::comboBox()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->ComboBox;
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::emitNodesAdded(const QModelIndex & parent, int start, int end)
{
  Q_D(qMRMLNodeComboBox);
  Q_ASSERT(this->model());
  for(int i = start; i <= end; ++i)
    {
    vtkMRMLNode* node = d->mrmlNodeFromIndex(this->model()->index(start, 0, parent));
    if (node)
      {
      emit nodeAdded(node);
      }
    }
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::emitNodesAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{
  Q_D(qMRMLNodeComboBox);
  Q_ASSERT(this->model());
  for(int i = start; i <= end; ++i)
    {
    vtkMRMLNode* node = d->mrmlNodeFromIndex(this->model()->index(start, 0, parent));
    if (node)
      {
      emit nodeAboutToBeRemoved(node);
      }
    }
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::refreshIfCurrentNodeHidden()
{
  /// Sometimes, a node can disappear/hide from the combobox
  /// (qMRMLSortFilterProxyModel) because of a changed property.
  /// If the node is the current node, we need to unselect it because it is
  /// not a valid current node anymore.
  vtkMRMLNode* node = this->currentNode();
  if (!node)
    {
    this->setCurrentNode(0);
    }
}

//--------------------------------------------------------------------------
QComboBox::SizeAdjustPolicy qMRMLNodeComboBox::sizeAdjustPolicy()const
{
  Q_D(const qMRMLNodeComboBox);
  return d->ComboBox->sizeAdjustPolicy();
}

//--------------------------------------------------------------------------
void qMRMLNodeComboBox::setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy policy)
{
  Q_D(qMRMLNodeComboBox);
  d->ComboBox->setSizeAdjustPolicy(policy);
}
