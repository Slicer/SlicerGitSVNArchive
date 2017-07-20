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

#ifndef __vtkMRMLPlotLayoutNode_h
#define __vtkMRMLPlotLayoutNode_h

#include "vtkMRMLNode.h"

class vtkCollection;
class vtkDataObject;
class vtkMRMLPlotNode;
class vtkStringArray;

class PlotIDMap;
class PlotLayoutPropertyMap;

#include <string>

/// \brief MRML node for referencing a collection of data to plot.
class VTK_MRML_EXPORT vtkMRMLPlotLayoutNode : public vtkMRMLNode
{
 public:
  //----------------------------------------------------------------
  /// Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLPlotLayoutNode *New();
  vtkTypeMacro(vtkMRMLPlotLayoutNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  ///
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts);

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "PlotLayout";};

  ///
  /// Method to propagate events generated in mrml
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData );

  //----------------------------------------------------------------
  /// Access methods
  //----------------------------------------------------------------

  ///
  /// Add and Observe a Plot to the PlotLayout. Parameter "name" is used for
  /// referencing the Plot when setting properties for plotting the
  /// Plot or for removing the Plot from the PlotLayout.
  virtual void AddAndObservePlot(const char *name, const char * id);

  ///
  /// Remove a Plot and Observation from the PlotLayout by a particular name
  virtual void RemovePlotAndObservationByName(const char *name);

  ///
  /// Remove a Plot and Observation from the PlotLayout by ID
  virtual void RemovePlotAndObservationByID(const char *id);

  ///
  /// Update all missing Observations
  virtual void UpdateObservations();

  ///
  /// Update a Plot Observation if is missing
  virtual void UpdateObservation(const char * id);

  ///
  /// Remove all the Plots
  virtual void ClearPlotIDs();

  ///
  /// Get the Plot id referenced by a particular name
  virtual const char* GetPlotID(const char *name);

  ///
  /// Get the Plot referenced by a particular name
  virtual vtkMRMLPlotNode* GetPlotbyName(const char *name);

  ///
  /// Get the Plot referenced by a particular id
  virtual vtkMRMLPlotNode* GetPlotByID(const char *id);

  ///
  /// Get the list of Plot names
  virtual vtkStringArray* GetPlotNames();
  virtual int GetPlotNames(std::vector<std::string> &plotNodeNames);

  ///
  /// Get the list of Plot ids
  virtual vtkStringArray* GetPlotIDs();
  virtual int GetPlotIDs(std::vector<std::string> &plotNodeIDs);

  ///
  /// Get the collection of PlotNodes
  virtual vtkCollection* GetPlotNodes();

  ///
  /// Set/Get the selected PlotNodeIDs
  virtual void SetSelectionPlotNodeIDs(vtkStringArray* selectedPlotNodeIDs);
  virtual vtkStringArray* GetSelectionPlotNodeIDs();

  ///
  /// Set/Get the collection of selectedIDArrays
  virtual void SetSelectionArrays(vtkCollection* selectedArrays);
  virtual vtkCollection *GetSelectionArrays();

  ///
  /// Set the PlotNodeIDs and selectedArrays
  virtual void SetSelection(vtkStringArray* selectedPlotNodeIDs,
                            vtkCollection* selectedArrays);

  ///
  /// Get the index of a PlotNode
  virtual vtkIdType GetPlotNodeIndex(vtkMRMLPlotNode* plotNode);

  ///
  /// Set/Get a property for a specific Plot to control how it will
  /// appear in the PlotLayout. If the Plot name is "default", then the property
  /// is either a property of the entire PlotLayout or a default property
  /// for the Plots (which can be overridden by properties assigned
  /// to specific Plots).  Available properties are:
  ///
  /// PlotLayout level properties
  ///
  /// \li  "type" - "Line", "Line and Scatter", "Scatter", "Bar"
  /// \li  "TitleName" - title displayed on the PlotLayout
  /// \li  "showTitle" - show title "on" or "off"
  /// \li  "XAxisLabelName" - label displayed on the x-axis
  /// \li  "showXAxisLabel" - show x-axis label "on" or "off"
  /// \li  "YAxisLabelName" - label displayed on the y-axis
  /// \li  "showYAxisLabel" - show y-axis label "on" or "off"
  /// \li  "showGrid" - show grid "on" or "off"
  /// \li  "showLegend" - show legend "on" or "off"
  /// \li  "FontType" - global Font for the PlotLayout: "Arial", "Times", "Courier"
  /// \li  "TitleFontSize" - default: "20"
  /// \li  "AxisTitleFontSize" - default: "16"
  /// \li  "AxisLabelFontSize" - default: "12"
  /// \li  "lookupTable" colorNodeID default: NULL
  ///

  ///
  /// Set/Get a property for the PlotLayoutNode
  virtual void SetProperty(const char *property, const char *value);
  virtual const char* GetProperty(const char *property);

  ///
  /// Remove a property for the PlotLayoutNode
  virtual void ClearProperty(const char *property);

  ///
  /// Remove all the properties for all the Plots
  virtual void ClearProperties();

  ///
  /// Updates this node if it depends on other nodes
  /// when the node is deleted in the scene
  virtual void SetSceneReferences();

  ///
  /// Updates this node if it depends on other nodes
  /// when the node is deleted in the scene
  virtual void UpdateReferences();

  ///
  /// Update the stored reference to another node in the scene
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  ///
  /// Events
  enum
  {
    vtkPlotRemovedEvent = 23000
  };

 protected:
  //----------------------------------------------------------------
  /// Constructor and destroctor
  //----------------------------------------------------------------
  vtkMRMLPlotLayoutNode();
  ~vtkMRMLPlotLayoutNode();
  vtkMRMLPlotLayoutNode(const vtkMRMLPlotLayoutNode&);
  void operator=(const vtkMRMLPlotLayoutNode&);

  ///
  /// Add a Plot to the PlotLayout. Parameter "name" is used for
  /// referencing the Plot when setting properties for plotting the
  /// Plot or for removing the Plot from the PlotLayout.
  virtual void AddPlot(const char *name, const char * id);

  ///
  /// Add the Plot to the plotNodes collection
  /// and start the Observation of the Plot.
  virtual void ObservePlot(const char * id);

  ///
  /// Remove a Plot Observation if is missing
  virtual void RemovePlotObservation(const char * id);

 protected:
  //----------------------------------------------------------------
  /// Data
  //----------------------------------------------------------------

  ///
  /// This map stores the names and the IDs of the Plot Node to observe
  PlotIDMap *MapPlotNamesIDs;

  ///
  /// Utility array for get methods (they are instantiate at the get call)
  vtkStringArray *PlotIDs;
  vtkStringArray *PlotNames;

  ///
  /// Both PlotLayout and Plots Properties are stored
  PlotLayoutPropertyMap *Properties;

  ///
  /// A collection of the pointers of the PlotNodes
  /// to observe. If the PlotNodeId is stored in MapPlotNamesIDs,
  /// but it is missing the relative Plot in plotNodes then it is needed to
  /// call UpdateObservations() to restore the observations.
  vtkSmartPointer<vtkCollection> plotNodes;

  ///
  ///
  /// Two collections of the pointers of the vktMRMLPlotNodes and vtkIdTypeArrays.
  /// They keep track of the user-selections in the chart View.
  vtkSmartPointer<vtkStringArray> selectionPlotNodeIDs;
  vtkSmartPointer<vtkCollection> selectionArrays;
};

#endif
