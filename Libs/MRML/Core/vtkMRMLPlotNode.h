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

#ifndef __vtkMRMLPlotNode_h
#define __vtkMRMLPlotNode_h

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

/// \brief MRML node to represent a Plot object
///
class VTK_MRML_EXPORT vtkMRMLPlotNode : public vtkMRMLNode
{
public:
  /// Data types supported by the Plot. Used in qMRMLPlotModel for visualization.

public:
  static vtkMRMLPlotNode *New();
  vtkTypeMacro(vtkMRMLPlotNode,vtkMRMLNode);

  // Description:
  // Enum of the available plot types
  enum {
    LINE,
    POINTS,
    BAR,
  };

  void PrintSelf(ostream& os, vtkIndent indent);

  //----------------------------------------------------------------
  /// Standard methods for MRML nodes
  //----------------------------------------------------------------

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
  /// Copy the node's attributes to this object but changing the name and type
  virtual void CopyAndSetNameAndType(vtkMRMLNode *node, const char *name, int Type);

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() { return "Plot"; };

  ///
  /// Method to propagate events generated in mrml
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData);

  ///
  /// Mark the Table node as references.
  virtual void SetSceneReferences();

  ///
  /// Updates this node if it depends on other nodes
  /// when the node is deleted in the scene.
  virtual void UpdateReferences();

  ///
  /// Update the stored reference to another node in the scene.
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  //----------------------------------------------------------------
  /// Get and Set Macros
  //----------------------------------------------------------------

  ///
  /// Set and observe a vtkPlot.
  /// \sa SetInputData()
  virtual void SetAndObservePlot(vtkPlot* plot);

  ///
  /// \brief vtkGetObjectMacro
  /// Get observed plot
  vtkGetObjectMacro(Plot, vtkPlot);

  ///
  /// Set and observe Table node ID.
  /// \sa TableNodeID, GetTableNodeID(), SetInputData()
  virtual void SetAndObserveTableNodeID(const char *TableNodeID);

  /// Set and observe Table node ID.
  /// Utility method that conveniently takes a string instead of a char*
  /// \sa TableNodeID, GetTableNodeID(), SetInputData()
  virtual void SetAndObserveTableNodeID(const std::string& TableNodeID);

  /// Get Table node ID.
  /// \sa TableNodeID, SetAndObserveTableNodeID()
  vtkGetStringMacro(TableNodeID);

  /// Get associated Table MRML node. Search the node into the scene if the node
  /// hasn't been cached yet. This can be a slow call.
  /// \sa TableNodeID, SetAndObserveTableNodeID, GetTableNodeID()
  virtual vtkMRMLTableNode* GetTableNode();

  /// Get the type of the plot (line, scatter, bar)
  /// \brief vtkGetMacro
  /// \sa SetTypeAndColor
  vtkGetMacro(Type, int);

  ///
  /// Set the type of the plot (line, scatter, bar)
  virtual void SetType(int type);

  ///
  /// Get/Set the status of the Plot
  vtkGetMacro(Dirty, bool);
  vtkSetMacro(Dirty, bool);

  ///
  /// Get the index of the XColumn
  /// \brief vtkGetMacro
  vtkGetMacro(XColumn, vtkIdType);

  ///
  /// Set the index of the XColumn and assure the data connection
  /// \brief vtkSetMacro
  /// \sa SetInputData
  virtual void SetXColumn(vtkIdType xColumn);

  ///
  /// Utility method to get the name of the columnX.
  /// The name is automatically set internally
  virtual std::string GetXColumnName();

  ///
  /// Get the index of the YColumn
  /// \brief vtkGetMacro
  vtkGetMacro(YColumn, vtkIdType);

  ///
  /// Set the index of the YColumn and assure the data connection
  /// \brief vtkSetMacro
  /// \sa SetInputData
  virtual void SetYColumn(vtkIdType yColumn);

  ///
  /// Utility method to get the name of the columnY.
  /// The name is automatically set internally
  virtual std::string GetYColumnName();

  ///
  /// Events
  enum
  {
    vtkPlotRemovedEvent = 23000
  };

  //----------------------------------------------------------------
  /// Constructor and destructor
  //----------------------------------------------------------------
 protected:
  vtkMRMLPlotNode();
  ~vtkMRMLPlotNode();
  vtkMRMLPlotNode(const vtkMRMLPlotNode&);
  void operator=(const vtkMRMLPlotNode&);

  /// Set input data from a vtkTable.
  /// This method is called internally everytime
  /// that a new vtkPlot or vtkMRMLTable has been
  /// set and observed.
  /// \sa vtkPlot->SetInputData(), SetXColumnName(), SetYColumnName()
  virtual void SetInputData(vtkMRMLTableNode* tableNode, vtkIdType xColumn, vtkIdType yColumn);

  /// Utility method for setting InputData without
  /// providing the xColumn and yColumn parameters
  /// \sa GetXColumn(), GetYColumn()
  /// \def default are 0, 1
  virtual void SetInputData(vtkMRMLTableNode* tableNode);

  ///
  /// Internal utility methods to store the name of the columns
  virtual void SetXColumnName(const std::string& xColumnName);
  virtual void SetYColumnName(const std::string& yColumnName);

  //----------------------------------------------------------------
  /// Data
  //----------------------------------------------------------------
 protected:
  vtkPlot* Plot;

  ///
  /// Type of Plot (line, scatter, bar)
  int Type;
  ///
  /// Plot require to be update in the chartXY
  /// (after a change of Type this is set to true)
  bool Dirty;

  vtkIdType XColumn;
  vtkIdType YColumn;

  std::string XColumnName;
  std::string YColumnName;

  ///
  /// String ID of the table MRML node.
  /// Note that anytime the Table node is modified, the observing display node
  /// fires a Modified event.
  /// No Table node by default.
  /// \sa SetTableNodeID(), GetTableNodeID(),
  /// TableNode
  char *TableNodeID;

  vtkMRMLTableNode *TableNode;

private:
  void SetTableNodeID(const char* id);
};

#endif
