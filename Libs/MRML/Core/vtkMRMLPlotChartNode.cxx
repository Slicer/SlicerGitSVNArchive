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

#include <sstream>
#include <map>
#include <string>

// VTK includes
#include <vtkCollection.h>
#include <vtkCommand.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlot.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

// MRML includes
#include "vtkMRMLPlotChartNode.h"
#include "vtkMRMLPlotDataNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTableNode.h"

const char* vtkMRMLPlotChartNode::PlotDataNodeReferenceRole = "plotData";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlotChartNode);

//----------------------------------------------------------------------------
vtkMRMLPlotChartNode::vtkMRMLPlotChartNode()
: Title(NULL)
, TitleFontSize(20)
, TitleVisibility(true)
, GridVisibility(true)
, LegendVisibility(true)
, XAxisTitle(NULL)
, XAxisTitleVisibility(true)
, YAxisTitle(NULL)
, YAxisTitleVisibility(true)
, AxisTitleFontSize(16)
, AxisLabelFontSize(12)
, FontType(NULL)
, ClickAndDragAlongX(true)
, ClickAndDragAlongY(true)
{
  this->HideFromEditors = 0;

  this->SetFontType("Arial");

  vtkNew<vtkIntArray>  events;
  events->InsertNextValue(vtkCommand::ModifiedEvent);
  events->InsertNextValue(vtkMRMLPlotChartNode::PlotModifiedEvent);
  events->InsertNextValue(vtkMRMLPlotDataNode::TableModifiedEvent);
  this->AddNodeReferenceRole(this->GetPlotDataNodeReferenceRole(), NULL, events.GetPointer());
}


//----------------------------------------------------------------------------
vtkMRMLPlotChartNode::~vtkMRMLPlotChartNode()
{
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotChartNode::GetPlotDataNodeReferenceRole()
{
  return vtkMRMLPlotChartNode::PlotDataNodeReferenceRole;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::OnNodeReferenceAdded(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceAdded(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotDataNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotChartNode::PlotModifiedEvent, reference->GetReferencedNode());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::OnNodeReferenceModified(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceModified(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotDataNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotChartNode::PlotModifiedEvent, reference->GetReferencedNode());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::OnNodeReferenceRemoved(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceRemoved(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotDataNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotChartNode::PlotModifiedEvent, reference->GetReferencedNode());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  vtkMRMLWriteXMLBeginMacro(of)
  vtkMRMLWriteXMLStringMacro(title, Title)
  vtkMRMLWriteXMLIntMacro(titleFontSize, TitleFontSize)
  vtkMRMLWriteXMLBooleanMacro(TitleVisibility, TitleVisibility)
  vtkMRMLWriteXMLBooleanMacro(gridVisibility, GridVisibility)
  vtkMRMLWriteXMLBooleanMacro(legendVisibility, LegendVisibility)
  vtkMRMLWriteXMLStringMacro(xAxisTitle, XAxisTitle)
  vtkMRMLWriteXMLBooleanMacro(xAxisTitleVisibility, XAxisTitleVisibility)
  vtkMRMLWriteXMLStringMacro(yAxisTitle, YAxisTitle)
  vtkMRMLWriteXMLBooleanMacro(yAxisTitleVisibility, YAxisTitleVisibility)
  vtkMRMLWriteXMLIntMacro(axisTitleFontSize, AxisTitleFontSize)
  vtkMRMLWriteXMLIntMacro(axisLabelFontSize, AxisLabelFontSize)
  vtkMRMLWriteXMLStringMacro(fontType, FontType)
  vtkMRMLWriteXMLBooleanMacro(clickAndDragAlongX, ClickAndDragAlongX)
  vtkMRMLWriteXMLBooleanMacro(clickAndDragAlongY, ClickAndDragAlongY)
  vtkMRMLWriteXMLEndMacro()
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  vtkMRMLNode::ReadXMLAttributes(atts);

  vtkMRMLReadXMLBeginMacro(atts)
  vtkMRMLReadXMLStringMacro(title, Title)
  vtkMRMLReadXMLIntMacro(titleFontSize, TitleFontSize)
  vtkMRMLReadXMLBooleanMacro(TitleVisibility, TitleVisibility)
  vtkMRMLReadXMLBooleanMacro(gridVisibility, GridVisibility)
  vtkMRMLReadXMLBooleanMacro(legendVisibility, LegendVisibility)
  vtkMRMLReadXMLStringMacro(xAxisTitle, XAxisTitle)
  vtkMRMLReadXMLBooleanMacro(xAxisTitleVisibility, XAxisTitleVisibility)
  vtkMRMLReadXMLStringMacro(yAxisTitle, YAxisTitle)
  vtkMRMLReadXMLBooleanMacro(yAxisTitleVisibility, YAxisTitleVisibility)
  vtkMRMLReadXMLIntMacro(axisTitleFontSize, AxisTitleFontSize)
  vtkMRMLReadXMLIntMacro(axisLabelFontSize, AxisLabelFontSize)
  vtkMRMLReadXMLStringMacro(fontType, FontType)
  vtkMRMLReadXMLBooleanMacro(clickAndDragAlongX, ClickAndDragAlongX)
  vtkMRMLReadXMLBooleanMacro(clickAndDragAlongY, ClickAndDragAlongY)
  vtkMRMLReadXMLEndMacro()

  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
void vtkMRMLPlotChartNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  vtkMRMLCopyBeginMacro(anode, vtkMRMLPlotChartNode)
  vtkMRMLCopyStringMacro(Title)
  vtkMRMLCopyIntMacro(TitleFontSize)
  vtkMRMLCopyBooleanMacro(TitleVisibility)
  vtkMRMLCopyBooleanMacro(GridVisibility)
  vtkMRMLCopyBooleanMacro(LegendVisibility)
  vtkMRMLCopyStringMacro(XAxisTitle)
  vtkMRMLCopyBooleanMacro(XAxisTitleVisibility)
  vtkMRMLCopyStringMacro(YAxisTitle)
  vtkMRMLCopyBooleanMacro(YAxisTitleVisibility)
  vtkMRMLCopyIntMacro(AxisTitleFontSize)
  vtkMRMLCopyIntMacro(AxisLabelFontSize)
  vtkMRMLCopyStringMacro(FontType)
  vtkMRMLCopyBooleanMacro(ClickAndDragAlongX)
  vtkMRMLCopyBooleanMacro(ClickAndDragAlongY)
  vtkMRMLCopyEndMacro()

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  vtkMRMLPrintBeginMacro(os, indent)
  vtkMRMLPrintStringMacro(Title)
  vtkMRMLPrintIntMacro(TitleFontSize)
  vtkMRMLPrintBooleanMacro(TitleVisibility)
  vtkMRMLPrintBooleanMacro(GridVisibility)
  vtkMRMLPrintBooleanMacro(LegendVisibility)
  vtkMRMLPrintStringMacro(XAxisTitle)
  vtkMRMLPrintBooleanMacro(XAxisTitleVisibility)
  vtkMRMLPrintStringMacro(YAxisTitle)
  vtkMRMLPrintBooleanMacro(YAxisTitleVisibility)
  vtkMRMLPrintIntMacro(AxisTitleFontSize)
  vtkMRMLPrintIntMacro(AxisLabelFontSize)
  vtkMRMLPrintStringMacro(FontType)
  vtkMRMLPrintBooleanMacro(ClickAndDragAlongX)
  vtkMRMLPrintBooleanMacro(ClickAndDragAlongY)
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::SetAndObservePlotDataNodeID(const char *plotDataNodeID)
{
  this->SetAndObserveNodeReferenceID(this->GetPlotDataNodeReferenceRole(), plotDataNodeID);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::AddAndObservePlotDataNodeID(const char *plotDataNodeID)
{
  this->AddAndObserveNodeReferenceID(this->GetPlotDataNodeReferenceRole(), plotDataNodeID);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::RemovePlotDataNodeID(const char *plotDataNodeID)
{
  if (!plotDataNodeID)
    {
    return;
    }

  this->RemoveNthPlotDataNodeID(this->GetPlotDataNodeIndexFromID(plotDataNodeID));
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::RemoveNthPlotDataNodeID(int n)
{
  this->RemoveNthNodeReferenceID(this->GetPlotDataNodeReferenceRole(), n);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::RemoveAllPlotDataNodeIDs()
{
  this->RemoveNodeReferenceIDs(this->GetPlotDataNodeReferenceRole());
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::SetAndObserveNthPlotDataNodeID(int n, const char *plotDataNodeID)
{
  this->SetAndObserveNthNodeReferenceID(this->GetPlotDataNodeReferenceRole(), n, plotDataNodeID);
}

//----------------------------------------------------------------------------
bool vtkMRMLPlotChartNode::HasPlotDataNodeID(const char* plotDataNodeID)
{
  return this->HasNodeReferenceID(this->GetPlotDataNodeReferenceRole(), plotDataNodeID);
}

//----------------------------------------------------------------------------
int vtkMRMLPlotChartNode::GetNumberOfPlotDataNodes()
{
  return this->GetNumberOfNodeReferences(this->GetPlotDataNodeReferenceRole());
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotChartNode::GetNthPlotDataNodeID(int n)
{
    return this->GetNthNodeReferenceID(this->GetPlotDataNodeReferenceRole(), n);
}

//----------------------------------------------------------------------------
int vtkMRMLPlotChartNode::GetPlotDataNodeIndexFromID(const char *plotDataNodeID)
{
  if (!plotDataNodeID)
    {
    return -1;
    }

  int numPlotDataNodes = this->GetNumberOfNodeReferences(
    this->GetPlotDataNodeReferenceRole());

  for (int plotIndex = 0; plotIndex < numPlotDataNodes; plotIndex++)
    {
    const char* id = this->GetNthNodeReferenceID(
      this->GetPlotDataNodeReferenceRole(), plotIndex);
    if (!strcmp(plotDataNodeID, id))
      {
      return plotIndex;
      break;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotChartNode::GetPlotDataNodeID()
{
  return this->GetNthPlotDataNodeID(0);
}

//----------------------------------------------------------------------------
vtkMRMLPlotDataNode* vtkMRMLPlotChartNode::GetNthPlotDataNode(int n)
{
  return vtkMRMLPlotDataNode::SafeDownCast(
    this->GetNthNodeReference(this->GetPlotDataNodeReferenceRole(), n));
}

//----------------------------------------------------------------------------
vtkMRMLPlotDataNode* vtkMRMLPlotChartNode::GetPlotDataNode()
{
  return this->GetNthPlotDataNode(0);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotChartNode::ProcessMRMLEvents(vtkObject *caller,
                                              unsigned long event,
                                              void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  int numPlotDataNodes = this->GetNumberOfNodeReferences(this->GetPlotDataNodeReferenceRole());
  for (int plotIndex = 0; plotIndex < numPlotDataNodes; plotIndex++)
    {
    vtkMRMLPlotDataNode *pnode = this->GetNthPlotDataNode(plotIndex);
    if (pnode != NULL && pnode == vtkMRMLPlotDataNode::SafeDownCast(caller) &&
       (event ==  vtkCommand::ModifiedEvent || event == vtkMRMLPlotDataNode::TableModifiedEvent))
      {
      this->InvokeEvent(vtkMRMLPlotChartNode::PlotModifiedEvent, pnode);
      this->Modified();
      }
    }

  return;
}

//----------------------------------------------------------------------------
int vtkMRMLPlotChartNode::GetPlotDataNodeNames(std::vector<std::string> &plotDataNodeNames)
{
  plotDataNodeNames.clear();
  int numPlotDataNodes = this->GetNumberOfNodeReferences(this->GetPlotDataNodeReferenceRole());
  for (int plotIndex = 0; plotIndex < numPlotDataNodes; plotIndex++)
    {
    vtkMRMLPlotDataNode *pnode = this->GetNthPlotDataNode(plotIndex);
    if (!pnode)
      {
      continue;
      }
    plotDataNodeNames.push_back(pnode->GetName());
    }

  return static_cast<int>(plotDataNodeNames.size());
}

//----------------------------------------------------------------------------
int vtkMRMLPlotChartNode::GetPlotDataNodeIDs(std::vector<std::string> &plotDataNodeIDs)
{
  plotDataNodeIDs.clear();
  int numPlotDataNodes = this->GetNumberOfNodeReferences(this->GetPlotDataNodeReferenceRole());
  for (int plotIndex = 0; plotIndex < numPlotDataNodes; plotIndex++)
    {
    plotDataNodeIDs.push_back(this->GetNthPlotDataNodeID(plotIndex));
    }

  return static_cast<int>(plotDataNodeIDs.size());
}

// --------------------------------------------------------------------------
void vtkMRMLPlotChartNode::SetPropertyToAllPlotDataNodes(PlotDataNodeProperty plotProperty, const char* value)
{
  if (!this->GetScene())
    {
    vtkErrorMacro("vtkMRMLPlotChartNode::SetPropertyToAllPlotDataNodes failed: invalid scene");
    return;
    }

  int numPlotDataNodes = this->GetNumberOfNodeReferences(this->GetPlotDataNodeReferenceRole());

  std::vector<int> plotDataNodesWasModifying(numPlotDataNodes, 0);

  // Update all plot nodes and invoke modified events at the end

  for (int plotIndex = 0; plotIndex < numPlotDataNodes; plotIndex++)
    {
    vtkMRMLPlotDataNode *plotDataNode = vtkMRMLPlotDataNode::SafeDownCast(this->GetNthNodeReference(this->GetPlotDataNodeReferenceRole(), plotIndex));
    if (!plotDataNode)
      {
      continue;
      }
    plotDataNodesWasModifying[plotIndex] = plotDataNode->StartModify();

    if (plotProperty == PlotType)
      {
      plotDataNode->SetPlotType(value);
      }
    else if (plotProperty == PlotXColumnName)
      {
      plotDataNode->SetXColumnName(value);
      }
    else if (plotProperty == PlotYColumnName)
      {
      plotDataNode->SetYColumnName(value);
      }
    else if (plotProperty == PlotMarkerStyle)
      {
      plotDataNode->SetMarkerStyle(plotDataNode->GetMarkerStyleFromString(value));
      }
    }

  for (int plotIndex = 0; plotIndex < numPlotDataNodes; plotIndex++)
    {
    vtkMRMLPlotDataNode *plotDataNode = vtkMRMLPlotDataNode::SafeDownCast(this->GetNthNodeReference(this->GetPlotDataNodeReferenceRole(), plotIndex));
    if (!plotDataNode)
      {
      continue;
      }
    plotDataNode->EndModify(plotDataNodesWasModifying[plotIndex]);
  }
}

// --------------------------------------------------------------------------
bool vtkMRMLPlotChartNode::GetPropertyFromAllPlotDataNodes(PlotDataNodeProperty plotProperty, std::string& value)
{
  value.clear();
  if (!this->GetScene())
    {
    vtkErrorMacro("vtkMRMLPlotChartNode::GetPropertyFromAllPlotDataNodes failed: invalid scene");
    return false;
    }

  int numPlotDataNodes = this->GetNumberOfNodeReferences(this->GetPlotDataNodeReferenceRole());

  if (numPlotDataNodes < 1)
    {
    return false;
    }

  bool commonPropertyDefined = false;

  for (int plotIndex = 0; plotIndex < numPlotDataNodes; plotIndex++)
    {
    vtkMRMLPlotDataNode *plotDataNode = vtkMRMLPlotDataNode::SafeDownCast(this->GetNthNodeReference(this->GetPlotDataNodeReferenceRole(), plotIndex));
    if (!plotDataNode)
      {
      continue;
      }

    // Get property value
    std::string propertyValue;
    if (plotProperty == PlotType)
      {
      propertyValue = plotDataNode->GetPlotTypeAsString(plotDataNode->GetPlotType());
      }
    else if (plotProperty == PlotXColumnName)
      {
      propertyValue = plotDataNode->GetXColumnName();
      }
    else if (plotProperty == PlotYColumnName)
      {
      propertyValue = plotDataNode->GetYColumnName();
      }
    else if (plotProperty == PlotMarkerStyle)
      {
      propertyValue = plotDataNode->GetMarkerStyleAsString(plotDataNode->GetMarkerStyle());
      }

    if (commonPropertyDefined)
      {
      if (propertyValue != value)
        {
        // not all plot nodes have the same property value
        return false;
        }
      }
    else
      {
      commonPropertyDefined = true;
      value = propertyValue;
      }
    }

  return true;
}
