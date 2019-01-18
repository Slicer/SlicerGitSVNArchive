/*=========================================================================

 Copyright (c) ProxSim ltd., Kwun Tong, Hong Kong. All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
 and development was supported by ProxSim ltd.

=========================================================================*/

/**
 * @class   vtkSlicerAbstractRepresentation
 * @brief   Default representation for the slicer markups widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerAbstractWidget. It works in conjunction with the
 * vtkSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerAbstractWidget
 * for details.
 *
 * Point picking is done using the vtkIncrementalOctreePointLocator
 * Line picking with vtkCellPicker (see child classes)
 *
 * @sa
 * vtkSlicerAbstractRepresentation vtkSlicerAbstractWidget vtkPointPlacer
 * vtkSlicerLineInterpolator
*/

#ifndef vtkSlicerAbstractRepresentation_h
#define vtkSlicerAbstractRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkWidgetRepresentation.h"

#include "vtkMRMLMarkupsNode.h"

#include <vector> // STL Header; Required for vector

class vtkPolyData;
class vtkPoints;

class vtkSlicerLineInterpolator;
class vtkIncrementalOctreePointLocator;
class vtkPointPlacer;
class vtkPolyData;
class vtkIdList;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractRepresentation : public vtkWidgetRepresentation
{
public:
  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerAbstractRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Add a node at a specific world position. Returns 0 if the
  /// node could not be added, 1 otherwise.
  virtual int AddNodeAtWorldPosition(double x, double y, double z);
  virtual int AddNodeAtWorldPosition(double worldPos[3]);

  /// Add a node at a specific display position. This will be
  /// converted into a world position according to the current
  /// constraints of the point placer. Return 0 if a point could
  /// not be added, 1 otherwise.
  virtual int AddNodeAtDisplayPosition(double displayPos[2]);
  virtual int AddNodeAtDisplayPosition(int displayPos[2]);
  virtual int AddNodeAtDisplayPosition(int X, int Y);

  /// Given a display position, activate a node. The closest
  /// node within tolerance will be activated. If a node is
  /// activated, 1 will be returned, otherwise 0 will be
  /// returned.
  virtual int ActivateNode(double displayPos[2]);
  virtual int ActivateNode(int displayPos[2]);
  virtual int ActivateNode(int X, int Y);

  /// Move the active node to a specified world position.
  /// Will return 0 if there is no active node or the node
  /// could not be moved to that position. 1 will be returned
  /// on success.
  virtual int SetActiveNodeToWorldPosition(double pos[3]);

  /// Move the active node based on a specified display position.
  /// The display position will be converted into a world
  /// position. If the new position is not valid or there is
  /// no active node, a 0 will be returned. Otherwise, on
  /// success a 1 will be returned.
  virtual int SetActiveNodeToDisplayPosition(double pos[2]);
  virtual int SetActiveNodeToDisplayPosition(int pos[2]);
  virtual int SetActiveNodeToDisplayPosition(int X, int Y);
  //@}

  /// Get/Set the active node.
  virtual int GetActiveNode();
  virtual void SetActiveNode(int index);

  /// Get the world position of the active node. Will return
  /// 0 if there is no active node, or 1 otherwise.
  virtual int GetActiveNodeWorldPosition(double pos[3]);

  /// Get the display position of the active node. Will return
  /// 0 if there is no active node, or 1 otherwise.
  virtual int GetActiveNodeDisplayPosition(double pos[2]);

  /// Set the active node's visibility.
  virtual void SetActiveNodeVisibility(bool visibility);

  /// Get the active node's visibility.
  virtual bool GetActiveNodeVisibility();

  /// Set if the active node is selected.
  virtual void SetActiveNodeSelected(bool selected);

  /// Get if the active node is locked.
  virtual bool GetActiveNodeSelected();

  /// Set if the active node is locked.
  virtual void SetActiveNodeLocked(bool locked);

  /// Get if the active node is locked.
  virtual bool GetActiveNodeLocked();

  /// Set the active node's orietation.
  virtual void SetActiveNodeOrientation(double orientation[4]);

  /// Get the active node's orietation.
  virtual void GetActiveNodeOrientation(double orientation[4]);

  /// Set the active node's label.
  virtual void SetActiveNodeLabel(vtkStdString label);

  /// Get the active node's label.
  virtual vtkStdString GetActiveNodeLabel();

  /// Get the number of nodes.
  virtual int GetNumberOfNodes();

  /// Get the nth node's display position. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  virtual int GetNthNodeDisplayPosition(int n, double pos[2]);

  /// Get the nth node's world position. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  virtual int GetNthNodeWorldPosition(int n, double pos[3]);

  /// Get the nth node's visibility.
  virtual bool GetNthNodeVisibility(int n);

  /// Set the nth node's visibility.
  virtual void SetNthNodeVisibility(int n, bool visibility);

  /// Get the if nth node is selected.
  virtual bool GetNthNodeSelected(int n);

  /// Set the nth node is selected.
  virtual void SetNthNodeSelected(int n, bool selected);

  /// Get if the nth node is locked.
  virtual bool GetNthNodeLocked(int n);

  /// Set if the nth node is locked.
  virtual void SetNthNodeLocked(int n, bool locked);

  /// Set the nth node's orietation.
  virtual void SetNthNodeOrientation(int n , double orientation[4]);

  /// Get the nth node's orietation.
  virtual void GetNthNodeOrientation(int n , double orientation[4]);

  /// Set the nth node's label.
  virtual void SetNthNodeLabel(int n , vtkStdString label);

  /// Get the nth node's label.
  virtual vtkStdString GetNthNodeLabel(int n);

  /// Get the nth node.
  virtual ControlPoint *GetNthNode(int n);

  /// Return true if n is a valid node, false otherwise
  bool NodeExists(int n);

  /// Set the nth node's display position. Display position
  /// will be converted into world position according to the
  /// constraints of the point placer. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting) or the world position
  /// is not valid.
  virtual int SetNthNodeDisplayPosition(int n, int X, int Y);
  virtual int SetNthNodeDisplayPosition(int n, int pos[2]);
  virtual int SetNthNodeDisplayPosition(int n, double pos[2]);

  /// Set the nth node's world position. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting) or the world
  /// position is not valid according to the point
  /// placer.
  virtual int SetNthNodeWorldPosition(int n, double pos[3]);

  /// Get the nth node's slope. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  virtual int  GetNthNodeSlope(int idx, double slope[3]);

  /// For a given node n, get the number of intermediate
  /// points between this node and the node at
  /// (n+1). If n is the last node and the loop is
  /// closed, this is the number of intermediate points
  /// between node n and node 0. 0 is returned if n is
  /// out of range.
  virtual int GetNumberOfIntermediatePoints(int n);

  /// Get the world position of the intermediate point at
  /// index idx between nodes n and (n+1) (or n and 0 if
  /// n is the last node and the loop is closed). Returns
  /// 1 on success or 0 if n or idx are out of range.
  virtual int GetIntermediatePointWorldPosition(int n,
                                                int idx, double point[3]);

  /// Get the display position of the intermediate point at
  /// index idx between nodes n and (n+1) (or n and 0 if
  /// n is the last node and the loop is closed). Returns
  /// 1 on success or 0 if n or idx are out of range.
  virtual int GetIntermediatePointDisplayPosition(int n,
                                                  int idx, double pos[2]);

  /// Add an intermediate point between node n and n+1
  /// (or n and 0 if n is the last node and the loop is closed).
  /// Returns 1 on success or 0 if n is out of range.
  virtual int AddIntermediatePointWorldPosition(int n,
                                                double point[3]);

  /// Delete the last node. Returns 1 on success or 0 if
  /// there were not any nodes.
  virtual int DeleteLastNode();

  /// Delete the active node. Returns 1 on success or 0 if
  /// the active node did not indicate a valid node.
  virtual int DeleteActiveNode();

  /// Delete the nth node. Return 1 on success or 0 if n
  /// is out of range.
  virtual int DeleteNthNode(int n);

  /// Delete all nodes.
  virtual void ClearAllNodes();

  /// Given a specific X, Y pixel location, add a new node
  /// on the widget at this location.
  virtual int AddNodeOnWidget(int X, int Y);

  /// Specify tolerance for performing pick operations of points (trough a locator).
  /// Tolerance is specified as fraction of rendering window size.
  /// (Rendering window size is measured across diagonal in display pixel coordinates)
  /// Default value is 0.001
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

  /// The tolerance used to pick points in pixel.
  /// Calculated from the tolerance value.
  vtkGetMacro(PixelTolerance,int);

  /// Give an inter which provides
  vtkGetMacro(ActiveControl,int);

  /// Used to communicate about the state of the representation
  enum {
    Outside = 0,
    OnControlPoint,
    OnLine
  };

  /// Used to communicate about the operation of the representation
  enum {
    Inactive = 0,
    Select,
    Translate,
    Scale,
    Pick,
    Rotate
  };

  /// Set / get the current operation. The widget is either
  /// inactive, or it is being translated.
  vtkGetMacro(CurrentOperation, int);
  vtkSetClampMacro(CurrentOperation, int,
                   vtkSlicerAbstractRepresentation::Inactive,
                   vtkSlicerAbstractRepresentation::Rotate);
  void SetCurrentOperationToInactive()
    { this->SetCurrentOperation(vtkSlicerAbstractRepresentation::Inactive); }
  void SetCurrentOperationToSelect()
    { this->SetCurrentOperation(vtkSlicerAbstractRepresentation::Select); }
  void SetCurrentOperationToTranslate()
    { this->SetCurrentOperation(vtkSlicerAbstractRepresentation::Translate); }
  void SetCurrentOperationToScale()
    { this->SetCurrentOperation(vtkSlicerAbstractRepresentation::Scale); }
  void SetCurrentOperationToPick()
    { this->SetCurrentOperation(vtkSlicerAbstractRepresentation::Pick); }
  void SetCurrentOperationToRotate()
    { this->SetCurrentOperation(vtkSlicerAbstractRepresentation::Rotate); }

  /// Set / get the Point Placer. The point placer is
  /// responsible for converting display coordinates into
  /// world coordinates according to some constraints, and
  /// for validating world positions.
  void SetPointPlacer(vtkPointPlacer *);
  vtkGetObjectMacro(PointPlacer, vtkPointPlacer);

  /// Set / Get the Line Interpolator. The line interpolator
  /// is responsible for generating the line segments connecting
  /// nodes.
  void SetLineInterpolator(vtkSlicerLineInterpolator *);
  vtkGetObjectMacro(LineInterpolator, vtkSlicerLineInterpolator);

  /// Subclasses of vtkSlicerAbstractRepresentation must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void StartWidgetInteraction(double eventPos[2]) VTK_OVERRIDE;
  void SetInteractionState(int state);

  /// Controls whether the widget should always appear on top
  /// of other actors in the scene. (In effect, this will disable OpenGL
  /// Depth buffer tests while rendering the widget).
  /// Default is to set it to false.
  vtkSetMacro(AlwaysOnTop, vtkTypeBool);
  vtkGetMacro(AlwaysOnTop, vtkTypeBool);
  vtkBooleanMacro(AlwaysOnTop, vtkTypeBool);

  /// Get the points in this widget as a vtkPolyData.
  virtual vtkPolyData* GetWidgetRepresentationAsPolyData() = 0;

  /// Get the nodes and not the intermediate points in this
  /// widget as a vtkPolyData.
  void GetNodePolyData(vtkPolyData* poly);

  /// Handle when rebuilding the locator
  vtkSetMacro(RebuildLocator,bool);

  enum { RestrictNone = 0, RestrictToX, RestrictToY, RestrictToZ };

  /// Set if translations should be restricted to one of the axes (disabled if
  /// RestrictNone is specified).
  vtkSetClampMacro(RestrictFlag, int, RestrictNone, RestrictToZ);
  vtkGetMacro(RestrictFlag, int);

  /// Initialize with poly data
  ///
  virtual void Initialize(vtkPolyData *);

  /// Set / Get the ClosedLoop value. This ivar indicates whether the widget
  /// forms a closed loop.
  void SetClosedLoop(vtkTypeBool val);
  vtkGetMacro(ClosedLoop, vtkTypeBool);
  vtkBooleanMacro(ClosedLoop, vtkTypeBool);

  /// Set/Get the vtkMRMLMarkipsNode connected with this representation
  virtual void SetMarkupsNode(vtkMRMLMarkupsNode *markupNode);
  virtual vtkMRMLMarkupsNode* GetMarkupsNode();

  /// Set the renderer
  virtual void SetRenderer(vtkRenderer *ren) VTK_OVERRIDE;

protected:
  vtkSlicerAbstractRepresentation();
  ~vtkSlicerAbstractRepresentation() VTK_OVERRIDE;

  vtkWeakPointer<vtkMRMLMarkupsNode> MarkupsNode;

  // Selection tolerance for the picking of points
  double PixelTolerance;
  double Tolerance;

  int ActiveControl;

  vtkPointPlacer             *PointPlacer;
  vtkSlicerLineInterpolator  *LineInterpolator;

  int CurrentOperation;
  vtkTypeBool ClosedLoop;

  virtual void AddNodeAtPositionInternal(double worldPos[3]);
  virtual void SetNthNodeWorldPositionInternal(int n, double worldPos[3]);
  virtual void FromWorldOrientToOrientationQuaternion(double worldOrient[9], double orientation[4]);
  virtual void FromOrientationQuaternionToWorldOrient(double orientation[4], double worldOrient[9]);

  // Given a world position and orientation, this computes the display position
  // using the renderer of this class.
  void GetRendererComputedDisplayPositionFromWorldPosition(double worldPos[3],
                                                           double displayPos[2]);

  virtual void UpdateLines(int index);
  void UpdateLine(int idx1, int idx2);

  virtual int FindClosestPointOnWidget(int X, int Y,
                                       double worldPos[3],
                                       int *idx);

  virtual void BuildLines()=0;

  // This method is called when something changes in the point placer.
  // It will cause all points to be updated, and all lines to be regenerated.
  // It should be extended to detect changes in the line interpolator too.
  virtual int  UpdateWidget(bool force = false);
  vtkTimeStamp WidgetBuildTime;

  void ComputeMidpoint(double p1[3], double p2[3], double mid[3])
  {
      mid[0] = (p1[0] + p2[0])/2;
      mid[1] = (p1[1] + p2[1])/2;
      mid[2] = (p1[2] + p2[2])/2;
  }

  // Adding a point locator to the representation to speed
  // up lookup of the active node when dealing with large datasets (100k+)
  vtkIncrementalOctreePointLocator *Locator;

  // Deletes the previous locator if it exists and creates
  // a new locator. Also deletes / recreates the attached data set.
  void ResetLocator();

  // Calculate view scale factor
  double CalculateViewScaleFactor();

  virtual void BuildLocator();

  bool RebuildLocator;

  // Axis restrict flag
  int RestrictFlag;

  // Render the cursor
  vtkPolyData          *CursorShape;
  vtkPolyData          *SelectedCursorShape;
  vtkPolyData          *ActiveCursorShape;

  vtkPolyData          *FocalData;
  vtkPoints            *FocalPoint;
  vtkPolyData          *SelectedFocalData;
  vtkPoints            *SelectedFocalPoint;
  vtkPolyData          *ActiveFocalData;
  vtkPoints            *ActiveFocalPoint;

  // Support picking
  double LastEventPosition[2];

  // Compute the centroid by sampling the points along the polyline of the widget at equal distances
  virtual void ComputeCentroid(double* ioCentroid);

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.

  virtual void  CreateDefaultProperties() = 0;

  vtkTypeBool AlwaysOnTop;

private:
  vtkSlicerAbstractRepresentation(const vtkSlicerAbstractRepresentation&) = delete;
  void operator=(const vtkSlicerAbstractRepresentation&) = delete;
};

#endif
