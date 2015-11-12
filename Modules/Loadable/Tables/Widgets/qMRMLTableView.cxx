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

// QT includes
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QSortFilterProxyModel>

// qMRML includes
#include "qMRMLTableView.h"
#include "qMRMLTableModel.h"

// MRML includes
#include <vtkMRMLTableNode.h>

#define VERIFY_TABLE_MODEL_AND_NODE(methodName, returnValue) \
  if (!this->tableModel()) \
    { \
    qWarning() << "qMRMLTableView:: " << #methodName << " failed: invalid model"; \
    return returnValue; \
    } \
  if (!this->mrmlTableNode()) \
    { \
    qWarning() << "qMRMLTableView::" << #methodName << " failed: invalid node"; \
    return returnValue; \
    }

//------------------------------------------------------------------------------
class qMRMLTableViewPrivate
{
  Q_DECLARE_PUBLIC(qMRMLTableView);
protected:
  qMRMLTableView* const q_ptr;
public:
  qMRMLTableViewPrivate(qMRMLTableView& object);
  void init();
};

//------------------------------------------------------------------------------
qMRMLTableViewPrivate::qMRMLTableViewPrivate(qMRMLTableView& object)
  : q_ptr(&object)
{
}

//------------------------------------------------------------------------------
void qMRMLTableViewPrivate::init()
{
  Q_Q(qMRMLTableView);

  qMRMLTableModel* tableModel = new qMRMLTableModel(q);
  QSortFilterProxyModel* sortFilterModel = new QSortFilterProxyModel(q);
  sortFilterModel->setSourceModel(tableModel);
  q->setModel(sortFilterModel);

  q->horizontalHeader()->setStretchLastSection(false);
}

//------------------------------------------------------------------------------
qMRMLTableView::qMRMLTableView(QWidget *_parent)
  : QTableView(_parent)
  , d_ptr(new qMRMLTableViewPrivate(*this))
{
  Q_D(qMRMLTableView);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLTableView::~qMRMLTableView()
{
}

//------------------------------------------------------------------------------
qMRMLTableModel* qMRMLTableView::tableModel()const
{
  return qobject_cast<qMRMLTableModel*>(this->sortFilterProxyModel()->sourceModel());
}

//------------------------------------------------------------------------------
QSortFilterProxyModel* qMRMLTableView::sortFilterProxyModel()const
{
  return qobject_cast<QSortFilterProxyModel*>(this->model());
}

//------------------------------------------------------------------------------
void qMRMLTableView::setMRMLTableNode(vtkMRMLNode* node)
{
  this->setMRMLTableNode(vtkMRMLTableNode::SafeDownCast(node));
}

//------------------------------------------------------------------------------
void qMRMLTableView::setMRMLTableNode(vtkMRMLTableNode* node)
{
  qMRMLTableModel* mrmlModel = this->tableModel();
  if (!mrmlModel)
    {
    qCritical("qMRMLTableView::setMRMLTableNode failed: invalid model");
    return;
    }

  mrmlModel->setMRMLTableNode(node);
  this->sortFilterProxyModel()->invalidate();

  this->horizontalHeader()->setMinimumSectionSize(60);
  this->resizeColumnsToContents();
}

//------------------------------------------------------------------------------
vtkMRMLTableNode* qMRMLTableView::mrmlTableNode()const
{
  qMRMLTableModel* mrmlModel = this->tableModel();
  if (!mrmlModel)
    {
    qCritical("qMRMLTableView::mrmlTableNode failed: model is invalid");
    return 0;
    }
  return mrmlModel->mrmlTableNode();
}

//------------------------------------------------------------------------------
void qMRMLTableView::setTransposed(bool transposed)
{
  VERIFY_TABLE_MODEL_AND_NODE(setTransposed);
  tableModel()->setTransposed(transposed);
}

//------------------------------------------------------------------------------
bool qMRMLTableView::transposed()const
{
  VERIFY_TABLE_MODEL_AND_NODE(transposed, false);
  return tableModel()->transposed();
}

//------------------------------------------------------------------------------
void qMRMLTableView::keyPressEvent(QKeyEvent *event)
{
  // Prevent giving the focus to the previous/next widget if arrow keys are used
  // at the edge of the table (without this: if the current cell is in the top
  // row and user press the Up key, the focus goes from the table to the previous
  // widget in the tab order)
  if (model() && (
    event->key() == Qt::Key_Left && currentIndex().column() == 0
    || event->key() == Qt::Key_Up && currentIndex().row() == 0
    || event->key() == Qt::Key_Right && currentIndex().column() == model()->columnCount()-1
    || event->key() == Qt::Key_Down && currentIndex().row() == model()->rowCount()-1 ))
    {
    return;
    }
  QTableView::keyPressEvent(event);
}

//-----------------------------------------------------------------------------
void qMRMLTableView::copySelection()
{
  Q_D(qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(copySelection);

  if (!selectionModel()->hasSelection())
    {
    return;
    }

  qMRMLTableModel* mrmlModel = tableModel();
  QItemSelectionModel* selection = selectionModel();
  QString textToCopy;
  bool firstLine = true;
  for (int rowIndex=0; rowIndex<mrmlModel->rowCount(); rowIndex++)
    {
    if (!selection->rowIntersectsSelection(rowIndex, QModelIndex()))
      {
      // no items are selected in this entire row, skip it
      continue;
      }
    if (firstLine)
      {
      firstLine = false;
      }
    else
      {
      textToCopy.append('\n');
      }
    bool firstItemInLine = true;
    for (int columnIndex=0; columnIndex<mrmlModel->columnCount(); columnIndex++)
      {
      if (!selection->columnIntersectsSelection(columnIndex, QModelIndex()))
        {
        // no items are selected in this entire column, skip it
        continue;
        }
      if (firstItemInLine)
        {
        firstItemInLine = false;
        }
      else
        {
        textToCopy.append('\t');
        }
      textToCopy.append(mrmlModel->item(rowIndex,columnIndex)->text());
      }
    }

  QApplication::clipboard()->setText(textToCopy);
}

//-----------------------------------------------------------------------------
void qMRMLTableView::pasteSelection()
{
  Q_D(qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(pasteSelection);

  QString text = QApplication::clipboard()->text();
  if (text.isEmpty())
    {
    return;
    }

  qMRMLTableModel* mrmlModel = tableModel();
  int rowIndex = currentIndex().row();
  int startColumnIndex = currentIndex().column();
  QStringList lines = text.split('\n');
  foreach(QString line, lines)
    {
    if (rowIndex>=mrmlModel->rowCount())
        {
        // reached last row in the table, ignore subsequent rows
        break;
        }
    int columnIndex = startColumnIndex;
    QStringList cells = line.split('\t');
    foreach(QString cell, cells)
      {
      if (columnIndex>=mrmlModel->columnCount())
        {
        // reached last column in the table, ignore subsequent columns
        break;
        }
      mrmlModel->item(rowIndex,columnIndex)->setText(cell);
      columnIndex++;
      }
    rowIndex++;
  }
}

//-----------------------------------------------------------------------------
void qMRMLTableView::insertColumn()
{
  Q_D(qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(insertColumn);
  if (tableModel()->transposed())
    {
    mrmlTableNode()->AddEmptyRow();
    }
  else
    {
    mrmlTableNode()->AddColumn();
    }
}

//-----------------------------------------------------------------------------
void qMRMLTableView::deleteColumn()
{
  Q_D(qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(deleteColumn);
  tableModel()->removeSelectionFromMRML(selectionModel()->selectedIndexes(), false);
}

//-----------------------------------------------------------------------------
void qMRMLTableView::insertRow()
{
  Q_D(qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(insertRow);
  if (tableModel()->transposed())
    {
    mrmlTableNode()->AddColumn();
    }
  else
    {
    mrmlTableNode()->AddEmptyRow();
    }
}

//-----------------------------------------------------------------------------
void qMRMLTableView::deleteRow()
{
  Q_D(qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(deleteRow);
  tableModel()->removeSelectionFromMRML(selectionModel()->selectedIndexes(), true);
}

//-----------------------------------------------------------------------------
bool qMRMLTableView::firstRowLocked()const
{
  Q_D(const qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(firstRowLocked, false);
  if (tableModel()->transposed())
    {
    return mrmlTableNode()->GetUseFirstColumnAsRowHeader();
    }
  else
    {
    return mrmlTableNode()->GetUseColumnNameAsColumnHeader();
    }
}


//-----------------------------------------------------------------------------
void qMRMLTableView::setFirstRowLocked(bool locked)
{
  Q_D(qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(firstRowLocked);
  if (tableModel()->transposed())
    {
    if (mrmlTableNode()->GetUseFirstColumnAsRowHeader()==locked)
      {
      //no change
      return;
      }
    mrmlTableNode()->SetUseFirstColumnAsRowHeader(locked);
    }
  else
    {
    if (mrmlTableNode()->GetUseColumnNameAsColumnHeader()==locked)
      {
      //no change
      return;
      }
    mrmlTableNode()->SetUseColumnNameAsColumnHeader(locked);
    }
  this->resizeColumnsToContents();
}

//-----------------------------------------------------------------------------
bool qMRMLTableView::firstColumnLocked()const
{
  Q_D(const qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(firstColumnLocked, false);
  if (tableModel()->transposed())
    {
    return mrmlTableNode()->GetUseColumnNameAsColumnHeader();
    }
  else
    {
    return mrmlTableNode()->GetUseFirstColumnAsRowHeader();
    }
}

//-----------------------------------------------------------------------------
void qMRMLTableView::setFirstColumnLocked(bool locked)
{
  Q_D(qMRMLTableView);
  VERIFY_TABLE_MODEL_AND_NODE(setFirstColumnLocked);
  if (tableModel()->transposed())
    {
    if (mrmlTableNode()->GetUseColumnNameAsColumnHeader()==locked)
      {
      //no change
      return;
      }
    mrmlTableNode()->SetUseColumnNameAsColumnHeader(locked);
    }
  else
    {
    if (mrmlTableNode()->GetUseFirstColumnAsRowHeader()==locked)
      {
      //no change
      return;
      }
    mrmlTableNode()->SetUseFirstColumnAsRowHeader(locked);
    }
  this->resizeColumnsToContents();
}
