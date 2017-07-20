/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright 2015 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Andras Lasso (PerkLab, Queen's
  University) and Kevin Wang (Princess Margaret Hospital, Toronto) and was
  supported through OCAIRO and the Applied Cancer Research Unit program of
  Cancer Care Ontario.

==============================================================================*/

// QT includes
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QKeyEvent>
#include <QSortFilterProxyModel>
#include <QString>
#include <QToolButton>

// CTK includes
#include <ctkPopupWidget.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkStringArray.h>

// qMRML includes
#include "qMRMLTableView.h"
#include "qMRMLTableView_p.h"
#include "qMRMLTableModel.h"

// MRML includes
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLPlotNode.h>
#include <vtkMRMLPlotLayoutNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTableNode.h>
#include <vtkMRMLTableViewNode.h>

#define CTK_CHECK_AND_RETURN_IF_FAIL(FUNC) \
  if (!FUNC(Q_FUNC_INFO))       \
    {                              \
    return;                        \
    }

#define CTK_CHECK_AND_RETURN_FALSE_IF_FAIL(FUNC) \
  if (!FUNC(Q_FUNC_INFO))            \
    {                                   \
    return false;                       \
    }

//------------------------------------------------------------------------------
qMRMLTableViewPrivate::qMRMLTableViewPrivate(qMRMLTableView& object)
  : q_ptr(&object)
  , MRMLScene(0)
  , MRMLTableViewNode(0)
{
}

//---------------------------------------------------------------------------
qMRMLTableViewPrivate::~qMRMLTableViewPrivate()
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

  // Let the view expand in both directions
  q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  this->PopupWidget = new ctkPopupWidget;
  QHBoxLayout* popupLayout = new QHBoxLayout;
  popupLayout->addWidget(new QToolButton);
  this->PopupWidget->setLayout(popupLayout);
}

//---------------------------------------------------------------------------
void qMRMLTableViewPrivate::setMRMLScene(vtkMRMLScene* newScene)
{
  //Q_Q(qMRMLTableView);
  if (newScene == this->MRMLScene)
    {
    return;
    }
  this->qvtkReconnect(
    this->mrmlScene(), newScene,
    vtkMRMLScene::StartBatchProcessEvent, this, SLOT(startProcessing()));

  this->qvtkReconnect(
    this->mrmlScene(), newScene,
    vtkMRMLScene::EndBatchProcessEvent, this, SLOT(endProcessing()));
  this->MRMLScene = newScene;
}

// --------------------------------------------------------------------------
void qMRMLTableViewPrivate::startProcessing()
{
}

// --------------------------------------------------------------------------
void qMRMLTableViewPrivate::endProcessing()
{
  this->updateWidgetFromViewNode();
}

// --------------------------------------------------------------------------
vtkMRMLScene* qMRMLTableViewPrivate::mrmlScene()
{
  return this->MRMLScene;
}

// --------------------------------------------------------------------------
bool qMRMLTableViewPrivate::verifyTableModelAndNode(const char* methodName) const
{
  Q_Q(const qMRMLTableView);
  if (!q->tableModel())
    {
    qWarning() << "qMRMLTableView:: " << methodName << " failed: invalid model";
    return false;
    }
  if (!q->mrmlTableNode())
    {
    qWarning() << "qMRMLTableView::" << methodName << " failed: invalid node";
    return false;
    }
  return true;
}

// --------------------------------------------------------------------------
void qMRMLTableViewPrivate::updateWidgetFromViewNode()
{
  Q_Q(qMRMLTableView);
  if (!this->MRMLScene || !this->MRMLTableViewNode)
    {
    q->setMRMLTableNode((vtkMRMLNode*)NULL);
    return;
    }

  if (!q->isEnabled())
    {
    return;
    }

  // Get the TableNode
  q->setMRMLTableNode(this->MRMLTableViewNode->GetTableNode());
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
bool qMRMLTableView::transposed()const
{
  Q_D(const qMRMLTableView);
  CTK_CHECK_AND_RETURN_FALSE_IF_FAIL(d->verifyTableModelAndNode)
  return tableModel()->transposed();
}

//------------------------------------------------------------------------------
void qMRMLTableView::setTransposed(bool transposed)
{
  Q_D(qMRMLTableView);
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)
  tableModel()->setTransposed(transposed);
}

//------------------------------------------------------------------------------
void qMRMLTableView::keyPressEvent(QKeyEvent *event)
{
  if(event->matches(QKeySequence::Copy) )
    {
    this->copySelection();
    return;
    }
  if(event->matches(QKeySequence::Paste) )
    {
    this->pasteSelection();
    return;
    }

  // Prevent giving the focus to the previous/next widget if arrow keys are used
  // at the edge of the table (without this: if the current cell is in the top
  // row and user press the Up key, the focus goes from the table to the previous
  // widget in the tab order)
  if (model() && (
    (event->key() == Qt::Key_Left && currentIndex().column() == 0)
    || (event->key() == Qt::Key_Up && currentIndex().row() == 0)
    || (event->key() == Qt::Key_Right && currentIndex().column() == model()->columnCount()-1)
    || (event->key() == Qt::Key_Down && currentIndex().row() == model()->rowCount()-1) ) )
    {
    return;
    }
  QTableView::keyPressEvent(event);
}

//-----------------------------------------------------------------------------
void qMRMLTableView::copySelection()
{
  Q_D(qMRMLTableView);
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)

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
      QStandardItem *item = mrmlModel->item(rowIndex, columnIndex);
      if (item->isCheckable())
        {
        textToCopy.append(item->checkState() == Qt::Checked ? "1" : "0");
        }
      else
        {
        textToCopy.append(item->text());
        }
      }
    }

  QApplication::clipboard()->setText(textToCopy);
}

//-----------------------------------------------------------------------------
void qMRMLTableView::pasteSelection()
{
  Q_D(qMRMLTableView);
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)

  QString text = QApplication::clipboard()->text();
  if (text.isEmpty())
    {
    return;
    }
  QStringList lines = text.split('\n');
  if (lines.empty())
    {
    // nothing to paste
    return;
    }
  if (lines.back().isEmpty())
    {
    // usually there is an extra empty line at the end
    // remove that to avoid adding an extra empty line to the table
    lines.pop_back();
    }
  if (lines.empty())
    {
    // nothing to paste
    return;
    }

  // If there is no selection then paste from top-left
  qMRMLTableModel* mrmlModel = tableModel();
  int rowIndex = currentIndex().row();
  if (rowIndex < 0)
    {
    rowIndex = 0;
    }
  int startColumnIndex = currentIndex().column();
  if (startColumnIndex < 0)
    {
    startColumnIndex = 0;
    }

  // If there are multiple table views then each cell modification would trigger
  // a table update, which may be very slow in case of large tables, therefore
  // we need to use StartModify/EndModify.
  vtkMRMLTableNode* tableNode = mrmlTableNode();
  int wasModified = tableNode->StartModify();

  // Pre-allocate new rows (to reduce number of updateModelFromMRML() calls
  if (tableNode->GetNumberOfColumns() == 0)
    {
    // insertRow() may insert two rows if the table is empty (one column header + one data item),
    // which could cause an extra row added to the table. To prevent this, we add a column instead,
    // which is just a single value.
    insertColumn();
    mrmlModel->updateModelFromMRML();
    }
  for (int i = lines.size() - (mrmlModel->rowCount() - rowIndex); i>0; i--)
    {
    insertRow();
    }
  mrmlModel->updateModelFromMRML();

  foreach(QString line, lines)
    {
    int columnIndex = startColumnIndex;
    QStringList cells = line.split('\t');
    foreach(QString cell, cells)
      {
      // Pre-allocate new columns (enough for at least for storing all the items in the current row)
      if (columnIndex >= mrmlModel->columnCount())
        {
        for (int i = cells.size() - (mrmlModel->columnCount() - startColumnIndex); i>0; i--)
          {
          insertColumn();
          }
        mrmlModel->updateModelFromMRML();
        }
      // Set values in items
      QStandardItem* item = mrmlModel->item(rowIndex,columnIndex);
      if (item != NULL)
        {
        if (item->isCheckable())
          {
          item->setCheckState(cell.toInt() == 0 ? Qt::Unchecked : Qt::Checked);
          }
        else
          {
          item->setText(cell);
          }
        }
      else
        {
        qWarning() << "Failed to set " << cell << " in table cell (" << rowIndex << ", " << columnIndex << ")";
        }
      columnIndex++;
      }
    rowIndex++;
    }
  tableNode->EndModify(wasModified);
}

//-----------------------------------------------------------------------------
void qMRMLTableView::plotSelection()
{
  Q_D(qMRMLTableView);
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)

  vtkMRMLTableNode* tableNode = mrmlTableNode();

  if(!this->mrmlScene())
    {
    qWarning() << "qMRMLTableView::plotSelection failed: no mrmlScene available";
    return;
    }

  vtkMRMLSelectionNode* selectionNode = vtkMRMLSelectionNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID("vtkMRMLSelectionNodeSingleton"));

  if (!selectionNode)
    {
    qWarning() << "qMRMLTableView::plotSelection failed: invalid selection Node";
    return;
    }

  vtkSmartPointer<vtkMRMLPlotLayoutNode> mrmlLayoutPlot = vtkMRMLPlotLayoutNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(selectionNode->GetActivePlotLayoutID()));

  if (!mrmlLayoutPlot)
    {
    mrmlLayoutPlot = vtkSmartPointer<vtkMRMLPlotLayoutNode>::New();
    this->mrmlScene()->AddNode(mrmlLayoutPlot);
    selectionNode->SetActivePlotLayoutID(mrmlLayoutPlot->GetID());
    }

  qMRMLTableModel* mrmlModel = tableModel();
  QItemSelectionModel* selection = selectionModel();
  vtkNew<vtkIntArray> columnIndexs;
  for (int columnIndex = 0; columnIndex < mrmlModel->columnCount(); columnIndex++)
    {
    if (!selection->columnIntersectsSelection(columnIndex, QModelIndex()))
      {
      // no items are selected in this entire column, skip it
      continue;
      }
    columnIndexs->InsertNextValue(columnIndex);
    }

  if (columnIndexs->GetNumberOfValues() < 2)
    {
    QString message = QString("To generate a plot, please select at least two columns");
    qCritical() << Q_FUNC_INFO << ": " << message;
    QMessageBox::warning(NULL, tr("Failed to plot data"), message);
    return;
    }

  // Remove columns/plots not selected from mrmlLayoutPlot
  mrmlLayoutPlot->ClearPlotIDs();

  // Add columns/plots not selected from mrmlLayoutPlot
  for (int columnIndex = 1; columnIndex < columnIndexs->GetNumberOfValues(); columnIndex++)
    {
    std::string columnName = tableNode->GetColumnName(columnIndexs->GetValue(columnIndex));

    vtkMRMLPlotNode *mrmlplot = NULL;
    vtkCollection* colPlots = this->mrmlScene()->GetNodesByClassByName("vtkMRMLPlotNode", columnName.c_str());

    if (!colPlots)
      {
      continue;
      }

    for (int plotIndex = 0; plotIndex < colPlots->GetNumberOfItems(); plotIndex++)
      {
      mrmlplot = vtkMRMLPlotNode::SafeDownCast(colPlots->GetItemAsObject(plotIndex));
      if (mrmlplot == NULL)
        {
        continue;
        }
      else
        {
        break;
        }
      }

    if (mrmlplot == NULL)
      {
      vtkSmartPointer<vtkMRMLNode> node = vtkSmartPointer<vtkMRMLNode>::Take(
        this->mrmlScene()->CreateNodeByClass("vtkMRMLPlotNode"));
      mrmlplot = vtkMRMLPlotNode::SafeDownCast(node);
      this->mrmlScene()->AddNode(mrmlplot);
      mrmlplot->SetName(columnName.c_str());
      mrmlplot->SetXColumn(columnIndexs->GetValue(0));
      mrmlplot->SetYColumn(columnIndexs->GetValue(columnIndex));
      mrmlplot->SetAndObserveTableNodeID(tableNode->GetID());
      }

    std::string namePlotNode = mrmlplot->GetName();
    std::size_t found = namePlotNode.find("Markups");
    if (found != std::string::npos)
      {
      mrmlLayoutPlot->RemovePlotAndObservationByName(namePlotNode.c_str());
      continue;
      }

    std::string Type = mrmlLayoutPlot->GetProperty("type");
    if (!Type.compare("Line"))
      {
      mrmlplot->SetType(vtkMRMLPlotNode::LINE);
      }
    else if (!Type.compare("Scatter"))
      {
      mrmlplot->SetType(vtkMRMLPlotNode::POINTS);
      }
    else if (!Type.compare("Line and Scatter"))
      {
      mrmlplot->SetType(vtkMRMLPlotNode::LINE);

      vtkSmartPointer<vtkMRMLPlotNode> plotNodeCopy = vtkMRMLPlotNode::SafeDownCast
        (mrmlplot->GetNodeReference("Markups"));

      if (plotNodeCopy)
        {
        plotNodeCopy->SetType(vtkMRMLPlotNode::POINTS);
        }
      else
        {
        vtkSmartPointer<vtkMRMLNode> node = vtkSmartPointer<vtkMRMLNode>::Take
          (this->mrmlScene()->CreateNodeByClass("vtkMRMLPlotNode"));
        plotNodeCopy = vtkMRMLPlotNode::SafeDownCast(node);

        std::string namePlotNodeCopy = namePlotNode + " Markups";

        plotNodeCopy->CopyAndSetNameAndType(mrmlplot, namePlotNodeCopy.c_str(), vtkMRMLPlotNode::POINTS);
        this->mrmlScene()->AddNode(plotNodeCopy);
        mrmlplot->AddNodeReferenceID("Markups", plotNodeCopy->GetID());
        plotNodeCopy->AddNodeReferenceID("Markups", mrmlplot->GetID());
        }

      mrmlLayoutPlot->AddAndObservePlot(plotNodeCopy->GetName(), plotNodeCopy->GetID());
      }
    else if (!Type.compare("Bar"))
      {
      mrmlplot->SetType(vtkMRMLPlotNode::BAR);
      }

    mrmlLayoutPlot->AddAndObservePlot(columnName.c_str(), mrmlplot->GetID());

    colPlots->Delete();
    colPlots = NULL;
    }
}

//-----------------------------------------------------------------------------
void qMRMLTableView::insertColumn()
{
  Q_D(qMRMLTableView);
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)
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
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)
  tableModel()->removeSelectionFromMRML(selectionModel()->selectedIndexes(), false);
  clearSelection();
}

//-----------------------------------------------------------------------------
void qMRMLTableView::insertRow()
{
  Q_D(qMRMLTableView);
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)
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
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)
  tableModel()->removeSelectionFromMRML(selectionModel()->selectedIndexes(), true);
  clearSelection();
}

//-----------------------------------------------------------------------------
bool qMRMLTableView::firstRowLocked()const
{
  Q_D(const qMRMLTableView);
  CTK_CHECK_AND_RETURN_FALSE_IF_FAIL(d->verifyTableModelAndNode)
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
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)
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
  CTK_CHECK_AND_RETURN_FALSE_IF_FAIL(d->verifyTableModelAndNode)
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
  CTK_CHECK_AND_RETURN_IF_FAIL(d->verifyTableModelAndNode)
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

//------------------------------------------------------------------------------
void qMRMLTableView::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLTableView);
  if (newScene == d->MRMLScene)
    {
    return;
    }

  d->setMRMLScene(newScene);

  if (d->MRMLTableViewNode && newScene != d->MRMLTableViewNode->GetScene())
    {
    this->setMRMLTableViewNode(0);
    }

  emit mrmlSceneChanged(newScene);
}

//---------------------------------------------------------------------------
void qMRMLTableView::setMRMLTableViewNode(vtkMRMLTableViewNode* newTableViewNode)
{
  Q_D(qMRMLTableView);
  if (d->MRMLTableViewNode == newTableViewNode)
    {
    return;
    }

  // connect modified event on TableViewNode to updating the widget
  d->qvtkReconnect(
    d->MRMLTableViewNode, newTableViewNode,
    vtkCommand::ModifiedEvent, d, SLOT(updateWidgetFromViewNode()));

  // cache the TableViewNode
  d->MRMLTableViewNode = newTableViewNode;

  // make sure the gui is up to date
  d->updateWidgetFromViewNode();
}

//---------------------------------------------------------------------------
vtkMRMLTableViewNode* qMRMLTableView::mrmlTableViewNode()const
{
  Q_D(const qMRMLTableView);
  return d->MRMLTableViewNode;
}

//---------------------------------------------------------------------------
vtkMRMLScene* qMRMLTableView::mrmlScene()const
{
  Q_D(const qMRMLTableView);
  return d->MRMLScene;
}
