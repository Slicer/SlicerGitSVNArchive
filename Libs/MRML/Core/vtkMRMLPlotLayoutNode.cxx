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

class PlotIDMap : public std::map<std::string, std::string> {} ;
class PlotLayoutPropertyMap : public std::map<std::string, std::string> {} ;

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlotLayoutNode);

//----------------------------------------------------------------------------
vtkMRMLPlotLayoutNode::vtkMRMLPlotLayoutNode()
{
  this->HideFromEditors = 0;

  this->MapPlotNamesIDs = new PlotIDMap;
  this->PlotNames = vtkStringArray::New();
  this->PlotIDs = vtkStringArray::New();
  this->Properties = new PlotLayoutPropertyMap;

  this->plotNodes = vtkSmartPointer<vtkCollection>::New();
  this->selectionPlotNodeIDs = vtkSmartPointer<vtkStringArray>::New();
  this->selectionArrays = vtkSmartPointer<vtkCollection>::New();

  // default properties
  this->SetProperty("type", "Line");

  this->SetProperty("showGrid", "on");
  this->SetProperty("showLegend", "on");

  this->SetProperty("showTitle", "on");
  this->SetProperty("showXAxisLabel", "on");
  this->SetProperty("showYAxisLabel", "on");

  this->SetProperty("TitleName", "");
  this->SetProperty("XAxisLabelName", "");
  this->SetProperty("YAxisLabelName", "");

  this->SetProperty("ClickAndDragAlongX", "on");
  this->SetProperty("ClickAndDragAlongY", "on");

  this->SetProperty("FontType", "Arial");
  this->SetProperty("TitleFontSize", "20");
  this->SetProperty("AxisTitleFontSize", "16");
  this->SetProperty("AxisLabelFontSize", "12");

  this->SetProperty("lookupTable", "");
}


//----------------------------------------------------------------------------
vtkMRMLPlotLayoutNode::~vtkMRMLPlotLayoutNode()
{
  this->ClearPlotIDs();
  this->ClearProperties();

  if (this->MapPlotNamesIDs)
    {
    delete this->MapPlotNamesIDs;
    this->MapPlotNamesIDs = NULL;
    }

  if (this->Properties)
    {
    delete this->Properties;
    this->Properties = NULL;
    }

  if (this->PlotNames)
    {
    this->PlotNames->Delete();
    this->PlotNames = NULL;
    }

  if (this->PlotIDs)
    {
    this->PlotIDs->Delete();
    this->PlotIDs = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  // Write all the IDs
  of << " PlotNodeIDs=\"";
  PlotIDMap::iterator it;
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end(); ++it)
    {
    if (it != this->MapPlotNamesIDs->begin())
      {
      of << " ";
      }
    of << "'" << (*it).first << "':'" << (*it).second << "'";
    }
  of << "\"";

  // Current Observed PlotNodes collection (this->plotNodes) does not need to be written in the scene.
  // The collection is created on request from MapPlotNamesIDs.

  // Write current selected PlotNodes and Arrays collections.
  // NOT IMPLEMENTED: in the case of selections of milions (or more) of data points
  //                  writing and reading all the index of the values in a xml is inefficient
  //                  and it will make the xml scene file illegible.
  // POSSIBLE SOLUTION: allocate a StorageNode to save the array something analogous
  //                    to the DoubleArray or the Tables.
  //                    (is it worth? the selection can be easily redone by the user).

  // Write out the properties
  of << " Properties=\"";
  PlotLayoutPropertyMap::iterator pit;
  for (pit = this->Properties->begin(); pit != this->Properties->end(); ++pit)
    {
    of << "'" << (*pit).first << "','" << (*pit).second << "'";
    }
  of << "\"";
}


//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  vtkMRMLNode::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "PlotNodeIDs"))
      {
      // format is 'name':'id'
      // Search for 4 single quotes and pull out the pieces.
      std::string text(attValue);
      const std::string::size_type n = text.length();
      std::string::size_type first=0, second, third, fourth;
      first = text.find_first_of("'");
      while (first < n)
       {
        second = text.find_first_of("'", first+1);
        third = text.find_first_of("'", second+1);
        fourth = text.find_first_of("'", third+1);

        this->AddPlot(text.substr(first+1, second-first-1).c_str(),
                      text.substr(third+1, fourth-third-1).c_str());

        first = text.find_first_of("'",fourth+1);
        }
      }
    else if (!strcmp(attName, "Properties"))
      {
      // format is 'Plotname','propertyname','value'
      // Search for 4 single quotes and pull out the pieces
      std::string text(attValue);
      const std::string::size_type n = text.length();
      std::string::size_type first=0, second, third, fourth;
      first = text.find_first_of("'");
      while (first < n)
        {
        second = text.find_first_of("'", first+1);
        third = text.find_first_of("'", second+1);
        fourth = text.find_first_of("'", third+1);

        this->SetProperty(text.substr(first+1, second-first-1).c_str(),
                          text.substr(third+1, fourth-third-1).c_str());

        first = text.find_first_of("'",fourth+1);
        }

      }
    }

  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
void vtkMRMLPlotLayoutNode::Copy(vtkMRMLNode *anode)
{
  vtkMRMLPlotLayoutNode *aPlotLayoutnode = vtkMRMLPlotLayoutNode::SafeDownCast(anode);

  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  // Need to manage references to other nodes.  Unregister this node's
  // currrent references (done implictly when clearing the PlotIDs and properties).
  this->ClearPlotIDs();
  this->ClearProperties();

  // copy the Plot list and properties from the other node. Don't
  // bother copying the ivars PlotNames, PlotIDs and PlotNodes as they are
  // constructed upon request
  if (aPlotLayoutnode)
    {
    *(this->MapPlotNamesIDs) = *(aPlotLayoutnode->MapPlotNamesIDs);
    *(this->Properties) = *(aPlotLayoutnode->Properties);
    this->selectionArrays = aPlotLayoutnode->selectionArrays;
    this->selectionPlotNodeIDs = aPlotLayoutnode->selectionPlotNodeIDs;
    // Add new references and set observations as well.
    this->SetSceneReferences();
    }

  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::ProcessMRMLEvents(vtkObject *caller,
                                              unsigned long event,
                                              void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  if (caller == NULL || this->GetScene() == NULL ||
      (event != vtkCommand::ModifiedEvent &&
       event != vtkMRMLPlotNode::TableModifiedEvent))
    {
    return;
    }

  vtkMRMLPlotNode* callerMRMLPlot = vtkMRMLPlotNode::SafeDownCast(caller);
  if (callerMRMLPlot != NULL)
    {
    vtkStringArray* plotNodesIDs = this->GetPlotIDs();
    if (!plotNodesIDs)
      {
      return;
      }

    for (int plotIDsIndex = 0; plotIDsIndex < plotNodesIDs->GetNumberOfValues(); plotIDsIndex++)
      {
      vtkMRMLPlotNode* plotNode = vtkMRMLPlotNode::SafeDownCast
        (this->GetScene()->GetNodeByID(plotNodesIDs->GetValue(plotIDsIndex)));
      if (plotNode == NULL)
        {
        continue;
        }

      if (callerMRMLPlot == plotNode)
        {
        this->Modified();
        }
      }
    return;
    }

  vtkMRMLTableNode* callerMRMLTable = vtkMRMLTableNode::SafeDownCast(caller);
  if (callerMRMLTable != NULL)
    {
    vtkStringArray* plotNodesIDs = this->GetPlotIDs();
    if (!plotNodesIDs)
      {
      return;
      }

    for (int plotIDsIndex = 0; plotIDsIndex < plotNodesIDs->GetNumberOfValues(); plotIDsIndex++)
      {
      vtkMRMLPlotNode* plotNode = vtkMRMLPlotNode::SafeDownCast
        (this->GetScene()->GetNodeByID(plotNodesIDs->GetValue(plotIDsIndex)));
      if (plotNode == NULL)
        {
        continue;
        }

      if (callerMRMLTable == plotNode->GetTableNode())
        {
        this->Modified();
        }
      }
    return;
    }

  return;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::AddAndObservePlot(const char *name, const char *id)
{
  this->AddPlot(name, id);
  this->ObservePlot(id);

  if (strcmp(this->GetProperty("type"), "Line and Scatter") != 0)
    {
    return;
    }

  vtkMRMLPlotNode *plotNode = NULL;
  if (this->GetScene())
    {
    plotNode = vtkMRMLPlotNode::SafeDownCast(this->GetScene()->GetNodeByID(id));
    }

  if (!plotNode)
    {
    return;
    }

  vtkMRMLPlotNode *markupsPlotNode = vtkMRMLPlotNode::SafeDownCast
    (plotNode->GetNodeReference("Markups"));

  if (!markupsPlotNode)
    {
    return;
    }

  this->AddPlot(markupsPlotNode->GetName(), markupsPlotNode->GetID());
  this->ObservePlot(markupsPlotNode->GetID());
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  // Write out the properties
  os << indent << "\n";
  os << indent << " Properties=\n";
  PlotLayoutPropertyMap::iterator pit;
  for (pit = this->Properties->begin(); pit != this->Properties->end(); ++pit)
    {
    os << indent << "'" << (*pit).first << "','" << (*pit).second << "'\n";
    }
  os << indent << "\n";

  // Write all the PlotNode IDs
  os << indent << " PlotNodeIDs=\n";
  PlotIDMap::iterator it;
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end(); ++it)
    {
    if (it != this->MapPlotNamesIDs->begin())
      {
      os << indent << " ";
      }
    os << indent << "'" << (*it).first << "':'" << (*it).second<< "'\n";
    }
  os << indent << "\n";

  // Write Plot node ID with observation allocated
  if (this->plotNodes)
    {
    os << indent << " PlotNodeIDs currently observed=\n";
    for (int plotIndex = 0; plotIndex < this->plotNodes->GetNumberOfItems(); plotIndex++)
      {
      vtkMRMLPlotNode* mrmlPlot = vtkMRMLPlotNode::SafeDownCast(this->plotNodes->GetItemAsObject(plotIndex));
      os << indent << "'" << mrmlPlot->GetID() << "'\n";
      }
    os << indent << "\n";
    }

  if (this->selectionArrays && this->selectionPlotNodeIDs)
    {
    os << indent << " PlotsNodeIDs and IdArrays currently selected =\n";
    for (int plotIndex = 0; plotIndex < this->selectionPlotNodeIDs->GetNumberOfValues(); plotIndex++)
      {
      os << indent << "'" << this->selectionPlotNodeIDs->GetValue(plotIndex) << ": ";
      vtkIdTypeArray* Array = vtkIdTypeArray::SafeDownCast(this->selectionArrays->GetItemAsObject(plotIndex));
      for (int arrayIndex = 0; arrayIndex < Array->GetNumberOfValues(); arrayIndex++)
        {
        os << indent << Array->GetValue(arrayIndex);
        if (arrayIndex < Array->GetNumberOfValues() - 1)
          {
          os<< ",";
          }
        }
      os << indent << "'\n";
      }
    os << indent << "\n";
    }

}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::AddPlot(const char *name, const char *id)
{
  if (!name || !id)
    {
    return;
    }

  PlotIDMap::iterator it = (*this->MapPlotNamesIDs).find(name);
  if (it != (*this->MapPlotNamesIDs).end())
    {
    if ((*it).second == id)
      {
      return;
      }
    }

  (*this->MapPlotNamesIDs)[name] = id;

  if (this->GetScene())
    {
    this->GetScene()->AddReferencedNodeID(id, this);
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::ObservePlot(const char *id)
{
  if (!id || !this->MRMLObserverManager || !this->plotNodes)
    {
    return;
    }

  vtkMRMLPlotNode *plotNode = NULL;
  if (this->GetScene())
    {
    plotNode = vtkMRMLPlotNode::SafeDownCast(this->GetScene()->GetNodeByID(id));
    }

  if (!plotNode)
    {
    return;
    }

  for (int plotNodesIndex = 0; plotNodesIndex < this->plotNodes->GetNumberOfItems(); plotNodesIndex++)
    {
    vtkMRMLPlotNode *plotNodeCol = vtkMRMLPlotNode::SafeDownCast
      (this->plotNodes->GetItemAsObject(plotNodesIndex));
    if (plotNodeCol == plotNode)
      {
      return;
      }
    }

  this->plotNodes->AddItem(plotNode);

  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkCommand::ModifiedEvent);
  events->InsertNextValue(vtkMRMLPlotNode::TableModifiedEvent);
  vtkNew<vtkFloatArray> priorities;
  priorities->InsertNextValue(1.);
  priorities->InsertNextValue(1.);
  this->MRMLObserverManager->AddObjectEvents(plotNode, events.GetPointer(), priorities.GetPointer());

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::RemovePlotAndObservationByName(const char *name)
{
  if (!name || !this->GetScene())
    {
    return;
    }

  PlotIDMap::iterator it = (*this->MapPlotNamesIDs).find(name);
  if (it == (*this->MapPlotNamesIDs).end())
    {
    return;
    }

  std::string id = this->GetPlotID(name);

  this->MapPlotNamesIDs->erase(name);
  this->GetScene()->RemoveReferencedNodeID(id.c_str(), this);

  vtkMRMLPlotNode *plotNode = NULL;
  plotNode = vtkMRMLPlotNode::SafeDownCast(this->GetScene()->GetNodeByID(id));

  if (!plotNode)
    {
    this->Modified();
    return;
    }

  vtkMRMLPlotNode *plotNodeMarkups = NULL;

  for (int plotNodesIndex = 0; plotNodesIndex < this->plotNodes->GetNumberOfItems(); plotNodesIndex++)
    {
    vtkMRMLPlotNode *plotNodeCol = vtkMRMLPlotNode::SafeDownCast
      (this->plotNodes->GetItemAsObject(plotNodesIndex));
    if (plotNodeCol == plotNode)
      {
      plotNodeMarkups = vtkMRMLPlotNode::SafeDownCast(plotNode->GetNodeReference("Markups"));
      this->plotNodes->RemoveItem(plotNode);
      this->MRMLObserverManager->RemoveObjectEvents(plotNode);
      break;
      }
    }

  // Check if there is a Markups Plot with the same name and remove it as well
  if (!plotNodeMarkups || (strcmp(this->GetProperty("type"), "Line and Scatter") != 0))
    {
    this->Modified();
    return;
    }
  std::string MarkupsName(plotNodeMarkups->GetName());
  it = (*this->MapPlotNamesIDs).find(MarkupsName.c_str());
  if (it == (*this->MapPlotNamesIDs).end())
    {
    this->Modified();
    return;
    }

  id = this->GetPlotID(MarkupsName.c_str());

  this->MapPlotNamesIDs->erase(MarkupsName);
  this->GetScene()->RemoveReferencedNodeID(id.c_str(), this);

  for (int plotNodesIndex = 0; plotNodesIndex < this->plotNodes->GetNumberOfItems(); plotNodesIndex++)
    {
    vtkMRMLPlotNode *plotNodeCol = vtkMRMLPlotNode::SafeDownCast
      (this->plotNodes->GetItemAsObject(plotNodesIndex));
    if (plotNodeCol == plotNodeMarkups)
      {
      this->plotNodes->RemoveItem(plotNode);
      this->MRMLObserverManager->RemoveObjectEvents(plotNode);
      break;
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::RemovePlotAndObservationByID(const char *id)
{
  this->RemovePlotAndObservationByName(this->GetPlotByID(id)->GetName());
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::UpdateObservations()
{
  if (!this->plotNodes)
    {
    return;
    }

  vtkStringArray* plotNodesIDs = this->GetPlotIDs();
  if (!plotNodesIDs)
    {
    return;
    }

  for (int plotIDIndex = 0; plotIDIndex < plotNodesIDs->GetNumberOfValues(); plotIDIndex++)
    {
    this->UpdateObservation(plotNodesIDs->GetValue(plotIDIndex));
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::UpdateObservation(const char *id)
{
  if (!id || !this->plotNodes || !this->GetScene())
    {
    return;
    }

  // confirm that the nodeID has been already added to PlotLayout
  vtkStringArray* plotNodesIDs = this->GetPlotIDs();
  if (!plotNodesIDs)
    {
    return;
    }

  bool plotIDFound = false;
  for (int plotIDIndex = 0; plotIDIndex < plotNodesIDs->GetNumberOfValues(); plotIDIndex++)
    {
    if (!strcmp(id, plotNodesIDs->GetValue(plotIDIndex)))
      {
      plotIDFound = true;
      break;
      }
    }

  if (!plotIDFound)
    {
    vtkDebugMacro("vtkMRMLPlotLayoutNode::UpdateObservation could not "
                  "update PlotNode observation because of ID is missing.")
    // Check and clean PlotNodes collection and Plot observations
    this->RemovePlotObservation(id);
    return;
    }

  // Plot is missing, add it to the collection and observe
  this->ObservePlot(id);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::RemovePlotObservation(const char *id)
{
  if (!id)
    {
    return;
    }

  vtkMRMLPlotNode *plotNode = NULL;
  if (this->GetScene())
    {
    plotNode = vtkMRMLPlotNode::SafeDownCast(this->GetScene()->GetNodeByID(id));
    }

  if (!plotNode)
    {
    return;
    }

  for (int plotNodesIndex = 0; plotNodesIndex < this->plotNodes->GetNumberOfItems(); plotNodesIndex++)
    {
    vtkMRMLPlotNode *plotNodeCol = vtkMRMLPlotNode::SafeDownCast
      (this->plotNodes->GetItemAsObject(plotNodesIndex));
    if (plotNodeCol == plotNode)
      {
      this->plotNodes->RemoveItem(plotNode);
      this->MRMLObserverManager->RemoveObjectEvents(plotNode);
      this->Modified();
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::ClearPlotIDs()
{
  PlotIDMap::iterator it;
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end(); ++it)
    {
    if (this->GetScene())
      {
      this->MRMLObserverManager->RemoveObjectEvents(vtkMRMLPlotNode::SafeDownCast
        (this->GetScene()->GetNodeByID((*it).second.c_str())));
      this->GetScene()->RemoveReferencedNodeID((*it).second.c_str(), this);
      }
    }
  this->MapPlotNamesIDs->clear();
  this->plotNodes->RemoveAllItems();
  this->Modified();
}


//----------------------------------------------------------------------------
const char* vtkMRMLPlotLayoutNode::GetPlotID(const char *name)
{
  if (!name)
    {
    return 0;
    }

  PlotIDMap::iterator it = (*this->MapPlotNamesIDs).find(name);

  if (it == this->MapPlotNamesIDs->end())
    {
    return 0;
    }

  return (*it).second.c_str();
}

//----------------------------------------------------------------------------
vtkMRMLPlotNode *vtkMRMLPlotLayoutNode::GetPlotbyName(const char *name)
{
  return this->GetPlotByID(this->GetPlotID(name));
}

//----------------------------------------------------------------------------
vtkMRMLPlotNode *vtkMRMLPlotLayoutNode::GetPlotByID(const char *id)
{
  for (int PlotIndex = 0 ; PlotIndex < this->plotNodes->GetNumberOfItems(); PlotIndex++)
    {
    vtkMRMLPlotNode* plotNode = vtkMRMLPlotNode::SafeDownCast(this->plotNodes->GetItemAsObject(PlotIndex));
    if (plotNode == NULL)
      {
      continue;
      }
    if (!strcmp(plotNode->GetID(), id))
      {
      return plotNode;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkStringArray* vtkMRMLPlotLayoutNode::GetPlotNames()
{
  PlotIDMap::iterator it;

  this->PlotNames->Initialize();
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end(); ++it)
    {
    this->PlotNames->InsertNextValue((*it).first);
    }

  return this->PlotNames;
}

//----------------------------------------------------------------------------
int vtkMRMLPlotLayoutNode::GetPlotNames(std::vector<std::string> &plotNodeNames)
{
  PlotIDMap::iterator it;

  plotNodeNames.clear();
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end(); ++it)
    {
    plotNodeNames.push_back((*it).first);
    }

  return static_cast<int>(plotNodeNames.size());
}

//----------------------------------------------------------------------------
vtkStringArray* vtkMRMLPlotLayoutNode::GetPlotIDs()
{
  PlotIDMap::iterator it;

  this->PlotIDs->Initialize();
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end(); ++it)
    {
    this->PlotIDs->InsertNextValue((*it).second);
    }

  return this->PlotIDs;
}

//----------------------------------------------------------------------------
int vtkMRMLPlotLayoutNode::GetPlotIDs(std::vector<std::string> &plotNodeIDs)
{
  PlotIDMap::iterator it;

  plotNodeIDs.clear();
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end(); ++it)
    {
    plotNodeIDs.push_back((*it).second);
    }

  return static_cast<int>(plotNodeIDs.size());
}

//----------------------------------------------------------------------------
vtkCollection *vtkMRMLPlotLayoutNode::GetPlotNodes()
{
  return (this->plotNodes ? this->plotNodes : NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::SetSelectionPlotNodeIDs(vtkStringArray *selectedPlotNodeIDs)
{
  if (this->selectionPlotNodeIDs == selectedPlotNodeIDs)
    {
    return;
    }

  this->selectionPlotNodeIDs = selectedPlotNodeIDs;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkStringArray *vtkMRMLPlotLayoutNode::GetSelectionPlotNodeIDs()
{
  return (this->selectionPlotNodeIDs ? this->selectionPlotNodeIDs : NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::SetSelectionArrays(vtkCollection *selectedArrays)
{
  if (this->selectionArrays == selectedArrays)
    {
    return;
    }

  this->selectionArrays = selectedArrays;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkCollection *vtkMRMLPlotLayoutNode::GetSelectionArrays()
{
  return (this->selectionArrays ? this->selectionArrays : NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::SetSelection(vtkStringArray *selectedPlotNodeIDs,
                                         vtkCollection *selectedArrays)
{
  if (this->selectionPlotNodeIDs == selectedPlotNodeIDs &&
      this->selectionArrays == selectedArrays)
    {
    return;
    }

  this->selectionPlotNodeIDs = selectedPlotNodeIDs;
  this->selectionArrays = selectedArrays;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkMRMLPlotLayoutNode::GetPlotNodeIndex(vtkMRMLPlotNode *plotNode)
{
  if (!plotNode)
    {
    return -1;
    }

  vtkIdType plotIndex = 0;

  std::string namePlotNode = plotNode->GetName();
  std::size_t found = namePlotNode.find("Markups");
  if (found == std::string::npos)
    {
    for (int ii = 0; ii < this->plotNodes->GetNumberOfItems(); ii++)
      {
      vtkMRMLPlotNode *plotNodeCol = vtkMRMLPlotNode::SafeDownCast
        (this->plotNodes->GetItemAsObject(ii));

      if (!plotNodeCol)
        {
        continue;
        }

      std::string namePlotNodeCol = plotNodeCol->GetName();
      std::size_t found = namePlotNodeCol.find("Markups");
      if (found == std::string::npos)
        {
        plotIndex++;
        }

      if (plotNode == plotNodeCol)
        {
        return plotIndex;
        }
      }
    }
  else
    {
    return this->GetPlotNodeIndex(vtkMRMLPlotNode::SafeDownCast
      (plotNode->GetNodeReference("Markups")));
    }
  return plotIndex;
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::SetProperty(const char *property,
                                        const char *value)
{
  PlotLayoutPropertyMap::iterator it;

  // check whether this property exist
  it = this->Properties->find(property);
  if (it != this->Properties->end())
    {
    // new value
    (*this->Properties)[property] = value;
    }
  else
    {
    // new property
    this->Properties->insert((std::make_pair(std::string(property), std::string(value))));
    }

  this->Modified();
}

//----------------------------------------------------------------------------
const char *vtkMRMLPlotLayoutNode::GetProperty(const char *property)
{
  PlotLayoutPropertyMap::iterator it = this->Properties->find(property);
  if (it == this->Properties->end())
    {
    return 0;
    }

  return (*it).second.c_str();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::ClearProperty(const char *property)
{
  PlotLayoutPropertyMap::iterator it = this->Properties->find(property);
  if (it == this->Properties->end())
    {
    return;
    }

  // erase the property
  this->Properties->erase(it);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::ClearProperties()
{
  // clear the entire property map
  this->Properties->clear();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::SetSceneReferences()
{
  if (this->GetScene() == NULL)
    {
    return;
    }

  this->Superclass::SetSceneReferences();
  // references in the Plot list
  PlotIDMap::iterator it;
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end(); ++it)
    {
    this->ObservePlot((*it).second.c_str());
  }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::UpdateReferences()
{
  Superclass::UpdateReferences();

  if (this->GetScene() == NULL)
    {
    return;
    }

  // PlotIDs
  PlotIDMap::iterator it;

  // create a separate Plot for removal of dangling references,
  // cannot remove inside the loop - inavlidates iterators.
  PlotIDMap MapPlotNamesIDsRemove;
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end();++it)
    {
    if (!this->GetScene()->GetNodeByID((*it).second.c_str()))
      {
      MapPlotNamesIDsRemove[(*it).first.c_str()] = (*it).second.c_str();
      }
    }
  // now remove dangling references
  for (it = MapPlotNamesIDsRemove.begin(); it != MapPlotNamesIDsRemove.end();++it)
    {
    this->RemovePlotAndObservationByName((*it).first.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID, newID);

  // Check to see if the old id is an Plot
  PlotIDMap::iterator it;
  for (it = this->MapPlotNamesIDs->begin(); it != this->MapPlotNamesIDs->end();++it)
    {
    if (!strcmp((*it).second.c_str(), oldID))
      {
      if (newID)
        {
        (*it).second = std::string(newID);
        if (this->GetScene())
          {
          this->GetScene()->RemoveReferencedNodeID(oldID, this);
          this->GetScene()->AddReferencedNodeID(newID, this);
          }
        }
      else
        {
        this->RemovePlotAndObservationByName((*it).first.c_str());
        }
      }
    }
}
