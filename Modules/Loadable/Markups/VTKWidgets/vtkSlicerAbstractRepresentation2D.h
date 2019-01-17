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
 * @class   vtkSlicerAbstractRepresentation2D
 * @brief   Default representation for the slicer markups widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerAbstractWidget. It works in conjunction with the
 * vtkSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerAbstractWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation2D vtkSlicerAbstractWidget vtkPointPlacer
 * vtkSlicerLineInterpolator
*/

#ifndef vtkSlicerAbstractRepresentation2D_h
#define vtkSlicerAbstractRepresentation2D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation.h"

#include "vtkMRMLSliceNode.h"

class vtkProperty2D;
class vtkActor2D;
class vtkOpenGLPolyDataMapper2D;
class vtkGlyph2D;
class vtkPropPicker;
class vtkTextProperty;
class vtkLabelPlacementMapper;
class vtkPointSetToLabelHierarchy;
class vtkStringArray;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractRepresentation2D : public vtkSlicerAbstractRepresentation
{
public:
  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerAbstractRepresentation2D,vtkSlicerAbstractRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// This is the property used when the handle is not active
  /// (the mouse is not near the handle)
  vtkGetObjectMacro(Property,vtkProperty2D);

  /// This is the selected property used when the handle is not active
  /// (the mouse is not near the handle)
  vtkGetObjectMacro(SelectedProperty,vtkProperty2D);

  /// This is the property used when the user is interacting
  /// with the handle.
  vtkGetObjectMacro(ActiveProperty,vtkProperty2D);

  /// This is the property used for the text when the handle is not active
  /// (the mouse is not near the handle)
  vtkGetObjectMacro(TextProperty,vtkTextProperty);

  /// This is the selected property used for the text when the handle is not active
  /// (the mouse is not near the handle)
  vtkGetObjectMacro(SelectedTextProperty,vtkTextProperty);

  /// This is the property used for the text when the user is interacting
  /// with the handle.
  vtkGetObjectMacro(ActiveTextProperty,vtkTextProperty);

  /// Register internal Pickers within PickingManager
  void RegisterPickers() VTK_OVERRIDE;

  /// Subclasses of vtkSlicerAbstractRepresentation2D must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void BuildRepresentation() VTK_OVERRIDE;
  int ComputeInteractionState(int X, int Y, int modified=0) VTK_OVERRIDE;
  void WidgetInteraction(double eventPos[2]) VTK_OVERRIDE;

  /// Methods to make this class behave as a vtkProp.
  void GetActors(vtkPropCollection *) VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  vtkTypeBool HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /// Set/Get the three leaders used to create this representation.
  /// By obtaining these leaders the user can set the appropriate
  /// properties, etc.
  vtkGetObjectMacro(Actor,vtkActor2D);
  vtkGetObjectMacro(SelectedActor,vtkActor2D);
  vtkGetObjectMacro(ActiveActor,vtkActor2D);
  vtkGetObjectMacro(LabelsActor,vtkActor2D);
  vtkGetObjectMacro(SelectedLabelsActor,vtkActor2D);
  vtkGetObjectMacro(ActiveLabelsActor,vtkActor2D);

  /// Specify the cursor shape. Keep in mind that the shape will be
  /// aligned with the constraining plane by orienting it such that
  /// the x axis of the geometry lies along the normal of the plane.
  void SetCursorShape(vtkPolyData *cursorShape);
  vtkPolyData *GetCursorShape();

  /// Specify the cursor shape. Keep in mind that the shape will be
  /// aligned with the constraining plane by orienting it such that
  /// the x axis of the geometry lies along the normal of the plane.
  void SetSelectedCursorShape(vtkPolyData *selectedShape);
  vtkPolyData *GetSelectedCursorShape();

  /// Specify the shape of the cursor (handle) when it is active.
  /// This is the geometry that will be used when the mouse is
  /// close to the handle or if the user is manipulating the handle.
  void SetActiveCursorShape(vtkPolyData *activeShape);
  vtkPolyData *GetActiveCursorShape();

  /// Set/Get method to the sliceNode connected to this representation
  void SetSliceNode(vtkMRMLSliceNode *sliceNode);
  vtkMRMLSliceNode *GetSliceNode();

  /// Get the nth node's position on the slice. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  int GetNthNodeSliceDisplayPosition( int n, double pos[2] );

  /// Set the nth node's position on the slice. slice position
  /// will be converted into world position according to the
  /// GetXYToRAS matrix. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting) or the world position
  /// is not valid.
  int SetNthNodeSliceDisplayPosition( int n, double pos[2] );

  /// Add a node at a specific position on the slice. This will be
  /// converted into a world position according to the current
  /// constraints of the point placer. Return 0 if a point could
  /// not be added, 1 otherwise.
  int AddNodeAtSliceDisplayPosition( double slicePos[2] );

  /// Delete the nth node. Return 1 on success or 0 if n
  /// is out of range.
  int DeleteNthNode( int n ) VTK_OVERRIDE;

  /// Delete the nth node. Return 1 on success or 0 if n
  /// is out of range.
  virtual void SetNthPointSliceVisibility( int n, bool visibility );

protected:
  vtkSlicerAbstractRepresentation2D();
  ~vtkSlicerAbstractRepresentation2D() VTK_OVERRIDE;

  void GetSliceToWorldCoordinates( double slicePos[2], double worldPos[3] );

  vtkWeakPointer<vtkMRMLSliceNode> SliceNode;

  // Support picking
  vtkPropPicker *CursorPicker;

  // Methods to manipulate the cursor
  virtual void TranslateNode(double eventPos[2]);
  virtual void ShiftWidget(double eventPos[2]);
  virtual void ScaleWidget(double eventPos[2]);
  virtual void RotateWidget(double eventPos[2]);

  // Render the cursor
  vtkActor2D                 *Actor;
  vtkOpenGLPolyDataMapper2D  *Mapper;
  vtkActor2D                 *SelectedActor;
  vtkOpenGLPolyDataMapper2D  *SelectedMapper;
  vtkActor2D                 *ActiveActor;
  vtkOpenGLPolyDataMapper2D  *ActiveMapper;
  vtkGlyph2D                 *Glypher;
  vtkGlyph2D                 *SelectedGlypher;
  vtkGlyph2D                 *ActiveGlypher;

  vtkActor2D                  *LabelsActor;
  vtkLabelPlacementMapper     *LabelsMapper;
  vtkPointSetToLabelHierarchy *PointSetToLabelHierarchyFilter;
  vtkStringArray              *Labels;
  vtkStringArray              *LabelsPriority;

  vtkActor2D                  *SelectedLabelsActor;
  vtkLabelPlacementMapper     *SelectedLabelsMapper;
  vtkPointSetToLabelHierarchy *SelectedPointSetToLabelHierarchyFilter;
  vtkStringArray              *SelectedLabels;
  vtkStringArray              *SelectedLabelsPriority;

  vtkActor2D                  *ActiveLabelsActor;
  vtkLabelPlacementMapper     *ActiveLabelsMapper;
  vtkPointSetToLabelHierarchy *ActivePointSetToLabelHierarchyFilter;
  vtkStringArray              *ActiveLabels;
  vtkStringArray              *ActiveLabelsPriority;

  vtkPolyData                 *LabelsFocalData;
  vtkPoints                   *LabelsFocalPoint;
  vtkPolyData                 *SelectedLabelsFocalData;
  vtkPoints                   *SelectedLabelsFocalPoint;
  vtkPolyData                 *ActiveLabelsFocalData;
  vtkPoints                   *ActiveLabelsFocalPoint;

  vtkIntArray *pointsVisibilityOnSlice;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty2D   *Property;
  vtkProperty2D   *SelectedProperty;
  vtkProperty2D   *ActiveProperty;

  vtkTextProperty   *TextProperty;
  vtkTextProperty   *SelectedTextProperty;
  vtkTextProperty   *ActiveTextProperty;
  virtual void  CreateDefaultProperties() VTK_OVERRIDE;

  void BuildLocator() VTK_OVERRIDE;

  virtual void AddNodeAtPositionInternal( double worldPos[3]) VTK_OVERRIDE;

private:
  vtkSlicerAbstractRepresentation2D(const vtkSlicerAbstractRepresentation2D&) = delete;
  void operator=(const vtkSlicerAbstractRepresentation2D&) = delete;
};

#endif
