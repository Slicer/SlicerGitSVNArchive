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
  and was supported through the European Research Consil grant nr. 291531.

==============================================================================*/

// Qt includes
#include <QActionGroup>
#include <QDebug>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QHBoxLayout>

// VTK includes
#include <vtkCollection.h>
#include <vtkStringArray.h>

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
#include <vtkMRMLPlotNode.h>
#include <vtkMRMLPlotLayoutNode.h>
#include <vtkMRMLPlotViewNode.h>
#include <vtkMRMLSceneViewNode.h>
#include <vtkSmartPointer.h>

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
  this->PlotLayoutNode = NULL;
  this->PlotViewNode = NULL;
  this->PlotView = NULL;
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

  // Connect PlotLayout selector
  this->connect(this->plotLayoutComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
                SLOT(onPlotLayoutNodeSelected(vtkMRMLNode*)));

  // Connect Plot selector
  this->connect(this->plotComboBox, SIGNAL(checkedNodesChanged()),
                SLOT(onPlotNodesSelected()));

  // Connect the Plot Type selector
  this->connect(this->plotTypeComboBox, SIGNAL(activated(const QString&)), SLOT(onPlotTypeSelected(const QString&)));

  // Connect the actions
  QObject::connect(this->actionShow_Grid, SIGNAL(toggled(bool)),
                   q, SLOT(showGrid(bool)));
  QObject::connect(this->actionShow_Legend, SIGNAL(toggled(bool)),
                   q, SLOT(showLegend(bool)));

  // Connect the buttons
  this->showGridToolButton->setDefaultAction(this->actionShow_Grid);
  this->showLegendToolButton->setDefaultAction(this->actionShow_Legend);

  // Connect the checkboxes
  QObject::connect(this->showTitleCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(showTitle(bool)));
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
                   this->plotComboBox, SLOT(setMRMLScene(vtkMRMLScene*)));

  QObject::connect(q, SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
                   this->plotLayoutComboBox, SLOT(setMRMLScene(vtkMRMLScene*)));
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::init()
{
  this->Superclass::init();
  this->ViewLabel->setText(qMRMLPlotViewControllerWidget::tr("P"));
  this->BarLayout->addStretch(1);
  this->setColor(QColor(27, 198, 207));
}


// --------------------------------------------------------------------------
vtkMRMLPlotLayoutNode* qMRMLPlotViewControllerWidgetPrivate::GetPlotLayoutNodeFromView()
{
  Q_Q(qMRMLPlotViewControllerWidget);

  if (!this->PlotViewNode || !q->mrmlScene())
    {
    // qDebug() << "No PlotViewNode or no Scene";
    return 0;
    }

  // Get the current PlotLayout node
  vtkMRMLPlotLayoutNode *PlotLayoutNodeFromViewNode
    = vtkMRMLPlotLayoutNode::SafeDownCast(q->mrmlScene()->GetNodeByID(this->PlotViewNode->GetPlotLayoutNodeID()));

  return PlotLayoutNodeFromViewNode;
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onPlotLayoutNodeSelected(vtkMRMLNode * node)
{
  Q_Q(qMRMLPlotViewControllerWidget);

  vtkMRMLPlotLayoutNode *mrmlPlotLayoutNode = vtkMRMLPlotLayoutNode::SafeDownCast(node);

  if (!this->PlotViewNode || this->PlotLayoutNode == mrmlPlotLayoutNode)
    {
    return;
    }

  this->qvtkReconnect(this->PlotLayoutNode, mrmlPlotLayoutNode, vtkCommand::ModifiedEvent,
                      q, SLOT(updateWidgetFromMRML()));

  this->PlotLayoutNode = mrmlPlotLayoutNode;

  this->PlotViewNode->SetAndUpdatePlotLayoutNodeID(mrmlPlotLayoutNode ? mrmlPlotLayoutNode->GetID() : NULL);

  q->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onPlotNodesSelected()
{
  Q_Q(qMRMLPlotViewControllerWidget);

  if (!this->PlotViewNode)
    {
    return;
    }

  if (!this->PlotLayoutNode)
    {
    return;
    }

  std::vector<std::string> plotNodesIDs;
  this->PlotLayoutNode->GetPlotIDs(plotNodesIDs);
  std::vector<std::string> plotNodesNames;
  this->PlotLayoutNode->GetPlotNames(plotNodesNames);

  // loop over arrays in the widget
  for (int idx = 0; idx < this->plotComboBox->nodeCount(); idx++)
    {
    vtkMRMLPlotNode *dn = vtkMRMLPlotNode::SafeDownCast(this->plotComboBox->nodeFromIndex(idx));

    bool checked = (this->plotComboBox->checkState(dn) == Qt::Checked);

    // is the node in the Plot?
    bool found = false;
    for (unsigned int ii = 0; ii < plotNodesIDs.size(); ii++)
      {
      if (!strcmp(dn->GetID(), plotNodesIDs[ii].c_str()))
        {
        if (!checked)
          {
          // plot is not checked but currently in the LayoutPlot, remove it
          // (might want to cache the old name in case user adds it back)
          this->PlotLayoutNode->RemovePlotAndObservationByName(plotNodesNames[ii].c_str());
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
        // (need a string for the name).
        this->PlotLayoutNode->AddAndObservePlot(dn->GetName(), dn->GetID());
        }
      }
    }
}

// --------------------------------------------------------------------------
void qMRMLPlotViewControllerWidgetPrivate::onPlotTypeSelected(const QString &Type)
{
  Q_Q(qMRMLPlotViewControllerWidget);
  if (!this->PlotLayoutNode || !q->mrmlScene())
    {
    return;
    }

  this->PlotLayoutNode->SetProperty("type", Type.toStdString().c_str());

  std::vector<std::string> plotNodesIDs;
  this->PlotLayoutNode->GetPlotIDs(plotNodesIDs);

  for (unsigned int plotIndex = 0; plotIndex < plotNodesIDs.size(); plotIndex++)
    {
    vtkMRMLPlotNode* plotNode = vtkMRMLPlotNode::SafeDownCast
      (q->mrmlScene()->GetNodeByID(plotNodesIDs[plotIndex]));
    if (!plotNode)
      {
      continue;
      }

    std::string namePlotNode = plotNode->GetName();
    std::size_t found = namePlotNode.find("Markups");
    if (found != std::string::npos)
      {
      this->PlotLayoutNode->RemovePlotAndObservationByName(namePlotNode.c_str());
      continue;
      }

    if (!Type.compare("Line"))
      {
      plotNode->SetType(vtkMRMLPlotNode::LINE);
      }
    else if (!Type.compare("Scatter"))
      {
      plotNode->SetType(vtkMRMLPlotNode::POINTS);
      }
    else if (!Type.compare("Line and Scatter"))
      {
      plotNode->SetType(vtkMRMLPlotNode::LINE);

      vtkSmartPointer<vtkMRMLPlotNode> plotNodeCopy = vtkMRMLPlotNode::SafeDownCast
        (plotNode->GetNodeReference("Markups"));

      if (plotNodeCopy)
        {
        plotNodeCopy->SetType(vtkMRMLPlotNode::POINTS);
        }
      else
        {
        vtkSmartPointer<vtkMRMLNode> node = vtkSmartPointer<vtkMRMLNode>::Take
          (q->mrmlScene()->CreateNodeByClass("vtkMRMLPlotNode"));
        plotNodeCopy = vtkMRMLPlotNode::SafeDownCast(node);

        std::string namePlotNodeCopy = namePlotNode + " Markups";

        plotNodeCopy->CopyAndSetNameAndType(plotNode, namePlotNodeCopy.c_str(), vtkMRMLPlotNode::POINTS);
        q->mrmlScene()->AddNode(plotNodeCopy);
        plotNode->AddNodeReferenceID("Markups", plotNodeCopy->GetID());
        plotNodeCopy->AddNodeReferenceID("Markups", plotNode->GetID());
        }

      this->PlotLayoutNode->AddAndObservePlot(plotNodeCopy->GetName(), plotNodeCopy->GetID());
      }
    else if (!Type.compare("Bar"))
      {
      plotNode->SetType(vtkMRMLPlotNode::BAR);
      }
    }

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
void qMRMLPlotViewControllerWidget::showGrid(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if(!d->PlotLayoutNode)
    {
    return;
    }

  if (show)
    {
    d->PlotLayoutNode->SetProperty("showGrid", "on");
    }
  else
    {
    d->PlotLayoutNode->SetProperty("showGrid", "off");
    }
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::showLegend(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if(!d->PlotLayoutNode)
    {
    return;
    }

  if (show)
    {
    d->PlotLayoutNode->SetProperty("showLegend", "on");
    }
  else
    {
    d->PlotLayoutNode->SetProperty("showLegend", "off");
    }
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::showTitle(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if(!d->PlotLayoutNode)
    {
    return;
    }

  if (show)
    {
    d->PlotLayoutNode->SetProperty("showTitle", "on");
    }
  else
    {
    d->PlotLayoutNode->SetProperty("showTitle", "off");
    }
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::showXAxisLabel(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if(!d->PlotLayoutNode)
    {
    return;
    }

  if (show)
    {
    d->PlotLayoutNode->SetProperty("showXAxisLabel", "on");
    }
  else
    {
    d->PlotLayoutNode->SetProperty("showXAxisLabel", "off");
    }
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::showYAxisLabel(bool show)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if(!d->PlotLayoutNode)
    {
    return;
    }

  if (show)
    {
    d->PlotLayoutNode->SetProperty("showYAxisLabel", "on");
    }
  else
    {
    d->PlotLayoutNode->SetProperty("showYAxisLabel", "off");
    }
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setTitle(const QString &str)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if(!d->PlotLayoutNode)
    {
    return;
    }

  d->PlotLayoutNode->SetProperty("TitleName", str.toLatin1());
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setXAxisLabel(const QString &str)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if(!d->PlotLayoutNode)
    {
    return;
    }

  d->PlotLayoutNode->SetProperty("XAxisLabelName", str.toLatin1());
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::setYAxisLabel(const QString &str)
{
  Q_D(qMRMLPlotViewControllerWidget);

  if(!d->PlotLayoutNode)
    {
    return;
    }

  d->PlotLayoutNode->SetProperty("YAxisLabelName", str.toLatin1());
}

//---------------------------------------------------------------------------
void qMRMLPlotViewControllerWidget::editTitle()
{
  Q_D(qMRMLPlotViewControllerWidget);

  if (!d->PlotLayoutNode)
    {
    return;
    }

  // Bring up a dialog to request a title
  bool ok = false;
  QString newTitle = QInputDialog::getText(
    this, "Edit Title", "Title",
    QLineEdit::Normal, d->PlotLayoutNode->GetProperty("TitleName"), &ok);
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

  if (!d->PlotLayoutNode)
    {
    return;
    }

  // Bring up a dialog to request a title
  bool ok = false;
  QString newXLabel = QInputDialog::getText(
    this, "Edit X-axis label", "X-axis label",
    QLineEdit::Normal, d->PlotLayoutNode->GetProperty("XAxisLabelName"), &ok);
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

  if (!d->PlotLayoutNode)
    {
    return;
    }

  // Bring up a dialog to request a title
  bool ok = false;
  QString newYLabel = QInputDialog::getText(
    this, "Edit Y-axis label", "Y-axis label",
    QLineEdit::Normal, d->PlotLayoutNode->GetProperty("YAxisLabelName"), &ok);
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

  //qDebug() << "qMRMLPlotViewControllerWidget::updateWidgetFromMRML()";

  if (!d->PlotViewNode || !this->mrmlScene())
    {
    return;
    }

  vtkMRMLPlotLayoutNode* mrmlPlotLayoutNode = d->GetPlotLayoutNodeFromView();
  // PlotLayoutNode selector
  d->plotLayoutComboBox->setCurrentNode(mrmlPlotLayoutNode);

  if (!mrmlPlotLayoutNode)
    {
    // Set the widgets to default states
    int tindex = d->plotTypeComboBox->findText(QString("Line"));
    d->plotTypeComboBox->setCurrentIndex(tindex);
    d->actionShow_Grid->setChecked(true);
    d->actionShow_Legend->setChecked(true);
    d->showTitleCheckBox->setChecked(true);
    d->showXAxisLabelCheckBox->setChecked(true);
    d->showYAxisLabelCheckBox->setChecked(true);
    d->titleLineEdit->setText("");
    d->xAxisLabelLineEdit->setText("");
    d->yAxisLabelLineEdit->setText("");

    bool plotBlockSignals = d->plotComboBox->blockSignals(true);
    for (int idx = 0; idx < d->plotComboBox->nodeCount(); idx++)
      {
      d->plotComboBox->setCheckState(d->plotComboBox->nodeFromIndex(idx),
                                      Qt::Unchecked);
      }
    d->plotComboBox->blockSignals(plotBlockSignals);

    return;
    }

  // Plot selector
  vtkStringArray *plotIDs = mrmlPlotLayoutNode->GetPlotIDs();
  bool plotBlockSignals = d->plotComboBox->blockSignals(true);
  for (int idx = 0; idx < d->plotComboBox->nodeCount(); idx++)
    {
    d->plotComboBox->setCheckState(d->plotComboBox->nodeFromIndex(idx),
                                    Qt::Unchecked);
    }
  for (int idx = 0; idx < plotIDs->GetNumberOfValues(); idx++)
    {
    vtkMRMLPlotNode *dn = vtkMRMLPlotNode::SafeDownCast
      (this->mrmlScene()->GetNodeByID(plotIDs->GetValue(idx).c_str()));
    if (dn)
      {
      d->plotComboBox->setCheckState(dn, Qt::Checked);
      }
    }
  d->plotComboBox->blockSignals(plotBlockSignals);

  const char *propertyValue;
  propertyValue = mrmlPlotLayoutNode->GetProperty("showGrid");
  d->actionShow_Grid->setChecked(propertyValue && !strcmp("on", propertyValue));

  propertyValue = mrmlPlotLayoutNode->GetProperty("showLegend");
  d->actionShow_Legend->setChecked(propertyValue && !strcmp("on", propertyValue));

  // Titles, axis labels (checkboxes AND text widgets)
  propertyValue = mrmlPlotLayoutNode->GetProperty("showTitle");
  d->showTitleCheckBox->setChecked(propertyValue && !strcmp("on", propertyValue));
  propertyValue = mrmlPlotLayoutNode->GetProperty("TitleName");
  if (propertyValue)
    {
    d->titleLineEdit->setText(propertyValue);
    }
  else
    {
    d->titleLineEdit->clear();
    }

  propertyValue = mrmlPlotLayoutNode->GetProperty("showXAxisLabel");
  d->showXAxisLabelCheckBox->setChecked(propertyValue && !strcmp("on", propertyValue));
  propertyValue = mrmlPlotLayoutNode->GetProperty("XAxisLabelName");
  if (propertyValue)
    {
    d->xAxisLabelLineEdit->setText(propertyValue);
    }
  else
    {
    d->xAxisLabelLineEdit->clear();
    }

  propertyValue = mrmlPlotLayoutNode->GetProperty("showYAxisLabel");
  d->showYAxisLabelCheckBox->setChecked(propertyValue && !strcmp("on", propertyValue));
  propertyValue = mrmlPlotLayoutNode->GetProperty("YAxisLabelName");
  if (propertyValue)
    {
    d->yAxisLabelLineEdit->setText(propertyValue);
    }
  else
    {
    d->yAxisLabelLineEdit->clear();
    }

  // PlotType selector
  const char *type;
  std::string stype("Line");
  type = mrmlPlotLayoutNode->GetProperty("type");
  if (!type)
    {
    // no type specified, default to "Line"
    type = stype.c_str();
    }
  if (type)
    {
    QString qtype(type);
    int tindex = d->plotTypeComboBox->findText(qtype);
    if (tindex != -1)
      {
      d->plotTypeComboBox->setCurrentIndex(tindex);
      }
    }
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
  // that the current node was not in the combo box list menu before
  bool plotLayoutBlockSignals = d->plotLayoutComboBox->blockSignals(true);
  bool plotBlockSignals = d->plotComboBox->blockSignals(true);

  this->Superclass::setMRMLScene(newScene);

  d->plotLayoutComboBox->blockSignals(plotLayoutBlockSignals);
  d->plotComboBox->blockSignals(plotBlockSignals);

  if (this->mrmlScene())
    {
    this->updateWidgetFromMRML();
    }
}
