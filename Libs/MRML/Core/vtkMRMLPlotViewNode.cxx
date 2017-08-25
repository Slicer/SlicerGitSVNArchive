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
#include "vtkMRMLPlotLayoutNode.h"
#include "vtkMRMLPlotViewNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLSelectionNode.h"

// VTK includes
#include <vtkAssignAttribute.h>
#include <vtkCommand.h>
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <sstream>

const char* vtkMRMLPlotViewNode::PlotLayoutNodeReferenceRole = "plotLayout";
const char* vtkMRMLPlotViewNode::PlotLayoutNodeReferenceMRMLAttributeName = "plotLayoutNodeRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlotViewNode);

//----------------------------------------------------------------------------
vtkMRMLPlotViewNode::vtkMRMLPlotViewNode()
: DoPropagatePlotLayoutSelection(true)
{
  vtkIntArray  *events = vtkIntArray::New();
  events->InsertNextValue(vtkCommand::ModifiedEvent);
  events->InsertNextValue(vtkMRMLPlotViewNode::PlotLayoutNodeChangedEvent);
  events->InsertNextValue(vtkMRMLPlotLayoutNode::PlotModifiedEvent);

  this->AddNodeReferenceRole(this->GetPlotLayoutNodeReferenceRole(),
                             this->GetPlotLayoutNodeReferenceMRMLAttributeName(),
                             events);
  events->Delete();
}

//----------------------------------------------------------------------------
vtkMRMLPlotViewNode::~vtkMRMLPlotViewNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  of << " doPropagatePlotLayoutSelection=\"" << (int)this->DoPropagatePlotLayoutSelection << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if(!strcmp(attName, "doPropagatePlotLayoutSelection"))
      {
      this->SetDoPropagatePlotLayoutSelection(atoi(attValue)?true:false);
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLPlotViewNode::Copy(vtkMRMLNode *anode)
{
  vtkMRMLPlotViewNode *aPlotviewnode = vtkMRMLPlotViewNode::SafeDownCast(anode);

  int disabledModify = this->StartModify();

  this->Superclass::Copy(anode);

  this->SetDoPropagatePlotLayoutSelection(aPlotviewnode->GetDoPropagatePlotLayoutSelection());

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DoPropagatePlotLayoutSelection: " << this->DoPropagatePlotLayoutSelection << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::SetPlotLayoutNodeID(const char* plotLayoutNodeId)
{
  this->SetNodeReferenceID(this->GetPlotLayoutNodeReferenceRole(), plotLayoutNodeId);
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotViewNode::GetPlotLayoutNodeID()
{
  return this->GetNodeReferenceID(this->GetPlotLayoutNodeReferenceRole());
}

//----------------------------------------------------------------------------
vtkMRMLPlotLayoutNode* vtkMRMLPlotViewNode::GetPlotLayoutNode()
{
  return vtkMRMLPlotLayoutNode::SafeDownCast(this->GetNodeReference(this->GetPlotLayoutNodeReferenceRole()));
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  vtkMRMLPlotLayoutNode *pnode = this->GetPlotLayoutNode();
  if (pnode != NULL && pnode == vtkMRMLPlotLayoutNode::SafeDownCast(caller) &&
     (event ==  vtkCommand::ModifiedEvent || event == vtkMRMLPlotLayoutNode::PlotModifiedEvent))
    {
    this->InvokeEvent(vtkMRMLPlotViewNode::PlotLayoutNodeChangedEvent, pnode);
    }

  return;
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotViewNode::GetPlotLayoutNodeReferenceRole()
{
  return vtkMRMLPlotViewNode::PlotLayoutNodeReferenceRole;
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlotViewNode::GetPlotLayoutNodeReferenceMRMLAttributeName()
{
    return vtkMRMLPlotViewNode::PlotLayoutNodeReferenceMRMLAttributeName;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::OnNodeReferenceAdded(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceAdded(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotLayoutNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotViewNode::PlotLayoutNodeChangedEvent, reference->GetReferencedNode());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::OnNodeReferenceModified(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceModified(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotLayoutNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotViewNode::PlotLayoutNodeChangedEvent, reference->GetReferencedNode());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::OnNodeReferenceRemoved(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceRemoved(reference);
  if (std::string(reference->GetReferenceRole()) == this->PlotLayoutNodeReferenceRole)
    {
    this->InvokeEvent(vtkMRMLPlotViewNode::PlotLayoutNodeChangedEvent, reference->GetReferencedNode());
    }
}
