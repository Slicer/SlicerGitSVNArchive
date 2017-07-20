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
#include <QDebug>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QToolButton>
#include <QWebFrame>

// STD includes
#include <vector>
#include <algorithm>

// CTK includes
#include <ctkAxesWidget.h>
#include <ctkLogger.h>
#include <ctkPopupWidget.h>

// qMRML includes
#include "qMRMLColors.h"
#include "qMRMLPlotView_p.h"

// MRML includes
#include <vtkMRMLPlotNode.h>
#include <vtkMRMLPlotLayoutNode.h>
#include <vtkMRMLPlotViewNode.h>
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTableNode.h>

// VTK includes
#include <vtkAxis.h>
#include <vtkBrush.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkCollection.h>
#include <vtkColorSeries.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkNew.h>
#include <vtkPlot.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkStringArray.h>
#include <vtkTextProperty.h>

//--------------------------------------------------------------------------
// qMRMLPlotViewPrivate methods

//---------------------------------------------------------------------------
qMRMLPlotViewPrivate::qMRMLPlotViewPrivate(qMRMLPlotView& object)
  : q_ptr(&object)
{
  this->MRMLScene = 0;
  this->MRMLPlotViewNode = 0;
  this->MRMLPlotLayoutNode = 0;
  this->ColorLogic = 0;
  this->PinButton = 0;
  this->PopupWidget = 0;
}

//---------------------------------------------------------------------------
qMRMLPlotViewPrivate::~qMRMLPlotViewPrivate()
{
}

namespace
{
//----------------------------------------------------------------------------
template <typename T> T StringToNumber(const char* num)
{
  std::stringstream ss;
  ss << num;
  T result;
  return ss >> result ? result : 0;
}

//----------------------------------------------------------------------------
int StringToInt(const char* str)
{
  return StringToNumber<int>(str);
}

}// end namespace

//---------------------------------------------------------------------------
void qMRMLPlotViewPrivate::init()
{
  Q_Q(qMRMLPlotView);

  // Let the QWebView expand in both directions
  q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  this->PopupWidget = new ctkPopupWidget;
  QHBoxLayout* popupLayout = new QHBoxLayout;
  popupLayout->addWidget(new QToolButton);
  this->PopupWidget->setLayout(popupLayout);

  if (!q->chart())
    {
    return;
    }

  q->chart()->SetActionToButton(vtkChart::CLICKANDDRAG, vtkContextMouseEvent::LEFT_BUTTON);
  q->chart()->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::RIGHT_BUTTON);

  qvtkConnect(q->chart(), vtkCommand::SelectionChangedEvent, this, SLOT(emitSelection()));

  if (!q->chart()->GetBackgroundBrush() ||
      !q->chart()->GetTitleProperties() ||
      !q->chart()->GetLegend()          ||
      !q->scene())
    {
    return;
    }

  if (!q->chart()->GetLegend()->GetLabelProperties() ||
      !q->scene()->GetRenderer())
    {
    return;
    }

  vtkColor4ub color;
  color.Set(255., 253., 246., 255.);
  q->chart()->GetBackgroundBrush()->SetColor(color);
  q->chart()->GetTitleProperties()->SetFontFamilyToArial();
  q->chart()->GetTitleProperties()->SetFontSize(20);
  q->chart()->GetLegend()->GetLabelProperties()->SetFontFamilyToArial();
  q->scene()->GetRenderer()->SetUseDepthPeeling(true);
  q->scene()->GetRenderer()->SetUseFXAA(true);

  vtkAxis* axis = q->chart()->GetAxis(vtkAxis::LEFT);
  if (axis)
    {
    axis->GetTitleProperties()->SetFontFamilyToArial();
    axis->GetTitleProperties()->SetFontSize(16);
    axis->GetTitleProperties()->SetBold(false);
    axis->GetLabelProperties()->SetFontFamilyToArial();
    axis->GetLabelProperties()->SetFontSize(12);
    }
  axis = q->chart()->GetAxis(vtkAxis::BOTTOM);
  if (axis)
    {
    axis->GetTitleProperties()->SetFontFamilyToArial();
    axis->GetTitleProperties()->SetFontSize(16);
    axis->GetTitleProperties()->SetBold(false);
    axis->GetLabelProperties()->SetFontFamilyToArial();
    axis->GetLabelProperties()->SetFontSize(12);
    }
  axis = q->chart()->GetAxis(vtkAxis::RIGHT);
  if (axis)
    {
    axis->GetTitleProperties()->SetFontFamilyToArial();
    axis->GetTitleProperties()->SetFontSize(16);
    axis->GetTitleProperties()->SetBold(false);
    axis->GetLabelProperties()->SetFontFamilyToArial();
    axis->GetLabelProperties()->SetFontSize(12);
    }
  axis = q->chart()->GetAxis(vtkAxis::TOP);
  if (axis)
    {
    axis->GetTitleProperties()->SetFontFamilyToArial();
    axis->GetTitleProperties()->SetFontSize(16);
    axis->GetTitleProperties()->SetBold(false);
    axis->GetLabelProperties()->SetFontFamilyToArial();
    axis->GetLabelProperties()->SetFontSize(12);
    }

}

//---------------------------------------------------------------------------
void qMRMLPlotViewPrivate::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_Q(qMRMLPlotView);
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
void qMRMLPlotViewPrivate::startProcessing()
{
}

//
// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::endProcessing()
{
  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
vtkMRMLScene* qMRMLPlotViewPrivate::mrmlScene()
{
  return this->MRMLScene;
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::onPlotLayoutNodeChanged()
{
  Q_Q(qMRMLPlotView);
  vtkMRMLPlotLayoutNode *newPlotLayoutNode = NULL;

  if (this->MRMLPlotViewNode && this->MRMLPlotViewNode->GetPlotLayoutNodeID())
    {
    newPlotLayoutNode = vtkMRMLPlotLayoutNode::SafeDownCast
      (this->MRMLScene->GetNodeByID(this->MRMLPlotViewNode->GetPlotLayoutNodeID()));
    }

  this->qvtkReconnect(
    this->MRMLPlotLayoutNode, newPlotLayoutNode,
    vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  this->qvtkReconnect(
    this->MRMLPlotLayoutNode, newPlotLayoutNode,
    vtkMRMLPlotLayoutNode::vtkPlotRemovedEvent, this, SLOT(cleanPlotsInView()));

  this->MRMLPlotLayoutNode = newPlotLayoutNode;
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::mouseDoubleClickEventInternalUpdate()
{
  Q_Q(qMRMLPlotView);

  if (!q->chart())
    {
    return;
    }

  q->chart()->RecalculateBounds();
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::emitSelection()
{
  Q_Q(qMRMLPlotView);

  if (!q->chart())
    {
    return;
    }

  char *PlotLayoutNodeID = this->MRMLPlotViewNode->GetPlotLayoutNodeID();

  vtkMRMLPlotLayoutNode* pln = vtkMRMLPlotLayoutNode::SafeDownCast
    (this->MRMLScene->GetNodeByID(PlotLayoutNodeID));
  if (!pln)
    {
    return;
    }

  vtkNew<vtkStringArray> mrmlPlotIDs;
  vtkNew<vtkCollection> selectionCol;

  for (int plotIndex = 0; plotIndex < q->chart()->GetNumberOfPlots(); plotIndex++)
    {
    vtkPlot *Plot = q->chart()->GetPlot(plotIndex);
    if (!Plot)
      {
      continue;
      }
    vtkIdTypeArray *Selection = Plot->GetSelection();
    if (!Selection)
      {
      continue;
      }

    if (Selection->GetNumberOfValues() > 0)
      {
      selectionCol->AddItem(Selection);
      // get MRMLPlotNode from pln and find the Node with the same vtkPlot address
      vtkCollection* plotNodes = pln->GetPlotNodes();
      for (int plotNodeIndex = 0; plotNodeIndex < plotNodes->GetNumberOfItems(); plotNodeIndex++)
        {
        vtkMRMLPlotNode *PlotNode = vtkMRMLPlotNode::SafeDownCast
          (plotNodes->GetItemAsObject(plotNodeIndex));
        if (!PlotNode)
          {
          continue;
          }
        if (PlotNode->GetPlot() == Plot)
          {
          mrmlPlotIDs->InsertNextValue(PlotNode->GetID());
          break;
          }
        }
      }
    }
  // emit the signal
  emit q->dataSelected(mrmlPlotIDs.GetPointer(), selectionCol.GetPointer());
  // save them in the PlotLayoutNode as well
  pln->SetSelection(mrmlPlotIDs.GetPointer(), selectionCol.GetPointer());
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::updateWidgetFromMRML()
{
  Q_Q(qMRMLPlotView);

  if (!q->isEnabled())
    {
    return;
    }

  if (!this->MRMLScene || !this->ColorLogic || !this->MRMLPlotViewNode || !q->chart())
    {
    return;
    }

  if (!q->chart()->GetLegend())
    {
    return;
    }

  // Get the PlotLayoutNode
  char *PlotLayoutNodeID = this->MRMLPlotViewNode->GetPlotLayoutNodeID();

  vtkMRMLPlotLayoutNode* pln = vtkMRMLPlotLayoutNode::SafeDownCast
    (this->MRMLScene->GetNodeByID(PlotLayoutNodeID));
  if (!pln)
    {
    // Clean all the plots in vtkChartXY
    while(q->chart()->GetNumberOfPlots() > 0)
      {
      // This if is necessary for a BUG at VTK level:
      // in the case of a plot removed with corner ID 0,
      // when successively the addPlot method is called
      // (to add the same plot instance to vtkChartXY) it will
      // fail to setup the graph in the vtkChartXY render.
      if (q->chart()->GetPlotCorner(q->chart()->GetPlot(0)) == 0)
        {
        q->chart()->SetPlotCorner(q->chart()->GetPlot(0), 1);
        }
      q->removePlot(q->chart()->GetPlot(0));
      }
    return;
    }

  const char* defaultPlotColorNodeID = this->ColorLogic->GetDefaultPlotColorNodeID();
  vtkMRMLColorNode *defaultColorNode = vtkMRMLColorNode::SafeDownCast
    (this->MRMLScene->GetNodeByID(defaultPlotColorNodeID));
  vtkMRMLColorNode *colorNode = vtkMRMLColorNode::SafeDownCast
    (this->MRMLScene->GetNodeByID(pln->GetProperty("lookupTable")));

  if (!colorNode)
    {
    colorNode = defaultColorNode;
    }

  if (!colorNode)
    {
    return;
    }

  vtkSmartPointer<vtkCollection> plotNodes = vtkSmartPointer<vtkCollection>::Take
    (this->mrmlScene()->GetNodesByClass("vtkMRMLPlotNode"));

  std::vector<std::string> plotNodesIDs;
  pln->GetPlotIDs(plotNodesIDs);

  for(int plotNodesIndex = 0; plotNodesIndex < plotNodes->GetNumberOfItems(); plotNodesIndex++)
    {
    vtkMRMLPlotNode* plotNode = vtkMRMLPlotNode::SafeDownCast
      (plotNodes->GetItemAsObject(plotNodesIndex));
    if (!plotNode)
      {
      continue;
      }

    bool plotFound = FALSE;
    for (unsigned int PlotNodeIDsIndex = 0; PlotNodeIDsIndex < plotNodesIDs.size(); PlotNodeIDsIndex++)
      {
      if (plotNodesIDs[PlotNodeIDsIndex].compare(plotNode->GetID()))
        {
        continue;
        }
      vtkPlot* plot = plotNode->GetPlot();
      if (!plot)
        {
        continue;
        }

      // Update Selection from PlotLayoutNode
      vtkStringArray *selectedPlotNodeIDs = pln->GetSelectionPlotNodeIDs();
      vtkCollection *selectedArrays = pln->GetSelectionArrays();

      if (selectedPlotNodeIDs && selectedArrays)
        {
        for (int selectedPlotNodesIndex = 0; selectedPlotNodesIndex < selectedPlotNodeIDs->GetNumberOfValues(); selectedPlotNodesIndex++)
          {
          vtkMRMLPlotNode* selectedPlotNode = vtkMRMLPlotNode::SafeDownCast
            (this->mrmlScene()->GetNodeByID(selectedPlotNodeIDs->GetValue(selectedPlotNodesIndex)));
          if (!selectedPlotNode)
            {
            continue;
            }

          std::string namePlotNode = selectedPlotNode->GetName();
          std::size_t found = namePlotNode.find("Markups");
          if (found != std::string::npos)
            {
            continue;
            }

          if (selectedPlotNode->GetPlot() != plot)
            {
            continue;
            }

          vtkIdTypeArray* selectedArray = vtkIdTypeArray::SafeDownCast
            (selectedArrays->GetItemAsObject(selectedPlotNodesIndex));
          if (!selectedArray)
            {
            continue;
            }

          plot->SetSelection(selectedArray);

          vtkMRMLPlotNode* MarkupsPlotNode = vtkMRMLPlotNode::SafeDownCast
            (plotNode->GetNodeReference("Markups"));

          if (MarkupsPlotNode)
            {
            if (MarkupsPlotNode->GetPlot())
              {
              MarkupsPlotNode->GetPlot()->SetSelection(selectedArray);
              }
            }
          break;
          }
        }

      // Set color of the plot
      double color[4] = {0., 0., 0., 0.};
      // Get the index from PlotLayoutNode (it doesn't count for Plot "Markups" Nodes)
      vtkIdType plotIndex = pln->GetPlotNodeIndex(plotNode);
      colorNode->GetColor(plotIndex , color);
      plot->SetColor(color[0], color[1], color[2]);

      // Add plot if not already in chart
      if (q->plotIndex(plot) < vtkIdType(0))
        {
        q->addPlot(plot);
        }
      plotFound = TRUE;
      break;
      }

    bool columnXFound = FALSE;
    bool columnYFound = FALSE;
    for (int columnIndex = 0; columnIndex < plotNode->GetTableNode()->GetNumberOfColumns(); columnIndex++)
      {
      if(!strcmp(plotNode->GetTableNode()->GetColumnName(columnIndex).c_str(), plotNode->GetXColumnName().c_str()))
        {
        columnXFound = TRUE;
        }
      if(!strcmp(plotNode->GetTableNode()->GetColumnName(columnIndex).c_str(), plotNode->GetYColumnName().c_str()))
        {
        columnYFound = TRUE;
        }
      }

    if (!plotFound || !columnXFound || !columnYFound)
      {
      // This if is necessary for a BUG at VTK level:
      // in the case of a plot removed with corner ID 0,
      // when successively the addPlot method is called
      // (to add the same plot instance to vtkChartXY) it will
      // fail to setup the graph in the vtkChartXY render.
      if (q->chart()->GetPlotCorner(plotNode->GetPlot()) == 0)
        {
        q->chart()->SetPlotCorner(plotNode->GetPlot(), 1);
        }
      q->removePlot(plotNode->GetPlot());
      }
    }

  // Setting Title
  if (!strcmp(pln->GetProperty("showTitle"), "on"))
    {
    q->chart()->SetTitle(pln->GetProperty("TitleName"));
    }
  else
    {
    q->chart()->SetTitle("");
    }

  // Setting Legend
  if (!strcmp(pln->GetProperty("showLegend"), "on"))
    {
    q->chart()->SetShowLegend(true);
    }
  else
    {
    q->chart()->SetShowLegend(false);
    }

  // Setting Title and Legend Properties
  int VTKFont = q->chart()->GetTitleProperties()->GetFontFamilyFromString(pln->GetProperty("FontType"));

  q->chart()->GetTitleProperties()->SetFontFamily(VTKFont);
  q->chart()->GetTitleProperties()->SetFontSize(StringToInt(pln->GetProperty("TitleFontSize")));
  q->chart()->GetLegend()->GetLabelProperties()->SetFontFamily(VTKFont);

  // Setting Axes
  // Assuming the the Top and Bottom axes are the "X" axis
  vtkAxis *axis = q->chart()->GetAxis(vtkAxis::BOTTOM);
  if (axis)
    {
    if (!strcmp(pln->GetProperty("showXAxisLabel"), "on"))
      {
      axis->SetTitle(pln->GetProperty("XAxisLabelName"));
      }
    else
      {
      axis->SetTitle("");
      }

    if (!strcmp(pln->GetProperty("showGrid"), "on"))
      {
      axis->SetGridVisible(true);
      }
    else
      {
      axis->SetGridVisible(false);
      }

    axis->GetTitleProperties()->SetFontFamily(VTKFont);
    axis->GetTitleProperties()->SetFontSize(StringToInt(pln->GetProperty("AxisTitleFontSize")));
    axis->GetLabelProperties()->SetFontFamily(VTKFont);
    axis->GetLabelProperties()->SetFontSize(StringToInt(pln->GetProperty("AxisLabelFontSize")));
    }

  axis = q->chart()->GetAxis(vtkAxis::TOP);
  if (axis)
    {
    if (!strcmp(pln->GetProperty("showXAxisLabel"), "on"))
      {
      axis->SetTitle(pln->GetProperty("XAxisLabelName"));
      }
    else
      {
      axis->SetTitle("");
      }

    if (!strcmp(pln->GetProperty("showGrid"), "on"))
      {
      axis->SetGridVisible(true);
      }
    else
      {
      axis->SetGridVisible(false);
      }

    axis->GetTitleProperties()->SetFontFamily(VTKFont);
    axis->GetTitleProperties()->SetFontSize(StringToInt(pln->GetProperty("AxisTitleFontSize")));
    axis->GetLabelProperties()->SetFontFamily(VTKFont);
    axis->GetLabelProperties()->SetFontSize(StringToInt(pln->GetProperty("AxisLabelFontSize")));
    }

  // Assuming the Left and Right axis are the "Y" axis
  axis = q->chart()->GetAxis(vtkAxis::LEFT);
  if (axis)
    {
    if (!strcmp(pln->GetProperty("showYAxisLabel"), "on"))
      {
      axis->SetTitle(pln->GetProperty("YAxisLabelName"));
      }
    else
      {
      axis->SetTitle("");
      }

    if (!strcmp(pln->GetProperty("showGrid"), "on"))
      {
      axis->SetGridVisible(true);
      }
    else
      {
      axis->SetGridVisible(false);
      }

    axis->GetTitleProperties()->SetFontFamily(VTKFont);
    axis->GetTitleProperties()->SetFontSize(StringToInt(pln->GetProperty("AxisTitleFontSize")));
    axis->GetLabelProperties()->SetFontFamily(VTKFont);
    axis->GetLabelProperties()->SetFontSize(StringToInt(pln->GetProperty("AxisLabelFontSize")));
    }

  axis = q->chart()->GetAxis(vtkAxis::RIGHT);
  if (axis)
    {
    if (!strcmp(pln->GetProperty("showYAxisLabel"), "on"))
      {
      axis->SetTitle(pln->GetProperty("YAxisLabelName"));
      }
    else
      {
      axis->SetTitle("");
      }

    if (!strcmp(pln->GetProperty("showGrid"), "on"))
      {
      axis->SetGridVisible(true);
      }
    else
      {
      axis->SetGridVisible(false);
      }

    axis->GetTitleProperties()->SetFontFamily(VTKFont);
    axis->GetTitleProperties()->SetFontSize(StringToInt(pln->GetProperty("AxisTitleFontSize")));
    axis->GetLabelProperties()->SetFontFamily(VTKFont);
    axis->GetLabelProperties()->SetFontSize(StringToInt(pln->GetProperty("AxisLabelFontSize")));
    }

  // Repaint the chart scene
  q->scene()->SetDirty(true);
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::cleanPlotsInView()
{
  Q_Q(qMRMLPlotView);

  if (!q->isEnabled())
    {
    return;
    }

  if (!this->MRMLScene || !this->MRMLPlotLayoutNode || !q->chart())
    {
    return;
    }

  vtkStringArray* plotNodeIDs = this->MRMLPlotLayoutNode->GetPlotIDs();

  if (!plotNodeIDs)
    {
    return;
    }

  for (int plotIndex = 0; plotIndex < plotNodeIDs->GetNumberOfValues(); plotIndex++)
    {
    vtkMRMLPlotNode* plotNode = vtkMRMLPlotNode::SafeDownCast
      (this->MRMLScene->GetNodeByID(plotNodeIDs->GetValue(plotIndex)));
    if (!plotNode)
      {
      continue;
      }

    if (plotNode->GetDirty())
      {
      q->removePlot(plotNode->GetPlot());
      // Plot Node is removed and it will be readded
      // by updateWidgetFromMRML automatically.
      // This clean chartXY from the plot and it is needed
      // when the Type of vtkMRMLPlotNode is changed.
      plotNode->SetDirty(false);
      }
    }
}

// --------------------------------------------------------------------------
// qMRMLPlotView methods

// --------------------------------------------------------------------------
qMRMLPlotView::qMRMLPlotView(QWidget* _parent) : Superclass(_parent)
  , d_ptr(new qMRMLPlotViewPrivate(*this))
{
  Q_D(qMRMLPlotView);
  d->init();
}

// --------------------------------------------------------------------------
qMRMLPlotView::~qMRMLPlotView()
{
  this->setMRMLScene(0);
}


//------------------------------------------------------------------------------
void qMRMLPlotView::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLPlotView);
  if (newScene == d->MRMLScene)
    {
    return;
    }

  d->setMRMLScene(newScene);

  if (d->MRMLPlotViewNode && newScene != d->MRMLPlotViewNode->GetScene())
    {
    this->setMRMLPlotViewNode(0);
    }

  emit mrmlSceneChanged(newScene);
}

//---------------------------------------------------------------------------
void qMRMLPlotView::setMRMLPlotViewNode(vtkMRMLPlotViewNode* newPlotViewNode)
{
  Q_D(qMRMLPlotView);
  if (d->MRMLPlotViewNode == newPlotViewNode)
    {
    return;
    }

  // connect modified event on PlotViewNode to updating the widget
  d->qvtkReconnect(
    d->MRMLPlotViewNode, newPlotViewNode,
    vtkCommand::ModifiedEvent, d, SLOT(updateWidgetFromMRML()));

  // connect on PlotNodeChangedEvent (e.g. PlotView is looking at a
  // different PlotNode
  d->qvtkReconnect(
    d->MRMLPlotViewNode, newPlotViewNode,
    vtkMRMLPlotViewNode::PlotLayoutNodeChangedEvent, d, SLOT(onPlotLayoutNodeChanged()));

  // cache the PlotViewNode
  d->MRMLPlotViewNode = newPlotViewNode;

  // ... and connect modified event on the PlotViewNode's PlotLayoutNode
  // to update the widget
  d->onPlotLayoutNodeChanged();

  // make sure the gui is up to date
  d->updateWidgetFromMRML();
}

//---------------------------------------------------------------------------
vtkMRMLPlotViewNode* qMRMLPlotView::mrmlPlotViewNode()const
{
  Q_D(const qMRMLPlotView);
  return d->MRMLPlotViewNode;
}

//---------------------------------------------------------------------------
void qMRMLPlotView::setColorLogic(vtkMRMLColorLogic *colorLogic)
{
  Q_D(qMRMLPlotView);
  d->ColorLogic = colorLogic;
}

//---------------------------------------------------------------------------
vtkMRMLColorLogic *qMRMLPlotView::colorLogic() const
{
  Q_D(const qMRMLPlotView);
  return d->ColorLogic;
}

//---------------------------------------------------------------------------
vtkMRMLScene* qMRMLPlotView::mrmlScene()const
{
  Q_D(const qMRMLPlotView);
  return d->MRMLScene;
}

//---------------------------------------------------------------------------
QSize qMRMLPlotView::sizeHint()const
{
  // return a default size hint (invalid size)
  return QSize();
}

// --------------------------------------------------------------------------
void qMRMLPlotView::mouseDoubleClickEvent(QMouseEvent *event)
{
  Q_D(qMRMLPlotView);
  this->Superclass::mouseDoubleClickEvent(event);

  d->mouseDoubleClickEventInternalUpdate();
}

// --------------------------------------------------------------------------
void qMRMLPlotView::keyPressEvent(QKeyEvent *event)
{
  this->Superclass::keyPressEvent(event);

  if (!this->chart())
    {
    return;
    }

  if (event->key() == Qt::Key_S)
    {
    int buttonSelect, buttonSelectPoly;
    buttonSelect = this->chart()->GetActionToButton(vtkChart::SELECT);
    buttonSelectPoly = this->chart()->GetActionToButton(vtkChart::SELECT_POLYGON);
    if (buttonSelect < 0 && buttonSelectPoly > 0)
      {
      this->chart()->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::RIGHT_BUTTON);
      }
    else if (buttonSelectPoly < 0 && buttonSelect > 0)
      {
      this->chart()->SetActionToButton(vtkChart::SELECT_POLYGON, vtkContextMouseEvent::RIGHT_BUTTON);
      }
    else if (buttonSelectPoly < 0 && buttonSelect < 0)
      {
      this->chart()->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::RIGHT_BUTTON);
      }
    }
}
