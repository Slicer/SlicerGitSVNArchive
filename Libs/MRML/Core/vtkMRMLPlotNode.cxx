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

// MRML includes
#include "vtkMRMLPlotNode.h"
#include "vtkMRMLPlotLayoutNode.h"
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

//----------------------------------------------------------------------------
vtkCxxSetReferenceStringMacro(vtkMRMLPlotNode, TableNodeID);

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlotNode);

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

//----------------------------------------------------------------------------
vtkMRMLPlotNode::vtkMRMLPlotNode()
{
  this->Plot = NULL;
  this->Type = -1;
  this->Dirty = false;
  this->XColumn = 0;
  this->YColumn = 1;
  this->XColumnName = "(none)";
  this->YColumnName = "(none)";
  this->TableNodeID = NULL;
  this->TableNode = NULL;
  this->HideFromEditorsOff();

  this->SetType(LINE);
}

//----------------------------------------------------------------------------
vtkMRMLPlotNode::~vtkMRMLPlotNode()
{
  if (this->Plot)
    {
    this->Plot->Delete();
    this->Plot = NULL;
    }

  if (this->TableNodeID)
    {
    delete [] this->TableNodeID;
    this->TableNodeID = NULL;
    }

  if (this->TableNode)
    {
    this->TableNode->Delete();
    this->TableNode = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);
  of << " Type=\"" << this->GetType() << "\"";
  of << " XColumn=\"" << this->GetXColumn() << "\"";
  of << " YColumn=\"" << this->GetYColumn() << "\"";
  of << " TableNodeID=\"" << (this->TableNodeID ? this->TableNodeID : "") << "\"";
  of << " ";
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  vtkMRMLNode::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "Type"))
      {
      this->SetType(StringToInt(attValue));
      }
    else if (!strcmp(attName, "XColumn"))
      {
      this->SetXColumn(StringToInt(attValue));
      }
    else if (!strcmp(attName, "YColumn"))
      {
      this->SetYColumn(StringToInt(attValue));
      }
    else if (!strcmp(attName, "TableNodeID"))
      {
      if (attValue && *attValue == '\0')
        {
        this->SetTableNodeID(NULL);
        }
      else
        {
        this->SetTableNodeID(attValue);
        }
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
//
void vtkMRMLPlotNode::Copy(vtkMRMLNode *anode)
{
  vtkMRMLPlotNode *node = vtkMRMLPlotNode::SafeDownCast(anode);
  if (!node)
    {
    vtkErrorMacro("vtkMRMLPlotNode::Copy failed: invalid or incompatible source node");
    return;
    }
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);

  this->SetType(node->GetType());
  this->SetXColumn(node->GetXColumn());
  this->SetYColumn(node->GetYColumn());

  // Table (SetAndObserveTableNode set also InputData for plot)
  this->SetAndObserveTableNodeID(node->GetTableNodeID());

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::CopyAndSetNameAndType(vtkMRMLNode *anode, const char *name, int Type)
{
  vtkMRMLPlotNode *node = vtkMRMLPlotNode::SafeDownCast(anode);
  if (!node)
    {
    vtkErrorMacro("vtkMRMLPlotNode::Copy failed: invalid or incompatible source node");
    return;
    }

  if (!name)
    {
    vtkErrorMacro("vtkMRMLPlotNode::Copy failed: invalid name");
    return;
    }

  int disabledModify = this->StartModify();
  Superclass::Copy(anode);

  this->SetName(name);
  this->SetType(Type);
  this->SetXColumn(node->GetXColumn());
  this->SetYColumn(node->GetYColumn());

  // Table (SetAndObserveTableNode set also InputData for plot)
  this->SetAndObserveTableNodeID(node->GetTableNodeID());

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
  vtkPlot* callerPlot = vtkPlot::SafeDownCast(caller);
  if (event == vtkCommand::ModifiedEvent &&  callerPlot != NULL
    && this->Plot != NULL && this->Plot == callerPlot)
    {
    // this indicates that data stored in the node is changed (either the Plot or other
    // data members are changed)
    this->Modified();
    return;
    }

  vtkMRMLTableNode* callerMRMLTable = vtkMRMLTableNode::SafeDownCast(caller);
  if (event == vtkCommand::ModifiedEvent &&  callerMRMLTable != NULL
    && this->TableNode != NULL && this->TableNode == callerMRMLTable)
    {
    // this indicates that data stored in the node is changed (either the table, the plot or other
    // data members are changed)
    if (this->Plot != NULL)
      {
      // this is necessary to ensure that the vtkTable of Plot is updated when reading a saved scene
      // (in that case SetAndObserveTableID is called before that the table is read from file)
      if (!this->Plot->GetInput())
        {
        this->SetInputData(callerMRMLTable);
        }
      this->Plot->Modified();
      }
    this->Modified();
    return;
    }

  return;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetSceneReferences()
{
  this->Superclass::SetSceneReferences();

  if (this->GetScene())
    {
    this->SetAndObserveTableNodeID(this->GetTableNodeID());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::UpdateReferences()
{
  if (this->GetScene() == NULL)
    {
    return;
    }

  if (this->TableNodeID != NULL && this->GetScene()->GetNodeByID(this->TableNodeID) == NULL)
    {
    this->SetAndObserveTableNodeID(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  this->Superclass::UpdateReferenceID(oldID, newID);
  if (this->TableNodeID && !strcmp(oldID, this->TableNodeID))
    {
    this->RemoveNodeReferenceIDs(oldID);
    this->SetAndObserveTableNodeID(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "\nType: " << this->Type;
  os << indent << "\nXColumn: " << this->XColumn;
  os << indent << "\nXColumnName: " << this->XColumnName;
  os << indent << "\nYColumn: " << this->YColumn;
  os << indent << "\nYColumnName: " << this->YColumnName;
  os << indent << "\nTableNodeID: " <<
    (this->TableNodeID ? this->TableNodeID : "(none)");
  os << indent << "\nvtkPlot: " <<
    (this->Plot ? this->Plot->GetClassName() : "(none)");

  if (this->Plot)
    {
    this->Plot->PrintSelf(os,indent);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetAndObservePlot(vtkPlot* plot)
{
  if (plot == this->Plot)
    {
    return;
    }
  // this is necessary to pass to chartXY in the view that the Plot needs to be cleaned
  this->Dirty = true;

  this->InvokeEvent(vtkMRMLPlotNode::vtkPlotRemovedEvent, this->Plot);

  vtkSetAndObserveMRMLObjectMacro(this->Plot, plot);
  this->SetInputData(this->TableNode);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetInputData(vtkMRMLTableNode *tableNode,
                                   vtkIdType xColumn,
                                   vtkIdType yColumn)
{
  if (tableNode == NULL)
    {
    return;
    }

  if (tableNode->GetTable() == NULL       ||
      tableNode->GetNumberOfColumns() < 2 ||
      this->GetPlot() == NULL)
    {
    return;
    }

  this->SetXColumnName(this->GetTableNode()->GetColumnName(xColumn));
  this->SetYColumnName(this->GetTableNode()->GetColumnName(yColumn));

  this->GetPlot()->SetInputData(tableNode->GetTable(), xColumn, yColumn);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetInputData(vtkMRMLTableNode *tableNode)
{
  if (tableNode == NULL)
    {
    return;
    }

  this->SetInputData(tableNode, this->GetXColumn(), this->GetYColumn());
}

//----------------------------------------------------------------------------
std::string vtkMRMLPlotNode::GetXColumnName()
{
  return this->XColumnName;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetXColumnName(const std::string &xColumnName)
{
  this->XColumnName = xColumnName;
}

//----------------------------------------------------------------------------
std::string vtkMRMLPlotNode::GetYColumnName()
{
  return this->YColumnName;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetYColumnName(const std::string &yColumnName)
{
  this->YColumnName = yColumnName;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetAndObserveTableNodeID(const char *TableNodeID)
{
  vtkMRMLTableNode* cnode = NULL;
  if (this->GetScene() && TableNodeID)
    {
    cnode = vtkMRMLTableNode::SafeDownCast(
      this->GetScene()->GetNodeByID(TableNodeID));
    }
  if (this->TableNode != cnode)
    {
    vtkSetAndObserveMRMLObjectMacro(this->TableNode, cnode);
    }

  this->SetTableNodeID(TableNodeID);
  if (this->GetScene())
    {
    this->GetScene()->AddReferencedNodeID(this->TableNodeID, this);
    }

  this->SetInputData(this->TableNode);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetAndObserveTableNodeID(const std::string &TableNodeID)
{
  this->SetAndObserveTableNodeID( TableNodeID.c_str() );
}

//----------------------------------------------------------------------------
vtkMRMLTableNode *vtkMRMLPlotNode::GetTableNode()
{
  if (this->TableNode == NULL && this->TableNodeID == NULL)
    {
    return NULL;
    }

  if (this->TableNode != NULL || this->TableNodeID == NULL)
    {
    return this->TableNode;
    }
  vtkMRMLTableNode* cnode = NULL;
  if (this->GetScene())
    {
    cnode = vtkMRMLTableNode::SafeDownCast(
      this->GetScene()->GetNodeByID(this->TableNodeID));
    }
  vtkSetAndObserveMRMLObjectMacro(this->TableNode, cnode);
  return cnode;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetType(int type)
{
  if (this->Type == type)
    {
    return;
    }

  this->Type = type;

  int wasModifyingNode = this->StartModify();
  switch (this->Type)
  {
  case LINE:
    {
    vtkSmartPointer<vtkPlotLine> line = vtkSmartPointer<vtkPlotLine>::New();
    line->SetWidth(4.0);
    line->GetSelectionPen()->SetColor(137., 0., 13.);
    this->SetAndObservePlot(line);
    break;
    }
  case POINTS:
    {
    vtkSmartPointer<vtkPlotPoints> points = vtkSmartPointer<vtkPlotPoints>::New();
    points->SetMarkerSize(10.0);
    points->GetSelectionPen()->SetColor(137., 0., 13.);
    this->SetAndObservePlot(points);
    break;
    }
  case BAR:
    {
    vtkSmartPointer<vtkPlotBar> bar = vtkSmartPointer<vtkPlotBar>::New();
    bar->GetSelectionPen()->SetColor(137., 0., 13.);
    this->SetAndObservePlot(bar);
    break;
    }
  default:
    vtkWarningMacro(<< "vtkMRMLPlotNode::SetType : Type not known, "
                       "no vtkPlot has been instantiate.");
    this->Plot = NULL;
    this->Type = -1;
  }

  this->EndModify(wasModifyingNode);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetXColumn(vtkIdType xColumn)
{
  this->XColumn = xColumn;
  this->SetInputData(this->GetTableNode());
}

//----------------------------------------------------------------------------
void vtkMRMLPlotNode::SetYColumn(vtkIdType yColumn)
{
  this->YColumn = yColumn;
  this->SetInputData(this->GetTableNode());
}
