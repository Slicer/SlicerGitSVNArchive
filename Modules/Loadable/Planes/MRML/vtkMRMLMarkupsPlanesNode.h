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

#ifndef __vtkMRMLMarkupsPlanesNode_h
#define __vtkMRMLMarkupsPlanesNode_h

// MRML includes
#include <vtkMRMLMarkupsNode.h>

// Markups includes
#include "vtkSlicerPlanesModuleMRMLExport.h"

// VTK includes
#include <vtkSmartPointer.h>

class vtkMRMLMarkupsDisplayNode;

//
//
class  VTK_SLICER_PLANES_MODULE_MRML_EXPORT vtkMRMLMarkupsPlanesNode : public vtkMRMLMarkupsNode
{
public:
  static vtkMRMLMarkupsPlanesNode *New();
  vtkTypeMacro(vtkMRMLMarkupsPlanesNode,vtkMRMLMarkupsNode);
  /// Print out the node information to the output stream
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual const char* GetIcon() {return ":/Icons/MarkupsPlaneMouseModePlace.png";};

  //--------------------------------------------------------------------------
  // MRMLNode methods
  //--------------------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance();
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "MarkupsPlane";};

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// Calls the superclass UpdateScene
  void UpdateScene(vtkMRMLScene *scene);

  /// Alternative method to propagate events generated in Display nodes
  virtual void ProcessMRMLEvents ( vtkObject * /*caller*/,
                                   unsigned long /*event*/,
                                   void * /*callData*/ );


  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode();

  /// Create and observe default display node(s)
  virtual void CreateDefaultDisplayNodes();

  /// Return a cast display node, returns null if none
  vtkMRMLMarkupsDisplayNode *GetMarkupsDisplayNode();

  // Wrapping some of the generic markup methods
  enum
    {
    ORIGIN_INDEX = 0,
    NORMAL_INDEX,
    BOUND_MIN_INDEX,
    BOUND_MAX_INDEX,
    } PlanePointIndexes;

  /// Add a new plane return the plane index
  int AddPlane(double ox, double oy, double oz,
               double nx, double ny, double nz,
               double xmin, double ymin, double zmin,
               double xmax, double ymax, double zmax,
               std::string label = std::string());

  /// Add a new fiducial from an array and return the plane index
  int AddPlaneFromArray(
    double origin[3], double normal[3], double min[3], double max[3],
    std::string label = std::string());

  /// Get the position of the nth plane origin or normal or bound.
  void GetNthPlaneOrigin(int n, double origin[3]);
  void GetNthPlaneNormal(int n, double normal[3]);
  void GetNthPlaneBoundMinimum(int n, double bound[3]);
  void GetNthPlaneBoundMaximum(int n, double bound[3]);

  /// Set the position of the nth plane origin or normal or bound from x, y, z coordinates
  void SetNthPlaneOrigin(int n, double x, double y, double z);
  void SetNthPlaneNormal(int n, double x, double y, double z);
  void SetNthPlaneBoundMinimum(int n, double x, double y, double z);
  void SetNthPlaneBoundMaximum(int n, double x, double y, double z);

  /// Set the position of the nth plane normal or origin from a double array
  void SetNthPlaneOriginFromArray(int n, double pos[3]);
  void SetNthPlaneNormalFromArray(int n, double normal[3]);
  void SetNthPlaneBoundMinimumFromArray(int n, double bound[3]);
  void SetNthPlaneBoundMaximumFromArray(int n, double bound[3]);

  /// Set/Get selected property on Nth plane
  bool GetNthPlaneSelected(int n = 0);
  void SetNthPlaneSelected(int n, bool selected);

  /// Set/Get visibility property on Nth fiducial. If the visibility is set to
  /// true on the node/list as a whole, the nth fiducial visibility is used to
  /// determine if it is visible. If the visibility is set to false on the node
  /// as a whole, all fiducials are hidden but keep this value for when the
  /// list as a whole is turned visible.
  /// \sa vtkMRMLDisplayableNode::SetDisplayVisibility
  /// \sa vtkMRMLDisplayNode::SetVisibility
  void SetNthPlaneVisibility(int n, bool flag);
  bool GetNthPlaneVisibility(int n = 0);

  /// Set/Get label on nth plane
  std::string GetNthPlaneLabel(int n = 0);
  void SetNthPlaneLabel(int n, std::string label);

  /// Set/Get associated node id on nth plane
  std::string GetNthPlaneAssociatedNodeID(int n = 0);
  void SetNthPlaneAssociatedNodeID(int n, const char* id);

  /// Set/Get world coordinates on nth plane normal or origin
  void SetNthPlaneOriginWorldCoordinates(int n, double coords[4]);
  void GetNthPlaneOriginWorldCoordinates(int n, double coords[4]);
  void SetNthPlaneBoundMinimumWorldCoordinates(int n, double coords[4]);
  void GetNthPlaneBoundMinimumWorldCoordinates(int n, double coords[4]);
  void SetNthPlaneBoundMaximumWorldCoordinates(int n, double coords[4]);
  void GetNthPlaneBoundMaximumWorldCoordinates(int n, double coords[4]);

  virtual void GetRASBounds(double bounds[6]);
  virtual void GetBounds(double bounds[6]);

protected:
  vtkMRMLMarkupsPlanesNode();
  ~vtkMRMLMarkupsPlanesNode();
  vtkMRMLMarkupsPlanesNode(const vtkMRMLMarkupsPlanesNode&);
  void operator=(const vtkMRMLMarkupsPlanesNode&);

};

#endif
