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
#include <vtkMRMLPlotViewNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkAssignAttribute.h>
#include <vtkObjectFactory.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkCxxSetReferenceStringMacro(vtkMRMLPlotViewNode, PlotLayoutNodeID);

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlotViewNode);

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
vtkMRMLPlotViewNode::vtkMRMLPlotViewNode()
: DoPropagatePlotLayoutSelection(true)
{
  this->PlotLayoutNodeID = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLPlotViewNode::~vtkMRMLPlotViewNode()
{
  if (this->PlotLayoutNodeID)
    {
    delete [] this->PlotLayoutNodeID;
    this->PlotLayoutNodeID = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  of << " PlotLayoutNodeID=\"" << (this->PlotLayoutNodeID ? this->PlotLayoutNodeID : "") << "\"";
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
    if (!strcmp(attName, "PlotLayoutNodeID"))
      {
      if (attValue && *attValue == '\0')
        {
        this->SetPlotLayoutNodeID(NULL);
        }
      else
        {
        this->SetPlotLayoutNodeID(attValue);
        }
      }
    else if(!strcmp(attName, "doPropagatePlotLayoutSelection"))
      {
      this->SetDoPropagatePlotLayoutSelection(StringToInt(attValue));
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

  this->SetAndUpdatePlotLayoutNodeID(aPlotviewnode->GetPlotLayoutNodeID());
  this->SetDoPropagatePlotLayoutSelection(aPlotviewnode->GetDoPropagatePlotLayoutSelection());

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PlotLayoutNodeID: " <<
   (this->PlotLayoutNodeID ? this->PlotLayoutNodeID : "(none)") << "\n";
  os << indent << "DoPropagatePlotLayoutSelection: " << this->DoPropagatePlotLayoutSelection << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::SetAndUpdatePlotLayoutNodeID(const char *PlotLayoutNodeID)
{
  this->SetPlotLayoutNodeID(PlotLayoutNodeID);

  // Add reference in the scene
  if (this->GetScene())
    {
    this->GetScene()->AddReferencedNodeID(this->PlotLayoutNodeID, this);
    }

  // Update singleton selection node
  vtkMRMLSelectionNode* selectionNode = NULL;
  if (this->GetScene())
    {
    selectionNode = vtkMRMLSelectionNode::SafeDownCast
      (this->GetScene()->GetNodeByID("vtkMRMLSelectionNodeSingleton"));
    }
  if (selectionNode)
    {
    selectionNode->SetReferenceActivePlotLayoutID(this->PlotLayoutNodeID);
    }

  this->InvokeEvent(vtkMRMLPlotViewNode::PlotLayoutNodeChangedEvent);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::SetAndUpdatePlotLayoutNodeID(const std::string &PlotLayoutNodeID)
{
    this->SetAndUpdatePlotLayoutNodeID( PlotLayoutNodeID.c_str() );
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::SetSceneReferences()
{
  this->Superclass::SetSceneReferences();

  if (this->GetScene())
    {
    this->SetAndUpdatePlotLayoutNodeID(this->GetPlotLayoutNodeID());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::UpdateReferences()
{
  Superclass::UpdateReferences();

  if (this->PlotLayoutNodeID != NULL && this->Scene->GetNodeByID(this->PlotLayoutNodeID) == NULL)
    {
    this->SetAndUpdatePlotLayoutNodeID(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotViewNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID, newID);

  if (this->PlotLayoutNodeID && !strcmp(oldID, this->PlotLayoutNodeID))
    {
    this->RemoveNodeReferenceIDs(oldID);
    this->SetAndUpdatePlotLayoutNodeID(newID);
    }
}
