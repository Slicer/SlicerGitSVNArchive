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

// MRML includes
#include "vtkMRMLPlotDataNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLSelectionNode.h"
#include "vtkMRMLTableNode.h"

// VTK includes
#include <vtkAlgorithmOutput.h>
#include <vtkAssignAttribute.h>
#include <vtkBrush.h>
#include <vtkCallbackCommand.h>
#include <vtkColorSeries.h>
#include <vtkCommand.h>
#include <vtkContextMapper2D.h>
#include <vtkEventBroker.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkPlotBar.h>
#include <vtkPlotLine.h>
#include <vtkPlotPoints.h>
#include <vtkTable.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <algorithm>
#include <sstream>

const char* vtkMRMLPlotDataNode::TableNodeReferenceRole = "table";
const char* vtkMRMLPlotDataNode::TableNodeReferenceMRMLAttributeName = "tableNodeRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlotDataNode);

//----------------------------------------------------------------------------
vtkMRMLPlotDataNode::vtkMRMLPlotDataNode()
  : PlotType(SCATTER)
  , LineWidth(2)
  , MarkerSize(7)
  , MarkerStyle(VTK_MARKER_NONE)
  , Opacity(1.0)
{
  this->HideFromEditors = 0;
  this->Color[0] = 0.0;
  this->Color[1] = 0.0;
  this->Color[2] = 0.0;

  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkCommand::ModifiedEvent);
  events->InsertNextValue(vtkMRMLPlotDataNode::TableModifiedEvent);
  this->AddNodeReferenceRole(this->GetTableNodeReferenceRole(),
                             this->GetTableNodeReferenceMRMLAttributeName(),
                             events.GetPointer());
}

//----------------------------------------------------------------------------
vtkMRMLPlotDataNode::~vtkMRMLPlotDataNode()
{
}

//----------------------------------------------------------------------------
const char *vtkMRMLPlotDataNode::GetTableNodeReferenceRole()
{
  return vtkMRMLPlotDataNode::TableNodeReferenceRole;
}

//----------------------------------------------------------------------------
const char *vtkMRMLPlotDataNode::GetTableNodeReferenceMRMLAttributeName()
{
  return vtkMRMLPlotDataNode::TableNodeReferenceMRMLAttributeName;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotDataNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  vtkMRMLWriteXMLBeginMacro(of)
  vtkMRMLWriteXMLEnumMacro(plotType, PlotType)
  vtkMRMLWriteXMLStdStringMacro(xColumnName, XColumnName)
  vtkMRMLWriteXMLStdStringMacro(yColumnName, YColumnName)
  vtkMRMLWriteXMLEnumMacro(markerStyle, MarkerStyle)
  vtkMRMLWriteXMLFloatMacro(markerSize, MarkerSize)
  vtkMRMLWriteXMLFloatMacro(lineWidth, LineWidth)
  vtkMRMLWriteXMLVectorMacro(color, Color, double, 3)
  vtkMRMLWriteXMLFloatMacro(opacity, Opacity)
  vtkMRMLWriteXMLEndMacro()
}

//----------------------------------------------------------------------------
void vtkMRMLPlotDataNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  vtkMRMLReadXMLBeginMacro(atts)
  vtkMRMLReadXMLEnumMacro(plotType, PlotType)
  vtkMRMLReadXMLStdStringMacro(xColumnName, XColumnName)
  vtkMRMLReadXMLStdStringMacro(yColumnName, YColumnName)
  vtkMRMLReadXMLEnumMacro(markerStyle, MarkerStyle)
  vtkMRMLReadXMLFloatMacro(markerSize, MarkerSize)
  vtkMRMLReadXMLFloatMacro(lineWidth, LineWidth)
  vtkMRMLReadXMLVectorMacro(color, Color, double, 3)
  vtkMRMLReadXMLFloatMacro(opacity, Opacity)
  vtkMRMLReadXMLEndMacro()

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
//
void vtkMRMLPlotDataNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  vtkMRMLCopyBeginMacro(anode, vtkMRMLPlotDataNode)
  vtkMRMLCopyEnumMacro(PlotType)
  vtkMRMLCopyStdStringMacro(XColumnName)
  vtkMRMLCopyStdStringMacro(YColumnName)
  vtkMRMLCopyEnumMacro(MarkerStyle)
  vtkMRMLCopyFloatMacro(MarkerSize)
  vtkMRMLCopyFloatMacro(LineWidth)
  vtkMRMLCopyVectorMacro(Color, double, 3)
  vtkMRMLCopyFloatMacro(Opacity)
  vtkMRMLCopyEndMacro()

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotDataNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  vtkMRMLPrintBeginMacro(os, indent)
  vtkMRMLPrintEnumMacro(PlotType)
  vtkMRMLPrintStdStringMacro(XColumnName)
  vtkMRMLPrintStdStringMacro(YColumnName)
  vtkMRMLPrintEnumMacro(MarkerStyle)
  vtkMRMLPrintFloatMacro(MarkerSize)
  vtkMRMLPrintFloatMacro(LineWidth)
  vtkMRMLPrintVectorMacro(Color, double, 3)
  vtkMRMLPrintFloatMacro(Opacity)
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotDataNode::ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  if (caller == NULL ||
      (event != vtkCommand::ModifiedEvent &&
       event != vtkMRMLPlotDataNode::TableModifiedEvent))
    {
    return;
    }

  vtkMRMLTableNode *tnode = this->GetTableNode();
  vtkMRMLTableNode *callerTable = vtkMRMLTableNode::SafeDownCast(caller);
  if (callerTable != NULL && tnode != NULL && tnode == callerTable &&
      event == vtkCommand::ModifiedEvent)
    {
    this->InvokeCustomModifiedEvent(vtkMRMLPlotDataNode::TableModifiedEvent, callerTable);
    }

  return;
}

//----------------------------------------------------------------------------
const char *vtkMRMLPlotDataNode::GetTableNodeID()
{
  return this->GetNodeReferenceID(this->GetTableNodeReferenceRole());
}

//----------------------------------------------------------------------------
void vtkMRMLPlotDataNode::SetAndObserveTableNodeID(const char *tableNodeID)
{
  // Set and Observe the MRMLTable reference
  this->SetAndObserveNodeReferenceID(this->GetTableNodeReferenceRole(), tableNodeID);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotDataNode::SetAndObserveTableNodeID(const std::string &tableNodeID)
{
  return this->SetAndObserveTableNodeID(tableNodeID.c_str());
}

//----------------------------------------------------------------------------
vtkMRMLTableNode *vtkMRMLPlotDataNode::GetTableNode()
{
  return vtkMRMLTableNode::SafeDownCast(
    this->GetNodeReference(this->GetTableNodeReferenceRole()));
}

//----------------------------------------------------------------------------
void vtkMRMLPlotDataNode::SetPlotType(const char *type)
{
  this->SetPlotType(this->GetPlotTypeFromString(type));
}


//-----------------------------------------------------------
const char* vtkMRMLPlotDataNode::GetPlotTypeAsString(int id)
{
  switch (id)
    {
    case SCATTER: return "Scatter";
    case BAR: return "Bar";
    case PIE: return "Pie";
    case BOX: return "Box";
    default:
      // invalid id
      return "";
    }
}

//-----------------------------------------------------------
int vtkMRMLPlotDataNode::GetPlotTypeFromString(const char* name)
{
  if (name == NULL)
    {
    // invalid name
    return -1;
    }
  for (int ii = 0; ii < PLOT_TYPE_LAST; ii++)
    {
    if (strcmp(name, GetPlotTypeAsString(ii)) == 0)
      {
      // found a matching name
      return ii;
      }
    }
  // unknown name
  return -1;
}

//-----------------------------------------------------------
const char* vtkMRMLPlotDataNode::GetMarkerStyleAsString(int id)
{
  switch (id)
    {
    case VTK_MARKER_NONE: return "None";
    case VTK_MARKER_CROSS: return "Cross";
    case VTK_MARKER_PLUS: return "Plus";
    case VTK_MARKER_SQUARE: return "Square";
    case VTK_MARKER_CIRCLE: return "Circle";
    case VTK_MARKER_DIAMOND: return "Diamond";
    default:
      // invalid id
      return "";
    }
}

//-----------------------------------------------------------
int vtkMRMLPlotDataNode::GetMarkerStyleFromString(const char* name)
{
  if (name == NULL)
    {
    // invalid name
    return -1;
    }
  for (int ii = 0; ii < VTK_MARKER_UNKNOWN; ii++)
    {
    if (strcmp(name, GetMarkerStyleAsString(ii)) == 0)
      {
      // found a matching name
      return ii;
      }
    }
  // unknown name
  return -1;
}

//-----------------------------------------------------------
bool vtkMRMLPlotDataNode::IsXColumnIndex()
{
  return (this->GetXColumnName().empty());
}
