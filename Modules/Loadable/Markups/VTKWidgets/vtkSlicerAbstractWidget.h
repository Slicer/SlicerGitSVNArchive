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
 * @class   vtkSlicerAbstractWidget
 * @brief   create a contour with a set of points
 *
 * The vtkSlicerAbstractWidget is the abstract class for the Points, Line, Angle
 * and Curve widgets. The widget handles all processing of widget
 * events (that are triggered by VTK events). See child classes for more informations.
 *
*/

#ifndef vtkSlicerAbstractWidget_h
#define vtkSlicerAbstractWidget_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkAbstractWidget.h"

#include "vtkMRMLMarkupsNode.h"

class vtkSlicerAbstractRepresentation;
class vtkPolyData;
class vtkIdList;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractWidget : public vtkAbstractWidget
{
public:
  /// Standard methods for a VTK class.
  vtkTypeMacro(vtkSlicerAbstractWidget,vtkAbstractWidget);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// The method for activating and deactivating this widget. This method
  /// must be overridden because it is a composite widget and does more than
  /// its superclasses' vtkAbstractWidget::SetEnabled() method.
  virtual void SetEnabled(int) VTK_OVERRIDE;

  /// Set the representation
  virtual void SetRepresentation(vtkSlicerAbstractRepresentation *r);

  /// Get the representation
  virtual vtkSlicerAbstractRepresentation *GetRepresentation();

  /// Build the actors of the representation with the info stored in the vtkMRMLMarkupsNode
  virtual void BuildRepresentation();

  /// Build the locator with the info stored in the vtkMRMLMarkupsNode
  virtual void BuildLocator();

  /// Create the default widget representation if one is not set.
  virtual void CreateDefaultRepresentation() = 0;

  /// Convenient method to close the contour loop.
  virtual void CloseLoop();

  /// Convenient method to change what state the widget is in.
  vtkSetMacro(WidgetState,int);

  /// Convenient method to determine the state of the method
  vtkGetMacro(WidgetState,int);

  /// During definition, the last node of the
  /// contour will automatically follow the cursor, without waiting for the
  /// point to be dropped. This is useful for some interpolators, such as the
  /// live-wire interpolator to see the shape of the contour that will be placed
  /// as you move the mouse cursor.
  vtkSetMacro(FollowCursor, vtkTypeBool);
  vtkGetMacro(FollowCursor, vtkTypeBool);
  vtkBooleanMacro(FollowCursor, vtkTypeBool);

  /// Convenient method to remap the horizonbtal axes constrain key.
  vtkSetMacro(HorizontalActiveKeyCode, const char);
  vtkGetMacro(HorizontalActiveKeyCode, char);

  /// Convenient method to remap the vertical axes constrain key.
  vtkSetMacro(VerticalActiveKeyCode, const char);
  vtkGetMacro(VerticalActiveKeyCode, char);

  /// Convenient method to remap the depth axes constrain key.
  vtkSetMacro(DepthActiveKeyCode, const char);
  vtkGetMacro(DepthActiveKeyCode, char);

  /// Initialize the contour widget from a user supplied set of points. The
  /// state of the widget decides if you are still defining the widget, or
  /// if you've finished defining (added the last point) are manipulating
  /// it. Note that if the polydata supplied is closed, the state will be
  /// set to manipulate.
  /// State: Define = 0, Manipulate = 1.
  virtual void Initialize(vtkPolyData * poly, int state = 1);
  virtual void Initialize()
    {this->Initialize(nullptr);}

  /// The state of the widget
  enum {Start,Define,Manipulate};

  /// Add a point preview to the current active Markup at input World coordiantes.
  int AddPreviewPointToRepresentationFromWorldCoordinate(double worldCoordinates [3]);

  /// Remove the point preview to the current active Markup.
  void RemoveLastPreviewPointToRepresentation();

protected:
  vtkSlicerAbstractWidget();
  ~vtkSlicerAbstractWidget() VTK_OVERRIDE;

  int WidgetState;
  int CurrentHandle;
  vtkTypeBool FollowCursor;

  // helper methods for cursor management
  void SetCursor(int state) override;

  // Callback interface to capture events when
  // placing the widget.
  static void SelectAction(vtkAbstractWidget*);
  static void PickAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void RotateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void EndAction(vtkAbstractWidget*);
  static void ResetAction(vtkAbstractWidget*);
  static void DeleteAction(vtkAbstractWidget*);

  // Manual axis constrain
  char HorizontalActiveKeyCode;
  char VerticalActiveKeyCode;
  char DepthActiveKeyCode;
  vtkCallbackCommand *KeyEventCallbackCommand;
  static void ProcessKeyEvents(vtkObject *, unsigned long, void *, void *);

private:
  vtkSlicerAbstractWidget(const vtkSlicerAbstractWidget&) = delete;
  void operator=(const vtkSlicerAbstractWidget&) = delete;
};

#endif
