/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// CTK includes
#include <ctkCheckableComboBox.h>

// qMRMLWidgets includes
#include "qMRMLCheckableNodeComboBox.h"
#include "qMRMLNodeComboBox_p.h"

// MRML includes
#include <vtkMRMLNode.h>

// -----------------------------------------------------------------------------
class qMRMLCheckableNodeComboBoxPrivate: public qMRMLNodeComboBoxPrivate
{
  Q_DECLARE_PUBLIC(qMRMLCheckableNodeComboBox);
protected:
  qMRMLCheckableNodeComboBox* const q_ptr;
  virtual void setModel(QAbstractItemModel* model);
public:
  qMRMLCheckableNodeComboBoxPrivate(qMRMLCheckableNodeComboBox& object);
  virtual void init(QAbstractItemModel* model);
};

// -----------------------------------------------------------------------------
qMRMLCheckableNodeComboBoxPrivate
::qMRMLCheckableNodeComboBoxPrivate(qMRMLCheckableNodeComboBox& object)
  : qMRMLNodeComboBoxPrivate(object)
  , q_ptr(&object)
{
}

// -----------------------------------------------------------------------------
void qMRMLCheckableNodeComboBoxPrivate::init(QAbstractItemModel* model)
{
  Q_Q(qMRMLCheckableNodeComboBox);
  this->ComboBox = new ctkCheckableComboBox;

  this->qMRMLNodeComboBoxPrivate::init(model);

  q->setAddEnabled(false);
  q->setRemoveEnabled(false);
  q->setEditEnabled(false);
  q->setRenameEnabled(false);
}

// --------------------------------------------------------------------------
void qMRMLCheckableNodeComboBoxPrivate::setModel(QAbstractItemModel* model)
{
  if (model)
    {
    qobject_cast<ctkCheckableComboBox*>(this->ComboBox)->setCheckableModel(model);
    }
  this->qMRMLNodeComboBoxPrivate::setModel(model);
}

// --------------------------------------------------------------------------
// qMRMLCheckableNodeComboBox

// --------------------------------------------------------------------------
qMRMLCheckableNodeComboBox::qMRMLCheckableNodeComboBox(QWidget* parentWidget)
  : Superclass(new qMRMLCheckableNodeComboBoxPrivate(*this), parentWidget)
{
  Q_D(qMRMLCheckableNodeComboBox);
  // Can't be done in XXXPrivate::init() because XXX is not constructed at that
  // time.
  this->connect(d->ComboBox, SIGNAL(checkedIndexesChanged()),
                this, SIGNAL(checkedNodesChanged()));
}

// --------------------------------------------------------------------------
qMRMLCheckableNodeComboBox::~qMRMLCheckableNodeComboBox()
{
}

// --------------------------------------------------------------------------
QList<vtkMRMLNode*> qMRMLCheckableNodeComboBox::checkedNodes()const
{
  Q_D(const qMRMLCheckableNodeComboBox);
  qDebug() << "qMRMLCheckableNodeComboBox::checkedNodes: ";
  QList<vtkMRMLNode*> res;
  const ctkCheckableComboBox* checkableComboBox =
    qobject_cast<const ctkCheckableComboBox*>(d->ComboBox);
  foreach(const QModelIndex& checkedIndex, checkableComboBox->checkedIndexes())
    {
    vtkMRMLNode* checkedNode = d->mrmlNodeFromIndex(checkedIndex);
    // MRMLScene or extra items could be checked, we don't want them
    qDebug() << checkedIndex << checkedNode;
    if (checkedNode)
      {
      res << checkedNode;
      }
    }
  return res;
}

// --------------------------------------------------------------------------
bool qMRMLCheckableNodeComboBox::allChecked()const
{
  Q_D(const qMRMLCheckableNodeComboBox);
  const ctkCheckableComboBox* checkableComboBox =
    qobject_cast<const ctkCheckableComboBox*>(d->ComboBox);
  return checkableComboBox->allChecked();
}

// --------------------------------------------------------------------------
bool qMRMLCheckableNodeComboBox::noneChecked()const
{
  Q_D(const qMRMLCheckableNodeComboBox);
  const ctkCheckableComboBox* checkableComboBox =
    qobject_cast<const ctkCheckableComboBox*>(d->ComboBox);
  return checkableComboBox->noneChecked();
}

// --------------------------------------------------------------------------
Qt::CheckState qMRMLCheckableNodeComboBox::checkState(vtkMRMLNode* node)const
{
  Q_D(const qMRMLCheckableNodeComboBox);
  Q_ASSERT(node);
  const ctkCheckableComboBox* checkableComboBox =
    qobject_cast<const ctkCheckableComboBox*>(d->ComboBox);
  QModelIndexList indexes = d->indexesFromMRMLNodeID(node->GetID());
  Q_ASSERT(indexes.size());
  return checkableComboBox->checkState(indexes[0]);
}

// --------------------------------------------------------------------------
void qMRMLCheckableNodeComboBox::setCheckState(vtkMRMLNode* node, Qt::CheckState check)
{
  Q_D(qMRMLCheckableNodeComboBox);
  Q_ASSERT(node);
  ctkCheckableComboBox* checkableComboBox =
    qobject_cast<ctkCheckableComboBox*>(d->ComboBox);
  QModelIndexList indexes = d->indexesFromMRMLNodeID(node->GetID());
  Q_ASSERT(indexes.size());
  return checkableComboBox->setCheckState(indexes[0], check);
}
