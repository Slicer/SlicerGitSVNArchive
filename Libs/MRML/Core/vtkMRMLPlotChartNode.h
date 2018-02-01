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

#ifndef __vtkMRMLPlotChartNode_h
#define __vtkMRMLPlotChartNode_h

#include "vtkMRMLNode.h"

class vtkCollection;
class vtkDataObject;
class vtkMRMLPlotDataNode;
class vtkStringArray;

#include <string>

/// \brief MRML node for referencing a collection of data to plot.
class VTK_MRML_EXPORT vtkMRMLPlotChartNode : public vtkMRMLNode
{
 public:
  //----------------------------------------------------------------
  /// Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLPlotChartNode *New();
  vtkTypeMacro(vtkMRMLPlotChartNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  ///
  /// Set node attributes.
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  ///
  /// Copy the node's attributes to this object.
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  ///
  /// Get node XML tag name (like Volume, Model).
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "PlotChart";};

  ///
  /// Method to propagate events generated in mrml.
  virtual void ProcessMRMLEvents(vtkObject *caller,
                                 unsigned long event,
                                 void *callData) VTK_OVERRIDE;

  /// PlotModifiedEvent is fired when:
  ///  - a new plot node is observed
  ///  - a plot node is not longer observed
  ///  - an associated plot node is modified
  /// Note that when SetAndObserve(Nth)NodeID() is called with an ID that
  /// has not yet any associated plot node in the scene, then
  /// plotModifiedEvent is not fired until found for the first time in
  /// the scene, e.g. Get(Nth)PlotDataNode(), UpdateScene()...
  enum
    {
    PlotModifiedEvent = 17000,
    };

  /// Properties used by SetPropertyToAllPlotDataNodes() and GetPropertyFromAllPlotDataNodes() methods.
  enum PlotDataNodeProperty
  {
    PlotType,
    PlotXColumnName,
    PlotYColumnName,
    PlotMarkerStyle
  };

  //----------------------------------------------------------------
  /// Access methods
  //----------------------------------------------------------------

  ///
  /// Convenience method that returns the ID of the first plot data node in the chart.
  /// \sa GetNthPlotDataNodeID(int), GetPlotDataNode()
  const char *GetPlotDataNodeID();

  ///
  /// Convenience method that returns the first plot data node.
  /// \sa GetNthPlotDataNode(int), GetPlotDataNodeID()
  vtkMRMLPlotDataNode* GetPlotDataNode();

  ///
  /// Return the ID of n-th plot data node ID. Or 0 if no such node exist.
  const char *GetNthPlotDataNodeID(int n);

  ///
  /// Get associated plot data node. Can be 0 in temporary states; e.g. if
  /// the plot node has no scene, or if the associated plot is not
  /// yet into the scene.
  vtkMRMLPlotDataNode* GetNthPlotDataNode(int n);

  ///
  /// Return the index of the Nth plot node ID.
  /// If not found, it returns -1.
  int GetPlotDataNodeIndexFromID(const char* plotDataNodeID);

  ///
  /// Get IDs of all associated plot data nodes.
  virtual int GetPlotDataNodeIDs(std::vector<std::string> &plotDataNodeIDs);

  ///
  /// Get names of all associated plot data nodes.
  virtual int GetPlotDataNodeNames(std::vector<std::string> &plotDataNodeNames);

  ///
  /// Return the number of plot node IDs (and plot nodes as they always
  /// have the same size).
  int GetNumberOfPlotDataNodes();

  ///
  /// Adds a plot data node to the chart.
  /// \sa SetAndObserverNthPlotDataNodeID(int, const char*)
  void AddAndObservePlotDataNodeID(const char *plotDataNodeID);

  ///
  /// Convenience method that sets the first plot data node in the chart.
  /// \sa SetAndObserverNthPlotDataNodeID(int, const char*)
  void SetAndObservePlotDataNodeID(const char *plotDataNodeID);

  ///
  /// Set and observe the Nth plot data node ID in the list.
  /// If n is larger than the number of plot nodes, the plot node ID
  /// is added at the end of the list. If PlotDataNodeID is 0, the node ID is
  /// removed from the list.
  /// \sa SetAndObservePlotDataNodeID(const char*),
  /// AddAndObservePlotDataNodeID(const char *), RemoveNthPlotDataNodeID(int)
  void SetAndObserveNthPlotDataNodeID(int n, const char *plotDataNodeID);

  ///
  /// Removes a plot data node from the chart.
  /// \sa SetAndObserverNthPlotDataNodeID(int, const char*)
  void RemovePlotDataNodeID(const char *plotDataNodeID);

  ///
  /// Removes n-th plot data node from the chart.
  /// \sa SetAndObserverNthPlotDataNodeID(int, const char*)
  void RemoveNthPlotDataNodeID(int n);

  ///
  /// Remove all plot data nodes from the chart.
  void RemoveAllPlotDataNodeIDs();

  ///
  /// Return true if PlotDataNodeID is in the plot node ID list.
  bool HasPlotDataNodeID(const char* plotDataNodeID);


  /// Title of the chart
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);

  /// Title font size. Default: 20.
  vtkSetMacro(TitleFontSize, int);
  vtkGetMacro(TitleFontSize, int);

  /// Show title of the chart
  vtkBooleanMacro(TitleVisibility, bool);
  vtkGetMacro(TitleVisibility, bool);
  vtkSetMacro(TitleVisibility, bool);

  /// Show horizontal and vertical grid lines
  vtkBooleanMacro(GridVisibility, bool);
  vtkGetMacro(GridVisibility, bool);
  vtkSetMacro(GridVisibility, bool);

  /// Show horizontal and vertical grid lines
  vtkBooleanMacro(LegendVisibility, bool);
  vtkGetMacro(LegendVisibility, bool);
  vtkSetMacro(LegendVisibility, bool);

  /// Title of X axis
  vtkSetStringMacro(XAxisTitle);
  vtkGetStringMacro(XAxisTitle);

  /// Show title of X axis
  vtkBooleanMacro(XAxisTitleVisibility, bool);
  vtkGetMacro(XAxisTitleVisibility, bool);
  vtkSetMacro(XAxisTitleVisibility, bool);

  /// Title of Y axis
  vtkSetStringMacro(YAxisTitle);
  vtkGetStringMacro(YAxisTitle);

  /// Show title of Y axis
  vtkBooleanMacro(YAxisTitleVisibility, bool);
  vtkGetMacro(YAxisTitleVisibility, bool);
  vtkSetMacro(YAxisTitleVisibility, bool);

  /// Axis title font size. Default: 16.
  vtkSetMacro(AxisTitleFontSize, int);
  vtkGetMacro(AxisTitleFontSize, int);

  /// Axis label font size. Default: 12.
  vtkSetMacro(AxisLabelFontSize, int);
  vtkGetMacro(AxisLabelFontSize, int);

  /// Font type for all text in the chart: "Arial", "Times", "Courier"
  vtkSetStringMacro(FontType);
  vtkGetStringMacro(FontType);

  /// Enable click and drag along X axis
  vtkBooleanMacro(ClickAndDragAlongX, bool);
  vtkGetMacro(ClickAndDragAlongX, bool);
  vtkSetMacro(ClickAndDragAlongX, bool);

  /// Enable click and drag along Y axis
  vtkBooleanMacro(ClickAndDragAlongY, bool);
  vtkGetMacro(ClickAndDragAlongY, bool);
  vtkSetMacro(ClickAndDragAlongY, bool);

  /// Node reference role used for storing plot data node references
  virtual const char* GetPlotDataNodeReferenceRole();

  /// Helper function to set common properties for all associated plot data nodes
  void SetPropertyToAllPlotDataNodes(PlotDataNodeProperty plotProperty, const char* value);

  /// Helper function to get common properties from all associated plot data nodes.
  /// Returns false if property is not the same in all plots.
  /// value contains the value found in the first plot data node.
  bool GetPropertyFromAllPlotDataNodes(PlotDataNodeProperty plotProperty, std::string& value);

 protected:
  //----------------------------------------------------------------
  /// Constructor and destructor
  //----------------------------------------------------------------
  vtkMRMLPlotChartNode();
  ~vtkMRMLPlotChartNode();
  vtkMRMLPlotChartNode(const vtkMRMLPlotChartNode&);
  void operator=(const vtkMRMLPlotChartNode&);

  ///
  /// Called when a node reference ID is added (list size increased).
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference) VTK_OVERRIDE;

  ///
  /// Called when a node reference ID is modified.
  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference) VTK_OVERRIDE;

  ///
  /// Called after a node reference ID is removed (list size decreased).
  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference *reference) VTK_OVERRIDE;

  static const char* PlotDataNodeReferenceRole;

  char *Title;
  int TitleFontSize;
  bool TitleVisibility;
  bool GridVisibility;
  bool LegendVisibility;
  char* XAxisTitle;
  bool XAxisTitleVisibility;
  char* YAxisTitle;
  bool YAxisTitleVisibility;
  int AxisTitleFontSize;
  int AxisLabelFontSize;
  char* FontType;
  bool ClickAndDragAlongX;
  bool ClickAndDragAlongY;
};

#endif
