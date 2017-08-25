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

class vtkMRMLPlotLayoutNode;

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

   virtual vtkMRMLNode* CreateNodeInstance()  VTK_OVERRIDE;

  ///
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE { return "PlotView"; };

  ///
  /// Set and Update the PlotLayout node id displayed in this Plot View
  virtual void SetPlotLayoutNodeID(const char *PlotLayoutNodeID);

  ///
  /// Get the PlotLayout node id displayed in this PlotLayout View
  const char* GetPlotLayoutNodeID();

  ///
  /// Get the PlotLayout node displayed in this PlotLayout View
  vtkMRMLPlotLayoutNode* GetPlotLayoutNode();

  ///
  /// Configures the behavior of PropagatePlotLayoutSelection().
  /// If DoPropagatePlotLayoutSelection set to false then this
  /// view will not be affected by PropagatePlotLayoutSelection.
  /// Default value is true.
  vtkSetMacro (DoPropagatePlotLayoutSelection, bool );
  vtkGetMacro (DoPropagatePlotLayoutSelection, bool );

  ///
  /// Method to propagate events generated in mrml
  virtual void ProcessMRMLEvents(vtkObject *caller,
                                 unsigned long event,
                                 void *callData) VTK_OVERRIDE;

  /// PlotModifiedEvent is fired when:
  ///  - a new plotLayout node is observed
  ///  - a plotLayout node is not longer observed
  ///  - an associated plotLayout node is modified
  /// Note that when SetAndObserve(Nth)NodeID() is called with an ID that
  /// has not yet any associated plot node in the scene, then
  /// plotModifiedEvent is not fired until found for the first time in
  /// the scene, e.g. Get(Nth)plotNode(), UpdateScene()...
  enum
  {
    PlotLayoutNodeChangedEvent = 18000
  };

  virtual const char* GetPlotLayoutNodeReferenceRole();

protected:
  vtkMRMLPlotViewNode();
  ~vtkMRMLPlotViewNode();
  vtkMRMLPlotViewNode(const vtkMRMLPlotViewNode&);
  void operator=(const vtkMRMLPlotViewNode&);

  virtual const char* GetPlotLayoutNodeReferenceMRMLAttributeName();

  static const char* PlotLayoutNodeReferenceRole;
  static const char* PlotLayoutNodeReferenceMRMLAttributeName;

  ///
  /// Called when a node reference ID is added (list size increased).
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference) VTK_OVERRIDE;

  ///
  /// Called when a node reference ID is modified.
  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference) VTK_OVERRIDE;

  ///
  /// Called after a node reference ID is removed (list size decreased).
  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference *reference) VTK_OVERRIDE;

  bool DoPropagatePlotLayoutSelection;
};

#endif
