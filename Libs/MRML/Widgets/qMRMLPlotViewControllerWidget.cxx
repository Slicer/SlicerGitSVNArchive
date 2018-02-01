/*==============================================================================

  Copyright (c) Kapteyn Astronomical Institute
  University of Groningen, Groningen, Netherlands. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Davide Punzo, Kapteyn Astronomical Institute,
  and was supported through the European Research Council grant nr. 291531.

==============================================================================*/

// Qt includes
#include <QActionGroup>
#include <QDebug>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QHBoxLayout>

// VTK includes
#include <vtkCollection.h>
#include <vtkFloatArray.h>
#include <vtkPlot.h>
#include <vtkPlotLine.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

// CTK includes
#include <ctkLogger.h>
#include <ctkPopupWidget.h>

// qMRML includes
#include "qMRMLColors.h"
#include "qMRMLNodeFactory.h"
#include "qMRMLSceneViewMenu.h"
#include "qMRMLPlotView.h"
#include "qMRMLPlotViewControllerWidget_p.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLPlotDataNode.h>
#include <vtkMRMLPlotChartNode.h>
#include <vtkMRMLPlotViewNode.h>
#include <vtkMRMLSceneViewNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLTableNode.h>

// STD include
#include <string>

//--------------------------------------------------------------------------
static ctkLogger logger("org.slicer.libs.qmrmlwidgets.qMRMLPlotViewControllerWidget");
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// qMRMLPlotViewControllerWidgetPrivate methods

//---------------------------------------------------------------------------
qMRMLPlotViewControllerWidgetPrivate::qMRMLPlotViewControllerWidgetPrivate(
  qMRMLPlotViewControllerWidget& object)
  : Superclass(object)
{
  this->FitToWindowToolButton = 0;

  this->PlotChartNode = 0;
  this->PlotViewNode = 0;
  this->PlotView = 0;
}

//---------------------------------------------------------------------------
qMRMLPlotViewControllerWidgetPrivate::~qMRMLPlotViewControllerWidgetPrivate()
{
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::setupPopupUi()
{
  Q_Q(qMRMLPlotViewControllerWidget);

  this->Superclass::setupPopupUi();
  this->PopupWidget->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
  this->Ui_qMRMLPlotViewControllerWidget::setupUi(this->PopupWidget);

  // Connect PlotChart selector
  this->connect(this->plotChartComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
                SLOT(onPlotChartNodeSelected(vtkMRMLNode*)));

  // Connect Plot selector
  this->connect(this->plotDataComboBox, SIGNAL(checkedNodesChanged()),
                SLOT(onPlotDataNodesSelected()));
  this->connect(this->plotDataComboBox, SIGNAL(nodeAddedByUser(vtkMRMLNode*)),
                SLOT(onPlotDataNodeAdded(vtkMRMLNode*)));
  this->connect(this->plotDataComboBox, SIGNAL(nodeAboutToBeEdited(vtkMRMLNode*)),
                SLOT(onPlotDataNodeEdited(vtkMRMLNode*)));

  // Connect the Plot Type selector
  this->connect(this->plotTypeComboBox, SIGNAL(currentIndexChanged(const QString&)),
                SLOT(onPlotTypeChanged(const QString&)));

  // Connect xAxis comboBox
  this->connect(this->xAxisComboBox, SIGNAL(currentIndexChanged(int)),
                SLOT(onXAxisChanged(int)));

  // Connect Markers comboBox
  this->connect(this->markersComboBox, SIGNAL(currentIndexChanged(const QString&)),
                SLOT(onMarkersChanged(const QString&)));

  // Connect the actions
  QObject::connect(this->actionShow_Grid, SIGNAL(toggled(bool)),
                   q, SLOT(gridVisibility(bool)));
  QObject::connect(this->actionShow_Legend, SIGNAL(toggled(bool)),
                   q, SLOT(legendVisibility(bool)));
  QObject::connect(this->actionFit_to_window, SIGNAL(triggered()),
                   q, SLOT(fitPlotToAxes()));

  // Connect the buttons
  this->showGridToolButton->setDefaultAction(this->actionShow_Grid);
  this->showLegendToolButton->setDefaultAction(this->actionShow_Legend);

  // Connect the checkboxes
  QObject::connect(this->showTitleCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(TitleVisibility(bool)));
  QObject::connect(this->showXAxisLabelCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(showXAxisLabel(bool)));
  QObject::connect(this->showYAxisLabelCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(showYAxisLabel(bool)));

  // Connect the line edit boxes
  QObject::connect(this->titleLineEdit, SIGNAL(textEdited(const QString&)),
                   q, SLOT(setTitle(const QString&)));
  QObject::connect(this->xAxisLabelLineEdit, SIGNAL(textEdited(const QString&)),
                   q, SLOT(setXAxisLabel(const QString&)));
  QObject::connect(this->yAxisLabelLineEdit, SIGNAL(textEdited(const QString&)),
                   q, SLOT(setYAxisLabel(const QString&)));

  // Connect the edit buttons to work around the issues of the
  // LineEdits not capturing the mouse focus when in ControllerWidget
  QObject::connect(this->editTitleButton, SIGNAL(clicked()),
                   q, SLOT(editTitle()));
  QObject::connect(this->editXAxisLabelButton, SIGNAL(clicked()),
                   q, SLOT(editXAxisLabel()));
  QObject::connect(this->editYAxisLabelButton, SIGNAL(clicked()),
                   q, SLOT(editYAxisLabel()));

  // Connect the scene
  QObject::connect(q, SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
                   this->plotDataComboBox, SLOT(setMRMLScene(vtkMRMLScene*)));

  QObject::connect(q, SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
                   this->plotChartComboBox, SLOT(setMRMLScene(vtkMRMLScene*)));
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::init()
{
  Q_Q(qMRMLPlotViewControllerWidget);

  this->Superclass::init();

  this->ViewLabel->setText(qMRMLPlotViewControllerWidget::tr("P"));
  this->BarLayout->addStretch(1);
  this->setColor(QColor(27, 198, 207));

  this->FitToWindowToolButton = new QToolButton(q);
  this->FitToWindowToolButton->setAutoRaise(true);
  this->FitToWindowToolButton->setDefaultAction(this->actionFit_to_window);
  this->FitToWindowToolButton->setFixedSize(15, 15);
  this->BarLayout->insertWidget(2, this->FitToWindowToolButton);
}


// --------------------------------------------------------------------------
vtkMRMLPlotChartNode* qMRMLPlotViewControllerWidgetPrivate::GetPlotChartNodeFromView()
{
  Q_Q(qMRMLPlotViewControllerWidget);

  if (!this->PlotViewNode || !q->mrmlScene())
    {
    // qDebug() << "No PlotViewNode or no Scene";
    return 0;
    }

  // Get the current PlotChart node
  vtkMRMLPlotChartNode *PlotChartNodeFromViewNode
    = vtkMRMLPlotChartNode::SafeDownCast(q->mrmlScene()->GetNodeByID(this->PlotViewNode->GetPlotChartNodeID()));

  return PlotChartNodeFromViewNode;
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onPlotChartNodeSelected(vtkMRMLNode *node)
{
  Q_Q(qMRMLPlotViewControllerWidget);

  vtkMRMLPlotChartNode *mrmlPlotChartNode = vtkMRMLPlotChartNode::SafeDownCast(node);

  if (!this->PlotViewNode || this->PlotChartNode == mrmlPlotChartNode)
    {
    return;
    }

  this->PlotViewNode->SetPlotChartNodeID(mrmlPlotChartNode ? mrmlPlotChartNode->GetID() : NULL);

  vtkMRMLSelectionNode* selectionNode = vtkMRMLSelectionNode::SafeDownCast(
    q->mrmlScene() ? q->mrmlScene()->GetNodeByID("vtkMRMLSelectionNodeSingleton") : NULL);
  if (selectionNode)
    {
    selectionNode->SetActivePlotChartID(mrmlPlotChartNode ? mrmlPlotChartNode->GetID() : "");
    }

  q->updateWidgetFromMRML();
}


// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onPlotDataNodesSelected()
{
  if (!this->PlotViewNode || !this->PlotChartNode)
    {
    return;
    }

  std::vector<std::string> plotDataNodesIDs;
  this->PlotChartNode->GetPlotDataNodeIDs(plotDataNodesIDs);

  // loop over arrays in the widget
  for (int idx = 0; idx < this->plotDataComboBox->nodeCount(); idx++)
    {
    vtkMRMLPlotDataNode *dn = vtkMRMLPlotDataNode::SafeDownCast(this->plotDataComboBox->nodeFromIndex(idx));

    bool checked = (this->plotDataComboBox->checkState(dn) == Qt::Checked);

    // is the node in the Plot?
    bool found = false;
    std::vector<std::string>::iterator it = plotDataNodesIDs.begin();
    for (; it != plotDataNodesIDs.end(); ++it)
      {
      if (!strcmp(dn->GetID(), (*it).c_str()))
        {
        if (!checked)
          {
          // plot is not checked but currently in the LayoutPlot, remove it
          // (might want to cache the old name in case user adds it back)
          this->PlotChartNode->RemovePlotDataNodeID((*it).c_str());
          }
        found = true;
        break;
        }
      }
    if (!found)
      {
      if (checked)
        {
        // plot is checked but not currently in the LayoutPlot, add it
        this->PlotChartNode->AddAndObservePlotDataNodeID(dn->GetID());
        }
      }
  }
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onPlotDataNodeAdded(vtkMRMLNode *node)
{
  Q_Q(qMRMLPlotViewControllerWidget);

  if (!this->PlotChartNode || !q->mrmlScene())
    {
    return;
    }

  vtkMRMLPlotDataNode *plotDataNode = vtkMRMLPlotDataNode::SafeDownCast(node);

  if (plotDataNode)
    {
    return;
    }

  q->mrmlScene()->AddNode(plotDataNode);

  // Add the reference of the PlotDataNode in the active PlotChartNode
  this->PlotChartNode->AddAndObservePlotDataNodeID(plotDataNode->GetID());
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onPlotDataNodeEdited(vtkMRMLNode *node)
{
  if (node == NULL)
    {
    return;
    }

  QString message = QString("To edit the node %1 : Please navigate to"
                            " the ViewController Module. Additional editing options"
                            " are available under the Advanced menu.").arg(node->GetName());
  qWarning() << Q_FUNC_INFO << ": " << message;
  QMessageBox::warning(NULL, tr("Edit PlotDataNode"), message);
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onPlotTypeChanged(const QString &type)
{
  if (!this->PlotChartNode)
    {
    return;
    }
  this->PlotChartNode->SetPropertyToAllPlotDataNodes(vtkMRMLPlotChartNode::PlotType,
    type.toLatin1().constData());
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onXAxisChanged(int index)
{
  if (!this->PlotChartNode)
    {
    return;
    }
  if (index >= 0)
    {
    this->PlotChartNode->SetPropertyToAllPlotDataNodes(vtkMRMLPlotChartNode::PlotXColumnName,
      this->xAxisComboBox->itemData(index).toString().toLatin1().constData());
    }
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onMarkersChanged(const QString &markerStyle)
{
  if (!this->PlotChartNode)
    {
    return;
    }
  this->PlotChartNode->SetPropertyToAllPlotDataNodes(vtkMRMLPlotChartNode::PlotMarkerStyle,
    markerStyle.toLatin1().constData());
}

// --------------------------------------------------------------------------
// qMRMLPlotViewControllerWidget methods

// --------------------------------------------------------------------------
qMRMLPlotViewControllerWidget::qMRMLPlotViewControllerWidget(QWidget* parentWidget)
  : Superclass(new qMRMLPlotViewControllerWidgetPrivate(*this), parentWidget)
{
  Q_D(qMRMLPlotViewControllerWidget);
  d->init();
}

// --------------------------------------------------------------------------
qMRMLPlotViewControllerWidget::~qMRMLPlotViewControllerWidget()
{
  this->setMRMLScene(0);
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setPlotView(qMRMLPlotView* view)
{
  Q_D(qMRMLPlotViewControllerWidget);
  d->PlotView = view;
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setViewLabel(const QString& newViewLabel)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if (d->PlotViewNode)
    {
    logger.error("setViewLabel should be called before setViewNode !");
    return;
    }

  d->PlotViewLabel = newViewLabel;
  d->ViewLabel->setText(d->PlotViewLabel);
}

//---------------------------------------------------------------------------
CTK_GET_CPP(qMRMLPlotViewControllerWidget, QString, viewLabel, PlotViewLabel);


//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setMRMLPlotViewNode(
    vtkMRMLPlotViewNode * viewNode)
{
  Q_D(qMRMLPlotViewControllerWidget);
  this->qvtkReconnect(d->PlotViewNode, viewNode, vtkCommand::ModifiedEvent,
                      this, SLOT(updateWidgetFromMRML()));
  d->PlotViewNode = viewNode;
  this->updateWidgetFromMRML();
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::gridVisibility(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);
  if(!d->PlotChartNode)
    {
    return;
    }
  d->PlotChartNode->SetGridVisibility(show);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::legendVisibility(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);
  if(!d->PlotChartNode)
    {
    return;
    }
  d->PlotChartNode->SetLegendVisibility(show);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::TitleVisibility(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);
  if (!d->PlotChartNode)
    {
    return;
    }
  d->PlotChartNode->SetTitleVisibility(show);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::fitPlotToAxes()
{
  Q_D(qMRMLPlotViewControllerWidget);
  if(!d->PlotView)
    {
    return;
    }
  d->PlotView->fitToContent();
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::showXAxisLabel(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);
  if(!d->PlotChartNode)
    {
    return;
    }
  d->PlotChartNode->SetXAxisTitleVisibility(show);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::showYAxisLabel(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);
  if (!d->PlotChartNode)
    {
    return;
    }
  d->PlotChartNode->SetYAxisTitleVisibility(show);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setTitle(const QString &str)
{
  Q_D(qMRMLPlotViewControllerWidget);
  if (!d->PlotChartNode)
    {
    return;
    }
  d->PlotChartNode->SetTitle(str.toLatin1().constData());
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setXAxisLabel(const QString &str)
{
  Q_D(qMRMLPlotViewControllerWidget);
  if (!d->PlotChartNode)
    {
    return;
    }
  d->PlotChartNode->SetXAxisTitle(str.toLatin1().constData());
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setYAxisLabel(const QString &str)
{
  Q_D(qMRMLPlotViewControllerWidget);
  if (!d->PlotChartNode)
    {
    return;
    }
  d->PlotChartNode->SetYAxisTitle(str.toLatin1().constData());
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::editTitle()
{
  Q_D(qMRMLPlotViewControllerWidget);

  if (!d->PlotChartNode)
    {
    return;
    }

  // Bring up a dialog to request a title
  bool ok = false;
  QString newTitle = QInputDialog::getText(
    this, "Edit Title", "Title",
    QLineEdit::Normal, d->PlotChartNode->GetTitle() ? d->PlotChartNode->GetTitle() : "", &ok);
  if (!ok)
    {
    return;
    }

  // Set the parameter
  this->setTitle(newTitle);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::editXAxisLabel()
{
  Q_D(qMRMLPlotViewControllerWidget);

  if (!d->PlotChartNode)
    {
    return;
    }

  // Bring up a dialog to request a title
  bool ok = false;
  QString newXLabel = QInputDialog::getText(
    this, "Edit X-axis label", "X-axis label",
    QLineEdit::Normal, d->PlotChartNode->GetXAxisTitle() ? d->PlotChartNode->GetXAxisTitle() : "", &ok);
  if (!ok)
    {
    return;
    }

  // Set the parameter
  this->setXAxisLabel(newXLabel);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::editYAxisLabel()
{
  Q_D(qMRMLPlotViewControllerWidget);

  if (!d->PlotChartNode)
    {
    return;
    }

  // Bring up a dialog to request a title
  bool ok = false;
  QString newYLabel = QInputDialog::getText(
    this, "Edit Y-axis label", "Y-axis label",
    QLineEdit::Normal, d->PlotChartNode->GetYAxisTitle() ? d->PlotChartNode->GetYAxisTitle() : "", &ok);
  if (!ok)
    {
    return;
    }

  // Set the parameter
  this->setYAxisLabel(newYLabel);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLPlotViewControllerWidget);

  if (!d->PlotViewNode || !this->mrmlScene())
    {
    return;
    }

  // PlotChartNode selector
  vtkMRMLPlotChartNode* mrmlPlotChartNode = d->GetPlotChartNodeFromView();

  if (mrmlPlotChartNode != d->PlotChartNode)
    {
    this->qvtkReconnect(d->PlotChartNode, mrmlPlotChartNode, vtkCommand::ModifiedEvent,
      this, SLOT(updateWidgetFromMRML()));
    d->PlotChartNode = mrmlPlotChartNode;
    }

  bool wasBlocked = d->plotChartComboBox->blockSignals(true);
  d->plotChartComboBox->setCurrentNode(mrmlPlotChartNode);
  d->plotChartComboBox->blockSignals(wasBlocked);

  if (!mrmlPlotChartNode)
    {
    // Set the widgets to default states
    bool wasBlocked = d->plotTypeComboBox->blockSignals(true);
    d->plotTypeComboBox->setCurrentIndex(-1);
    d->plotTypeComboBox->blockSignals(wasBlocked);
    wasBlocked = d->xAxisComboBox->blockSignals(true);
    d->xAxisComboBox->clear();
    d->xAxisComboBox->blockSignals(wasBlocked);
    wasBlocked = d->markersComboBox->blockSignals(true);
    d->markersComboBox->setCurrentIndex(-1);
    d->markersComboBox->blockSignals(wasBlocked);
    d->actionShow_Grid->setChecked(true);
    d->actionShow_Legend->setChecked(true);
    d->showTitleCheckBox->setChecked(true);
    d->showXAxisLabelCheckBox->setChecked(true);
    d->showYAxisLabelCheckBox->setChecked(true);
    d->titleLineEdit->clear();
    d->xAxisLabelLineEdit->clear();
    d->yAxisLabelLineEdit->clear();

    bool plotBlockSignals = d->plotDataComboBox->blockSignals(true);
    for (int idx = 0; idx < d->plotDataComboBox->nodeCount(); idx++)
      {
      d->plotDataComboBox->setCheckState(d->plotDataComboBox->nodeFromIndex(idx),
                                         Qt::Unchecked);
      }
    d->plotDataComboBox->blockSignals(plotBlockSignals);

    return;
    }

  // Plot and x axis selector
  bool xAxisComboBoxBlockSignals = d->xAxisComboBox->blockSignals(true);
  bool plotBlockSignals = d->plotDataComboBox->blockSignals(true);

  for (int idx = 0; idx < d->plotDataComboBox->nodeCount(); idx++)
    {
    vtkMRMLNode* node = d->plotDataComboBox->nodeFromIndex(idx);
    d->plotDataComboBox->setCheckState(node, Qt::Unchecked);
    }

  d->xAxisComboBox->clear();
  d->xAxisComboBox->addItem("Indexes", QString());

  std::vector<std::string> plotDataNodesIDs;
  mrmlPlotChartNode->GetPlotDataNodeIDs(plotDataNodesIDs);
  for (std::vector<std::string>::iterator it = plotDataNodesIDs.begin();
    it != plotDataNodesIDs.end(); ++it)
    {
    vtkMRMLPlotDataNode *plotDataNode = vtkMRMLPlotDataNode::SafeDownCast
      (this->mrmlScene()->GetNodeByID((*it).c_str()));
    if (plotDataNode == NULL)
      {
      continue;
      }
    d->plotDataComboBox->setCheckState(plotDataNode, Qt::Checked);
    vtkMRMLTableNode* mrmlTableNode = plotDataNode->GetTableNode();
    if (mrmlTableNode == NULL)
      {
      continue;
      }
    int numCol = mrmlTableNode->GetNumberOfColumns();
    for (int ii = 0; ii < numCol; ++ii)
      {
      QString columnName = QString::fromStdString(mrmlTableNode->GetColumnName(ii));
      if (d->xAxisComboBox->findData(columnName) == -1)
        {
        d->xAxisComboBox->addItem(columnName, columnName);
        }
      }
    }

  std::string xColumnName;
  if (mrmlPlotChartNode->GetPropertyFromAllPlotDataNodes(vtkMRMLPlotChartNode::PlotXColumnName, xColumnName))
    {
    d->xAxisComboBox->setCurrentIndex(d->xAxisComboBox->findData(xColumnName.c_str()));
    }
  else
    {
    d->xAxisComboBox->setCurrentIndex(-1);
    }


  d->xAxisComboBox->blockSignals(xAxisComboBoxBlockSignals);
  d->plotDataComboBox->blockSignals(plotBlockSignals);

  d->actionShow_Grid->setChecked(mrmlPlotChartNode->GetGridVisibility());
  d->actionShow_Legend->setChecked(mrmlPlotChartNode->GetLegendVisibility());

  // Titles, axis labels (checkboxes AND text widgets)
  d->showTitleCheckBox->setChecked(mrmlPlotChartNode->GetTitleVisibility());
  d->titleLineEdit->setText(mrmlPlotChartNode->GetTitle() ? mrmlPlotChartNode->GetTitle() : "");

  d->showXAxisLabelCheckBox->setChecked(mrmlPlotChartNode->GetXAxisTitleVisibility());
  d->xAxisLabelLineEdit->setText(mrmlPlotChartNode->GetXAxisTitle() ? mrmlPlotChartNode->GetXAxisTitle() : "");

  d->showYAxisLabelCheckBox->setChecked(mrmlPlotChartNode->GetYAxisTitleVisibility());
  d->yAxisLabelLineEdit->setText(mrmlPlotChartNode->GetYAxisTitle() ? mrmlPlotChartNode->GetYAxisTitle() : "");

  // Show plot type and marker type if they are the same in all selected plot nodes.

  wasBlocked = d->plotTypeComboBox->blockSignals(true);
  std::string plotType;
  if (mrmlPlotChartNode->GetPropertyFromAllPlotDataNodes(vtkMRMLPlotChartNode::PlotType, plotType))
    {
    d->plotTypeComboBox->setCurrentIndex(d->plotTypeComboBox->findText(plotType.c_str()));
    }
  else
    {
    d->plotTypeComboBox->setCurrentIndex(-1);
    }
  d->plotTypeComboBox->blockSignals(wasBlocked);

  wasBlocked = d->markersComboBox->blockSignals(true);
  std::string markerStyle;
  if (mrmlPlotChartNode->GetPropertyFromAllPlotDataNodes(vtkMRMLPlotChartNode::PlotMarkerStyle, markerStyle))
    {
    d->markersComboBox->setCurrentIndex(d->markersComboBox->findText(markerStyle.c_str()));
    }
  else
    {
    d->markersComboBox->setCurrentIndex(-1);
    }
  d->markersComboBox->blockSignals(wasBlocked);
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if (this->mrmlScene() == newScene)
    {
    return;
    }

   d->qvtkReconnect(this->mrmlScene(), newScene, vtkMRMLScene::EndBatchProcessEvent,
                    this, SLOT(updateWidgetFromMRML()));

  // Disable the node selectors as they would fire signal currentIndexChanged(0)
  // meaning that there is no current node anymore. It's not true, it just means
  // that the current node was not in the combo box list menu before.
  bool plotChartBlockSignals = d->plotChartComboBox->blockSignals(true);
  bool plotBlockSignals = d->plotDataComboBox->blockSignals(true);

  this->Superclass::setMRMLScene(newScene);

  d->plotChartComboBox->blockSignals(plotChartBlockSignals);
  d->plotDataComboBox->blockSignals(plotBlockSignals);

  if (this->mrmlScene())
    {
    this->updateWidgetFromMRML();
    }
}
