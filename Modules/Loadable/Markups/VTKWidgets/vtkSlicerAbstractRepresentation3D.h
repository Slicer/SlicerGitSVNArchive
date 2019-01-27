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

  /// Given a display position, activate a node. The closest
  /// node within tolerance will be activated. If a node is
  /// activated, 1 will be returned, otherwise 0 will be
  /// returned.
  virtual int ActivateNode(int X, int Y) VTK_OVERRIDE;

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

  // Methods to manipulate the cursor
  virtual void TranslateNode(double eventPos[2]);
  virtual void TranslateWidget(double eventPos[2]);
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

  vtkActor2D                  *SelectedLabelsActor;
  vtkLabelPlacementMapper     *SelectedLabelsMapper;

  vtkActor2D                  *ActiveLabelsActor;
  vtkLabelPlacementMapper     *ActiveLabelsMapper;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty   *Property;
  vtkProperty   *SelectedProperty;
  vtkProperty   *ActiveProperty;

  virtual void CreateDefaultProperties() VTK_OVERRIDE;
  virtual void BuildRepresentationPointsAndLabels();

private:
  vtkSlicerAbstractRepresentation3D(const vtkSlicerAbstractRepresentation3D&) = delete;
  void operator=(const vtkSlicerAbstractRepresentation3D&) = delete;
};

#endif
