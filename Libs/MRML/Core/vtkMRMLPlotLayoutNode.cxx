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
#include "vtkMRMLPlotLayoutNode.h"
#include "vtkMRMLPlotNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTableNode.h"

const char* vtkMRMLPlotLayoutNode::PlotNodeReferenceRole = "plot";
const char* vtkMRMLPlotLayoutNode::PlotNodeReferenceMRMLAttributeName = "plotNodeRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlotLayoutNode);

//----------------------------------------------------------------------------
vtkMRMLPlotLayoutNode::vtkMRMLPlotLayoutNode()
{
  this->HideFromEditors = 0;

  // default properties
  this->SetAttribute("Type", "Line");

  this->SetAttribute("ShowGrid", "on");
  this->SetAttribute("ShowLegend", "on");

  this->SetAttribute("ShowTitle", "on");
  this->SetAttribute("ShowXAxisLabel", "on");
  this->SetAttribute("ShowYAxisLabel", "on");

  this->SetAttribute("TitleName", "");
  this->SetAttribute("XAxisLabelName", "");
  this->SetAttribute("YAxisLabelName", "");

  this->SetAttribute("ClickAndDragAlongX", "on");
  this->SetAttribute("ClickAndDragAlongY", "on");

  this->SetAttribute("FontType", "Arial");
  this->SetAttribute("TitleFontSize", "20");
  this->SetAttribute("AxisTitleFontSize", "16");
  this->SetAttribute("AxisLabelFontSize", "12");

  this->SetAttribute("LookupTable", "");

  vtkIntArray  *events = vtkIntArray::New();
  events->InsertNextValue(vtkCommand::ModifiedEvent);
  events->InsertNextValue(vtkMRMLPlotLayoutNode::PlotModifiedEvent);
  events->InsertNextValue(vtkMRMLPlotNode::TableModifiedEvent);

  this->AddNodeReferenceRole(this->GetPlotNodeReferenceRole(),
                             this->GetPlotNodeReferenceMRMLAttributeName(),
                             events);
  events->Delete();
}


//----------------------------------------------------------------------------
vtkMRMLPlotLayoutNode::~vtkMRMLPlotLayoutNode()
{
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotLayoutNode::GetPlotNodeReferenceRole()
{
  return vtkMRMLPlotLayoutNode::PlotNodeReferenceRole;
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotLayoutNode::GetPlotNodeReferenceMRMLAttributeName()
{
  return vtkMRMLPlotLayoutNode::PlotNodeReferenceMRMLAttributeName;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::OnNodeReferenceAdded(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceAdded(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotLayoutNode::PlotModifiedEvent, reference->GetReferencedNode());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::OnNodeReferenceModified(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceModified(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotLayoutNode::PlotModifiedEvent, reference->GetReferencedNode());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::OnNodeReferenceRemoved(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceRemoved(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotLayoutNode::PlotModifiedEvent, reference->GetReferencedNode());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  vtkMRMLNode::ReadXMLAttributes(atts);

  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
void vtkMRMLPlotLayoutNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::SetAndObservePlotNodeID(const char *plotNodeID)
{
  this->SetAndObserveNodeReferenceID(this->GetPlotNodeReferenceRole(), plotNodeID);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::AddAndObservePlotNodeID(const char *plotNodeID)
{
  this->AddAndObserveNodeReferenceID(this->GetPlotNodeReferenceRole(), plotNodeID);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::RemovePlotNodeID(const char *plotNodeID)
{
  if (!plotNodeID)
    {
    return;
    }

  this->RemoveNthPlotNodeID(this->GetNthPlotIdexFromID(plotNodeID));
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::RemoveNthPlotNodeID(int n)
{
  this->RemoveNthNodeReferenceID(this->GetPlotNodeReferenceRole(), n);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::RemoveAllPlotNodeIDs()
{
  this->RemoveNodeReferenceIDs(this->GetPlotNodeReferenceRole());
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::SetAndObserveNthPlotNodeID(int n, const char *plotNodeID)
{
  this->SetAndObserveNthNodeReferenceID(this->GetPlotNodeReferenceRole(), n, plotNodeID);
}

//----------------------------------------------------------------------------
bool vtkMRMLPlotLayoutNode::HasPlotNodeID(const char* plotNodeID)
{
  return this->HasNodeReferenceID(this->GetPlotNodeReferenceRole(), plotNodeID);
}

//----------------------------------------------------------------------------
int vtkMRMLPlotLayoutNode::GetNumberOfPlotNodes()
{
  return this->GetNumberOfNodeReferences(this->GetPlotNodeReferenceRole());
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotLayoutNode::GetNthPlotNodeID(int n)
{
    return this->GetNthNodeReferenceID(this->GetPlotNodeReferenceRole(), n);
}

//----------------------------------------------------------------------------
int vtkMRMLPlotLayoutNode::GetNthPlotIdexFromID(const char *plotNodeID)
{
  if (!plotNodeID)
    {
    return -1;
    }

  int numPlotNodes = this->GetNumberOfNodeReferences(
    this->GetPlotNodeReferenceRole());

  for (int plotIndex = 0; plotIndex < numPlotNodes; plotIndex++)
    {
    const char* id = this->GetNthNodeReferenceID(
      this->GetPlotNodeReferenceRole(), plotIndex);
    if (!strcmp(plotNodeID, id))
      {
      return plotIndex;
      break;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
vtkIdType vtkMRMLPlotLayoutNode::GetColorPlotIdexFromID(const char *plotNodeID)
{
  std::string tempPlotNodeID(plotNodeID);

  vtkMRMLPlotNode* plotNode = this->GetNthPlotNode
    (this->GetNthPlotIdexFromID(plotNodeID));
  if (!plotNode)
    {
    return -1;
    }
  std::string namePlotNode = plotNode->GetName();
  std::size_t found = namePlotNode.find("Markups");
  if (found != std::string::npos)
    {
    vtkMRMLPlotNode* markupsPlotNode = vtkMRMLPlotNode::SafeDownCast
      (plotNode->GetNodeReference("Markups"));
    if (!markupsPlotNode)
      {
      return -1;
      }
    tempPlotNodeID = markupsPlotNode->GetID();
    }

  return this->GetNthPlotIdexFromID(tempPlotNodeID.c_str());
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotLayoutNode::GetPlotNodeID()
{
  return this->GetNthPlotNodeID(0);
}

//----------------------------------------------------------------------------
vtkMRMLPlotNode* vtkMRMLPlotLayoutNode::GetNthPlotNode(int n)
{
  return vtkMRMLPlotNode::SafeDownCast(
    this->GetNthNodeReference(this->GetPlotNodeReferenceRole(), n));
}

//----------------------------------------------------------------------------
vtkMRMLPlotNode* vtkMRMLPlotLayoutNode::GetPlotNode()
{
  return this->GetNthPlotNode(0);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::ProcessMRMLEvents(vtkObject *caller,
                                              unsigned long event,
                                              void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  int numPlotNodes = this->GetNumberOfNodeReferences(this->GetPlotNodeReferenceRole());
  for (int plotIndex = 0; plotIndex < numPlotNodes; plotIndex++)
    {
    vtkMRMLPlotNode *pnode = this->GetNthPlotNode(plotIndex);
    if (pnode != NULL && pnode == vtkMRMLPlotNode::SafeDownCast(caller) &&
       (event ==  vtkCommand::ModifiedEvent || event == vtkMRMLPlotNode::TableModifiedEvent))
      {
      this->InvokeEvent(vtkMRMLPlotLayoutNode::PlotModifiedEvent, pnode);
      this->Modified();
      }
    }

  return;
}

//----------------------------------------------------------------------------
int vtkMRMLPlotLayoutNode::GetPlotNames(std::vector<std::string> &plotNodeNames)
{
  plotNodeNames.clear();
  int numPlotNodes = this->GetNumberOfNodeReferences(this->GetPlotNodeReferenceRole());
  for (int plotIndex = 0; plotIndex < numPlotNodes; plotIndex++)
    {
    vtkMRMLPlotNode *pnode = this->GetNthPlotNode(plotIndex);
    if (!pnode)
      {
      continue;
      }
    plotNodeNames.push_back(pnode->GetName());
    }

  return static_cast<int>(plotNodeNames.size());
}

//----------------------------------------------------------------------------
int vtkMRMLPlotLayoutNode::GetPlotIDs(std::vector<std::string> &plotNodeIDs)
{
  plotNodeIDs.clear();
  int numPlotNodes = this->GetNumberOfNodeReferences(this->GetPlotNodeReferenceRole());
  for (int plotIndex = 0; plotIndex < numPlotNodes; plotIndex++)
    {
    plotNodeIDs.push_back(this->GetNthPlotNodeID(plotIndex));
    }

  return static_cast<int>(plotNodeIDs.size());
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::SetPlotType(const char *Type)
{
    if (!this->GetScene())
      {
      return;
      }

    int wasModifying = this->StartModify();
    std::vector<std::string> plotNodesIDs;
    this->GetPlotIDs(plotNodesIDs);

    std::vector<std::string>::iterator it = plotNodesIDs.begin();
    for (; it != plotNodesIDs.end(); ++it)
      {
      vtkMRMLPlotNode* plotNode = vtkMRMLPlotNode::SafeDownCast
        (this->GetScene()->GetNodeByID((*it).c_str()));
      if (!plotNode)
        {
        continue;
        }

      std::string namePlotNode = plotNode->GetName();
      std::size_t found = namePlotNode.find("Markups");
      if (found != std::string::npos)
        {
        this->RemovePlotNodeID(plotNode->GetID());
        plotNode->GetNodeReference("Markups")->RemoveNodeReferenceIDs("Markups");
        this->GetScene()->RemoveNode(plotNode);
        continue;
        }

      if (!strcmp(Type,"Line"))
        {
        plotNode->SetType(vtkMRMLPlotNode::LINE);
        }
      else if (!strcmp(Type,"Scatter"))
        {
        plotNode->SetType(vtkMRMLPlotNode::POINTS);
        }
      else if (!strcmp(Type,"Line and Scatter"))
        {
        plotNode->SetType(vtkMRMLPlotNode::LINE);

        vtkMRMLPlotNode* plotNodeCopy = vtkMRMLPlotNode::SafeDownCast
          (plotNode->GetNodeReference("Markups"));

        if (plotNodeCopy)
          {
          plotNodeCopy->SetType(vtkMRMLPlotNode::POINTS);
          }
        else
          {
          vtkSmartPointer<vtkMRMLNode> node = vtkSmartPointer<vtkMRMLNode>::Take
            (this->GetScene()->CreateNodeByClass("vtkMRMLPlotNode"));
          plotNodeCopy = vtkMRMLPlotNode::SafeDownCast(node);
          std::string namePlotNodeCopy = namePlotNode + " Markups";
          plotNodeCopy->CopyWithScene(plotNode);
          plotNodeCopy->SetName(namePlotNodeCopy.c_str());
          plotNodeCopy->SetType(vtkMRMLPlotNode::POINTS);
          this->GetScene()->AddNode(plotNodeCopy);
          plotNode->AddNodeReferenceID("Markups", plotNodeCopy->GetID());
          plotNodeCopy->AddNodeReferenceID("Markups", plotNode->GetID());
          }

        this->AddAndObservePlotNodeID(plotNodeCopy->GetID());
        }
      else if (!strcmp(Type,"Bar"))
        {
        plotNode->SetType(vtkMRMLPlotNode::BAR);
        }
      }

    this->SetAttribute("Type", Type);

    this->EndModify(wasModifying);
}
