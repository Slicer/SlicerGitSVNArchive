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

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyChartsPlugin.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLChartViewNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QStandardItem>
#include <QAction>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Widgets
class qSlicerSubjectHierarchyChartsPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyChartsPlugin);
protected:
  qSlicerSubjectHierarchyChartsPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyChartsPluginPrivate(qSlicerSubjectHierarchyChartsPlugin& object);
  ~qSlicerSubjectHierarchyChartsPluginPrivate();
  void init();
public:
  QIcon ChartIcon;

  QIcon VisibleIcon;
  QIcon HiddenIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyChartsPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyChartsPluginPrivate::qSlicerSubjectHierarchyChartsPluginPrivate(qSlicerSubjectHierarchyChartsPlugin& object)
: q_ptr(&object)
{
  this->ChartIcon = QIcon(":Icons/Chart.png");

  this->VisibleIcon = QIcon(":Icons/VisibleOn.png");
  this->HiddenIcon = QIcon(":Icons/VisibleOff.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyChartsPluginPrivate::~qSlicerSubjectHierarchyChartsPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyChartsPlugin::qSlicerSubjectHierarchyChartsPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyChartsPluginPrivate(*this) )
{
  this->m_Name = QString("Charts");

  Q_D(qSlicerSubjectHierarchyChartsPlugin);
  d->init();
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyChartsPluginPrivate::init()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyChartsPlugin::~qSlicerSubjectHierarchyChartsPlugin()
{
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyChartsPlugin::canAddNodeToSubjectHierarchy(
  vtkMRMLNode* node, SubjectHierarchyItemID parentItemID/*=vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID*/)const
{
  Q_UNUSED(parentItemID);
  if (!node)
    {
    qCritical() << Q_FUNC_INFO << ": Input node is NULL!";
    return 0.0;
    }
  else if (node->IsA("vtkMRMLChartNode"))
    {
    // Node is a chart
    return 0.5;
    }

  return 0.0;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyChartsPlugin::canOwnSubjectHierarchyItem(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return 0.0;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return 0.0;
    }

  // Chart
  vtkMRMLNode* associatedNode = shNode->GetItemDataNode(itemID);
  if (associatedNode && associatedNode->IsA("vtkMRMLChartNode"))
    {
    return 0.5; // There may be other plugins that can handle special charts better
    }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyChartsPlugin::roleForPlugin()const
{
  return "Chart";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyChartsPlugin::icon(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QIcon();
    }

  Q_D(qSlicerSubjectHierarchyChartsPlugin);

  if (this->canOwnSubjectHierarchyItem(itemID))
    {
    return d->ChartIcon;
    }

  // Item unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyChartsPlugin::visibilityIcon(int visible)
{
  Q_D(qSlicerSubjectHierarchyChartsPlugin);

  if (visible)
    {
    return d->VisibleIcon;
    }
  else
    {
    return d->HiddenIcon;
    }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyChartsPlugin::setDisplayVisibility(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID, int visible)
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene!";
    return;
    }

  if (this->getDisplayVisibility(itemID) == visible)
    {
    return;
    }

  // Get layout node
  vtkSmartPointer<vtkCollection> layoutNodes =
    vtkSmartPointer<vtkCollection>::Take( scene->GetNodesByClass("vtkMRMLLayoutNode") );
  layoutNodes->InitTraversal();
  vtkObject* layoutNodeVtkObject = layoutNodes->GetNextItemAsObject();
  vtkMRMLLayoutNode* layoutNode = vtkMRMLLayoutNode::SafeDownCast(layoutNodeVtkObject);
  if (!layoutNode)
    {
    qCritical() << Q_FUNC_INFO << ": Unable to get layout node!";
    return;
    }

  vtkMRMLChartViewNode* chartViewNode = this->getChartViewNode();

  vtkMRMLChartNode* associatedChartNode = vtkMRMLChartNode::SafeDownCast(shNode->GetItemDataNode(itemID));
  if (associatedChartNode && visible)
    {
    // Switch to four-up quantitative layout
    layoutNode->SetViewArrangement( vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView );

    // Make sure we have a valid chart view node (if we want to show the chart, but there was
    // no chart view, then one was just created when we switched to quantitative layout)
    if (!chartViewNode)
      {
      chartViewNode = this->getChartViewNode();
      }

    // Hide currently shown chart and trigger icon update
    if ( chartViewNode->GetChartNodeID()
      && strcmp(chartViewNode->GetChartNodeID(), associatedChartNode->GetID()) )
      {
      chartViewNode->SetChartNodeID(NULL);
      associatedChartNode->Modified(); //TODO: Does this trigger SH item update? We need to do that
      }

    // Select chart to show
    chartViewNode->SetChartNodeID(associatedChartNode->GetID());
    }
  else if (chartViewNode)
    {
    // Hide chart
    chartViewNode->SetChartNodeID(NULL);
    }

  // Trigger icon update
  //node->Modified(); //TODO:
}

//-----------------------------------------------------------------------------
int qSlicerSubjectHierarchyChartsPlugin::getDisplayVisibility(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return -1;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return -1;
    }

  vtkMRMLChartViewNode* chartViewNode = this->getChartViewNode();
  if (!chartViewNode)
    {
    // No quantitative view has been set yet
    return 0;
    }

  // Return hidden if current layout is not one of the quantitative ones
  if ( qSlicerApplication::application()->layoutManager()->layout() != vtkMRMLLayoutNode::SlicerLayoutFourUpQuantitativeView
    && qSlicerApplication::application()->layoutManager()->layout() != vtkMRMLLayoutNode::SlicerLayoutOneUpQuantitativeView
    && qSlicerApplication::application()->layoutManager()->layout() != vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView
    && qSlicerApplication::application()->layoutManager()->layout() != vtkMRMLLayoutNode::SlicerLayoutThreeOverThreeQuantitativeView )
    {
    return 0;
    }

  // Return shown if chart in chart view is the examined item's associated data node
  vtkMRMLChartNode* associatedChartNode = vtkMRMLChartNode::SafeDownCast(shNode->GetItemDataNode(itemID));
  if ( associatedChartNode && chartViewNode->GetChartNodeID()
    && !strcmp(chartViewNode->GetChartNodeID(), associatedChartNode->GetID()) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyChartsPlugin::editProperties(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)
{
  Q_UNUSED(itemID);
  // No module to edit Charts, just switch layout
}

//---------------------------------------------------------------------------
vtkMRMLChartViewNode* qSlicerSubjectHierarchyChartsPlugin::getChartViewNode()const
{
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene!";
    return NULL;
    }

  vtkSmartPointer<vtkCollection> chartViewNodes =
    vtkSmartPointer<vtkCollection>::Take( scene->GetNodesByClass("vtkMRMLChartViewNode") );
  chartViewNodes->InitTraversal();
  vtkMRMLChartViewNode* chartViewNode = vtkMRMLChartViewNode::SafeDownCast( chartViewNodes->GetNextItemAsObject() );
  if (!chartViewNode)
    {
    qCritical() << Q_FUNC_INFO << ": Unable to get chart view node!";
    return NULL;
    }

  return chartViewNode;
}
