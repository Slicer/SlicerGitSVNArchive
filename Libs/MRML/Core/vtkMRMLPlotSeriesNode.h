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

#ifndef __vtkMRMLPlotSeriesNode_h
#define __vtkMRMLPlotSeriesNode_h

#include <string>
#include <vector>

// MRML Includes
#include "vtkMRMLNode.h"

class vtkMRMLStorageNode;
class vtkMRMLTableNode;

// VTK Includes
class vtkColor4ub;
class vtkPlot;
class vtkTable;

/// \brief MRML node to represent a vtkPlot object
///
/// Plot nodes describe the properties of a single plot:
/// input data (one or two columns of a table node),
/// and display properties (plot type, marker style, color, etc).
class VTK_MRML_EXPORT vtkMRMLPlotSeriesNode : public vtkMRMLNode
{
public:
  static vtkMRMLPlotSeriesNode *New();
  vtkTypeMacro(vtkMRMLPlotSeriesNode,vtkMRMLNode);

  // Description:
  // Enum of the available plot types
  enum {
    SCATTER,
    LINE,
    BAR,
    PLOT_TYPE_LAST // must be last
  };

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //----------------------------------------------------------------
  /// Standard methods for MRML nodes
  //----------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  ///
  /// Set node attributes.
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  ///
  /// Get node XML tag name (like Volume, Model).
  virtual const char* GetNodeTagName() VTK_OVERRIDE { return "PlotSeries"; };

  ///
  /// Set and observe Table node ID.
  /// \sa TableNodeID, GetTableNodeID(), SetInputData()
  virtual void SetAndObserveTableNodeID(const char *tableNodeID);

  ///
  /// Set and observe Table node ID.
  /// Utility method that conveniently takes a string instead of a char*.
  /// \sa TableNodeID, GetTableNodeID(), SetInputData()
  virtual void SetAndObserveTableNodeID(const std::string& tableNodeID);

  ///
  /// Get associated Table MRML noide.
  virtual vtkMRMLTableNode* GetTableNode();

  ///
  /// Method to propagate events generated in Plot nodes.
  virtual void ProcessMRMLEvents (vtkObject *caller,
                                  unsigned long event,
                                  void *callData) VTK_OVERRIDE;

  ///
  /// TableModifiedEvent is send when the parent table is modified
  enum
    {
      TableModifiedEvent = 15000
    };

  ///
  /// Get referenced transform node id
  const char *GetTableNodeID();

  //----------------------------------------------------------------
  /// Get and Set Macros
  //----------------------------------------------------------------

  /// Get/Set the type of the plot (scatter, line, bar).
  /// Scatter: X and Y columns define X and Y coordinates of points
  /// Line: line points are equally spaced along X axis, Y column defines height of each point
  /// Bar: vertical bars equally spaced along X axis, Y column defines height of each bar
  /// \brief vtkGetMacro
  vtkGetMacro(PlotType, int);
  vtkSetMacro(PlotType, int);

  ///
  /// Convenience method to set the type of
  /// the plot (scatter, line, bar) from strings.
  virtual void SetPlotType(const char* type);

  /// Get the name of the Y column in the referenced table (for scatter plots).
  vtkGetMacro(XColumnName, std::string);
  vtkSetMacro(XColumnName, std::string);

  ///
  /// Get the name of the Y column in the referenced table.
  vtkGetMacro(YColumnName, std::string);
  vtkSetMacro(YColumnName, std::string);

  /// Get/Set the name of the X (for scatter plot)
  /// or labels (for line or bar plot) column in the referenced table.
  vtkGetMacro(LabelColumnName, std::string);
  vtkSetMacro(LabelColumnName, std::string);

  ///
  /// Convert between plot type ID and name
  virtual const char *GetPlotTypeAsString(int id);
  virtual int GetPlotTypeFromString(const char *name);

  ///
  /// Utility methods to set/get the marker style
  /// available for Line and Points Plots.
  vtkGetMacro(MarkerStyle, int);
  vtkSetMacro(MarkerStyle, int);

  ///
  /// Convert between plot markers style ID and name
  const char *GetMarkerStyleAsString(int id);
  int GetMarkerStyleFromString(const char *name);

  ///
  /// Utility methods to set/get the marker size
  /// available for Line and Points Plots.
  vtkGetMacro(MarkerSize, float);
  vtkSetMacro(MarkerSize, float);

  ///
  /// Set/get the line style.
  /// Values defined in vtkPen (NO_PEN, SOLID_LINE, DASH_LINE,
  /// DOT_LINE, DASH_DOT_LINE, DASH_DOT_DOT_LINE).
  vtkGetMacro(LineStyle, int);
  vtkSetMacro(LineStyle, int);

  ///
  /// Convert between line style ID and name
  const char *GetLineStyleAsString(int id);
  int GetLineStyleFromString(const char *name);

  ///
  /// Utility methods to set/get the Line width
  /// available for Line Plots.
  vtkGetMacro(LineWidth, float);
  vtkSetMacro(LineWidth, float);

  ///
  /// Set/Get Color of the line and markers of the plot
  vtkGetVectorMacro(Color, double, 3);
  vtkSetVectorMacro(Color, double, 3);

  ///
  /// Get set line opacity
  vtkGetMacro(Opacity, double);
  vtkSetMacro(Opacity, double);

  //----------------------------------------------------------------
  /// Constructor and destructor
  //----------------------------------------------------------------
protected:
  vtkMRMLPlotSeriesNode();
  ~vtkMRMLPlotSeriesNode();
  vtkMRMLPlotSeriesNode(const vtkMRMLPlotSeriesNode&);
  void operator=(const vtkMRMLPlotSeriesNode&);

  static const char* TableNodeReferenceRole;
  static const char* TableNodeReferenceMRMLAttributeName;

  virtual const char* GetTableNodeReferenceRole();
  virtual const char* GetTableNodeReferenceMRMLAttributeName();

  ///
  /// Called when a node reference ID is added (list size increased).
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference) VTK_OVERRIDE
  {
    Superclass::OnNodeReferenceAdded(reference);
    if (std::string(reference->GetReferenceRole()) == this->TableNodeReferenceRole)
      {
      this->InvokeCustomModifiedEvent(vtkMRMLPlotSeriesNode::TableModifiedEvent, reference->GetReferencedNode());
      }
  }

  ///
  /// Called when a node reference ID is modified.
  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference) VTK_OVERRIDE
  {
    Superclass::OnNodeReferenceModified(reference);
    if (std::string(reference->GetReferenceRole()) == this->TableNodeReferenceRole)
    {
      this->InvokeCustomModifiedEvent(vtkMRMLPlotSeriesNode::TableModifiedEvent, reference->GetReferencedNode());
    }
  }

  ///
  /// Called after a node reference ID is removed (list size decreased).
  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference *reference) VTK_OVERRIDE
  {
    Superclass::OnNodeReferenceRemoved(reference);
    if (std::string(reference->GetReferenceRole()) == this->TableNodeReferenceRole)
    {
      this->InvokeCustomModifiedEvent(vtkMRMLPlotSeriesNode::TableModifiedEvent, reference->GetReferencedNode());
    }
  }

  ///
  /// Copy the node's attributes to this object
  /// This is used only internally.
  /// Externally CopyWithScene has to be called.
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  //----------------------------------------------------------------
  /// Data
  //----------------------------------------------------------------
 protected:

  ///
  /// Type of Plot (scatter, line, bar).
  int PlotType;

  std::string XColumnName;
  std::string YColumnName;
  std::string LabelColumnName;

  float LineWidth;
  int LineStyle;

  float MarkerSize;
  int MarkerStyle;

  double Color[3];
  double Opacity;
};

#endif
