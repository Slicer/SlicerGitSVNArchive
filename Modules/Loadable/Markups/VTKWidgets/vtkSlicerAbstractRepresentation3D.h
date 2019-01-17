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
 * @class   vtkSlicerAbstractRepresentation3D
 * @brief   Default representation for the slicer markups widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerAbstractWidget. It works in conjunction with the
 * vtkSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerAbstractWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation3D vtkSlicerAbstractWidget vtkPointPlacer
 * vtkSlicerLineInterpolator
*/

#ifndef vtkSlicerAbstractRepresentation3D_h
#define vtkSlicerAbstractRepresentation3D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation.h"

#include "vtkMRMLMarkupsNode.h"

#include <vector> // STL Header; Required for vector

class vtkProperty;
class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkGlyph3D;
class vtkCellPicker;
class vtkLabelPlacementMapper;
class vtkPointSetToLabelHierarchy;
class vtkStringArray;
class vtkActor2D;
class vtkTextProperty;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractRepresentation3D : public vtkSlicerAbstractRepresentation
{
public:
  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerAbstractRepresentation3D,vtkSlicerAbstractRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// This is the property used when the handle is not active
  /// (the mouse is not near the handle)
  vtkGetObjectMacro(Property,vtkProperty);

  /// This is the selected property used when the handle is not active
  /// (the mouse is not near the handle)
  vtkGetObjectMacro(SelectedProperty,vtkProperty);

  /// This is the property used when the user is interacting
  /// with the handle.
  vtkGetObjectMacro(ActiveProperty,vtkProperty);

  /// This is the property used for the text when the handle is not active
  /// (the mouse is not near the handle)
  vtkGetObjectMacro(TextProperty,vtkTextProperty);

  /// This is the selected property used for the text when the handle is not active
  /// (the mouse is not near the handle)
  vtkGetObjectMacro(SelectedTextProperty,vtkTextProperty);

  /// This is the property used for the text when the user is interacting
  /// with the handle.
  vtkGetObjectMacro(ActiveTextProperty,vtkTextProperty);

  /// Specify tolerance for performing pick operation. Tolerance is specified
  /// as fraction of rendering window size. (Rendering window size is measured
  /// across diagonal in display pixel coordinates)
  void SetTolerance(double tol) VTK_OVERRIDE;

  /// Register internal Pickers within PickingManager
  void RegisterPickers() VTK_OVERRIDE;

  /// Subclasses of vtkSlicerAbstractRepresentation3D must implement these methods. These
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
  vtkGetObjectMacro(Actor,vtkOpenGLActor);
  vtkGetObjectMacro(SelectedActor,vtkOpenGLActor);
  vtkGetObjectMacro(ActiveActor,vtkOpenGLActor);
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

protected:
  vtkSlicerAbstractRepresentation3D();
  ~vtkSlicerAbstractRepresentation3D() VTK_OVERRIDE;

  // Support picking
  vtkCellPicker *CursorPicker;

  // Methods to manipulate the cursor
  virtual void TranslateNode(double eventPos[2]);
  virtual void ShiftWidget(double eventPos[2]);
  virtual void ScaleWidget(double eventPos[2]);
  virtual void RotateWidget(double eventPos[2]);

  // Render the cursor
  vtkOpenGLActor              *Actor;
  vtkOpenGLPolyDataMapper     *Mapper;
  vtkOpenGLActor              *SelectedActor;
  vtkOpenGLPolyDataMapper     *SelectedMapper;
  vtkOpenGLActor              *ActiveActor;
  vtkOpenGLPolyDataMapper     *ActiveMapper;

  vtkGlyph3D                  *Glypher;
  vtkGlyph3D                  *SelectedGlypher;
  vtkGlyph3D                  *ActiveGlypher;

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

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty   *Property;
  vtkProperty   *SelectedProperty;
  vtkProperty   *ActiveProperty;

  vtkTextProperty   *TextProperty;
  vtkTextProperty   *SelectedTextProperty;
  vtkTextProperty   *ActiveTextProperty;
  virtual void  CreateDefaultProperties() VTK_OVERRIDE;

private:
  vtkSlicerAbstractRepresentation3D(const vtkSlicerAbstractRepresentation3D&) = delete;
  void operator=(const vtkSlicerAbstractRepresentation3D&) = delete;
};

#endif
