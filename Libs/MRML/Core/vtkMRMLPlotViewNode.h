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

#ifndef __vtkMRMLPlotViewNode_h
#define __vtkMRMLPlotViewNode_h

#include "vtkMRMLAbstractViewNode.h"

/// \brief MRML node to represent Plot view parameters.
///
/// PlotViewNodes are associated one to one with a PlotWidget.
class VTK_MRML_EXPORT vtkMRMLPlotViewNode : public vtkMRMLAbstractViewNode
{
public:
  static vtkMRMLPlotViewNode *New();
  vtkTypeMacro(vtkMRMLPlotViewNode, vtkMRMLAbstractViewNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //--------------------------------------------------------------------------
  /// MRMLNode methods
  //--------------------------------------------------------------------------

   virtual vtkMRMLNode* CreateNodeInstance();

  ///
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() { return "PlotView"; };

  ///
  /// Set and Update the PlotLayout node id displayed in this Plot View
  virtual void SetAndUpdatePlotLayoutNodeID(const char *PlotLayoutNodeID);

  ///
  /// Set and Update the PlotLayout node id displayed in this Plot View
  /// Utility method that conveniently takes a string instead of a char*
  virtual void SetAndUpdatePlotLayoutNodeID(const std::string& PlotLayoutNodeID);

  ///
  /// Get the Plot node id displayed in this Plot View
  vtkGetStringMacro(PlotLayoutNodeID);

  ///
  /// Configures the behavior of PropagatePlotLayoutSelection().
  /// If DoPropagatePlotLayoutSelection set to false then this
  /// view will not be affected by PropagatePlotLayoutSelection.
  /// Default value is true.
  vtkSetMacro (DoPropagatePlotLayoutSelection, bool );
  vtkGetMacro (DoPropagatePlotLayoutSelection, bool );

  ///
  /// Mark the PlotLayout node as references.
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
    PlotLayoutNodeChangedEvent = 18000
  };

protected:
  vtkMRMLPlotViewNode();
  ~vtkMRMLPlotViewNode();
  vtkMRMLPlotViewNode(const vtkMRMLPlotViewNode&);
  void operator=(const vtkMRMLPlotViewNode&);

  char* PlotLayoutNodeID;
  bool DoPropagatePlotLayoutSelection;

private:
  ///
  /// Set the PlotLayout node id displayed in this Plot View
  void SetPlotLayoutNodeID(const char* id);
};

#endif
