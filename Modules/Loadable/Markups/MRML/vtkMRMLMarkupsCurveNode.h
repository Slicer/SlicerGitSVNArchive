/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __vtkMRMLMarkupsCurveNode_h
#define __vtkMRMLMarkupsCurveNode_h

// MRML includes
#include "vtkMRMLDisplayableNode.h"

// Markups includes
#include "vtkSlicerMarkupsModuleMRMLExport.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsNode.h"

/// \brief MRML node to represent an curve markup
/// Curve Markups nodes contain N control points.
/// Visualization parameters are set in the vtkMRMLMarkupsDisplayNode class.
///
/// Markups is intended to be used for manual marking/editing of point positions.
///
/// \ingroup Slicer_QtModules_Markups
class  VTK_SLICER_MARKUPS_MODULE_MRML_EXPORT vtkMRMLMarkupsCurveNode : public vtkMRMLMarkupsNode
{
public:
  static vtkMRMLMarkupsCurveNode *New();
  vtkTypeMacro(vtkMRMLMarkupsCurveNode,vtkMRMLMarkupsNode);
  /// Print out the node information to the output stream
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char* GetIcon() VTK_OVERRIDE {return ":/Icons/MarkupsCurveMouseModePlace.png";}

  //--------------------------------------------------------------------------
  // MRMLNode methods
  //--------------------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "MarkupsCurve";}

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  /// Calls the superclass UpdateScene
  void UpdateScene(vtkMRMLScene *scene) VTK_OVERRIDE;

  /// Alternative method to propagate events generated in Display nodes
  virtual void ProcessMRMLEvents ( vtkObject * /*caller*/,
                                   unsigned long /*event*/,
                                   void * /*callData*/ ) VTK_OVERRIDE;


  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode() VTK_OVERRIDE;

  /// Create and observe default display node(s)
  virtual void CreateDefaultDisplayNodes() VTK_OVERRIDE;

  /// Get the number of Points in this node (maximum number of point is 3)
  int GetNumberOfPoints() { return this->GetNumberOfControlPoints(); } ;
  /// Add a new Point from x,y,z coordinates and return the Point index,
  /// if point index is -1, the point has not been added
  int AddPoint(double x, double y, double z);
  int AddPoint(double x, double y, double z, std::string label);
  /// Add a new Point from an array and return the Point index,
  /// if point index is -1, the point has not been added
  int AddPointFromArray(double pos[3], std::string label = std::string());
  /// Get the position of the nth Point, returning it in the pos array
  void GetNthPointPosition(int n, double pos[3]);
  /// Set the position of the nth Point from x, y, z coordinates
  void SetNthPointPosition(int n, double x, double y, double z);
  /// Set the position of the nth Point from a double array
  void SetNthPointPositionFromArray(int n, double pos[3]);
  /// Get selected property on Nth Point
  bool GetNthPointSelected(int n = 0);
  /// Set selected property on Nth Point
  void SetNthPointSelected(int n, bool flag);
  /// Get locked property on Nth Point
  bool GetNthPointLocked(int n = 0);
  /// Set locked property on Nth Point
  void SetNthPointLocked(int n, bool flag);
  /// Get visibility property on Nth Point
  bool GetNthPointVisibility(int n = 0);
  /// Set visibility property on Nth Point. If the visibility is set to
  /// true on the node/list as a whole, the nth Point visibility is used to
  /// determine if it is visible. If the visibility is set to false on the node
  /// as a whole, all Points are hidden but keep this value for when the
  /// list as a whole is turned visible.
  /// \sa vtkMRMLDisplayableNode::SetDisplayVisibility
  /// \sa vtkMRMLDisplayNode::SetVisibility
  void SetNthPointVisibility(int n, bool flag);
  /// Get label on nth Point
  std::string GetNthPointLabel(int n = 0);
  /// Set label on nth Point
  void SetNthPointLabel(int n, std::string label);
  /// Get associated node id on nth Point
  std::string GetNthPointAssociatedNodeID(int n = 0);
  /// Set associated node id on nth Point
  void SetNthPointAssociatedNodeID(int n, const char* id);
  /// Set world coordinates on nth Point
  void SetNthPointWorldCoordinates(int n, double coords[4]);
  /// Get world coordinates on nth Point
  void GetNthPointWorldCoordinates(int n, double coords[4]);

  virtual void GetRASBounds(double bounds[6]) VTK_OVERRIDE;
  virtual void GetBounds(double bounds[6]) VTK_OVERRIDE;

protected:
  vtkMRMLMarkupsCurveNode();
  ~vtkMRMLMarkupsCurveNode();
  vtkMRMLMarkupsCurveNode(const vtkMRMLMarkupsCurveNode&);
  void operator=(const vtkMRMLMarkupsCurveNode&);

};

#endif
