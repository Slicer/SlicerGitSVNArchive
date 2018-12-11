/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerAbstractWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSlicerAbstractWidget
 * @brief   create a contour with a set of points
 *
 * The vtkSlicerAbstractWidget is used to select a set of points, and draw lines
 * between these points. The contour may be opened or closed, depending on
 * how the last point is added. The widget handles all processing of widget
 * events (that are triggered by VTK events). The vtkSlicerAbstractRepresentation is
 * responsible for all placement of the points, calculation of the lines, and
 * contour manipulation. This is done through two main helper classes:
 * vtkPointPlacer and vtkContourLineInterpolator. The representation is also
 * responsible for drawing the points and lines.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - triggers a Select event
 *   Alt + LeftButtonPressEvent - triggers a Rotate event
 *   MiddleButtonPressEvent - triggers a Shift event
 *   RightButtonPressEvent - triggers a AddFinalPoint event
 *
 *   MouseMoveEvent - triggers a Move event
 *
 *   LeftButtonReleaseEvent - triggers an EndAction event
 *   MiddleButtonReleaseEvent - triggers an EndAction event
 *   RightButtonReleaseEvent - triggers an EndAction event
 *
 *   LeftButtonDoubleClickEvent - triggers an PickOne event
 *   MiddleButtonDoubleClickEvent - triggers an PickTwo event
 *   RightButtonDoubleClickEvent - triggers an AddPoint event
 *
 *
 *   Delete key event - triggers a Delete event
 *   Shift + Delete key event - triggers a Reset event
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkSlicerAbstractWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select
 *        widget state is:
 *            Start or
 *            Define: If we already have at least 2 nodes, test
 *                 whether the current (X,Y) location is near an existing
 *                 node. If so, close the contour and change to Manipulate
 *                 state. Otherwise, attempt to add a node at this (X,Y)
 *                 location.
 *            Manipulate: If this (X,Y) location activates a node or the line, then
 *                 set the current operation to Translate.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::PickPoint
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node or the line. The node or line
 *                 will be selected, but no translate will be possible.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Rotate
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node or the line, then
 *                 set the current operation to Rotate. if any node is locked, the rotation is aborted.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Shift
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node or the line, then
 *                 set the current operation to Shift.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Scale
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node or the line, then
 *                 set the current operation to Scale.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::AddPoint
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: if the current (X,Y) location is near the line, attempt to add a
 *                 new node on the contour at this location.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::AddFinalPoint
 *        widget state is:
 *            Start: Do nothing.
 *            Define: If we already have at least 2 nodes, test
 *                 whether the current (X,Y) location is near an existing
 *                 node. If so, close the contour and change to Manipulate
 *                 state. Otherwise, attempt to add a node at this (X,Y)
 *                 location. If we do, then leave the contour open and
 *                 change to Manipulate state.
 *            Manipulate: Do nothing.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Move
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is Translate, Shift or Scale, then invoke
 *                  WidgetInteraction() on the representation. If our
 *                  operation is Inactive, then just attempt to activate
 *                  a node at this (X,Y) location or the line.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::EndAction
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is not Inactive, set it to
 *                  Inactive.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Delete
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Remove the last point on the contour.
 *            Manipulate: Attempt to activate a node at (X,Y). If
 *                   we do activate a node, delete it. If we now
 *                   have less than 3 nodes, go back to Define state.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Reset
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Remove all points and line segments of the contour.
 *                 Essentially calls Initialize(nullptr)
 *            Manipulate: Do nothing.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following VTK events on itself (which observers
 * can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (beginning to interact;
 *                                      call data includes handle id)
 *   vtkCommand::EndInteractionEvent (completing interaction;
 *                                    call data includes handle id)
 *   vtkCommand::InteractionEvent (moving after selecting something;
 *                                 call data includes handle id)
 *   vtkCommand::PlacePointEvent (after point is positioned;
 *                                call data includes handle id)
 *   vtkCommand::DeletePointEvent (after point is positioned;
 *                                 call data includes handle id)
 *
 *   Note: handle id conuter start from 0. -1 indicates the line.
 * </pre>
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

  ///
  virtual void SetRepresentation(vtkSlicerAbstractRepresentation *r);

  ///
  virtual vtkSlicerAbstractRepresentation *GetRepresentation();

  ///
  virtual void BuildRepresentation();

  ///
  virtual void BuildLocator();

  /// Create the default widget representation if one is not set.
  virtual void CreateDefaultRepresentation() = 0;

  /// Convenient method to close the contour loop.
  virtual void CloseLoop();

  /// Convenient method to change what state the widget is in.
  vtkSetMacro(WidgetState,int);

  /// Convenient method to determine the state of the method
  vtkGetMacro(WidgetState,int);

  /// Follow the cursor ? If this is ON, during definition, the last node of the
  /// contour will automatically follow the cursor, without waiting for the
  /// point to be dropped. This may be useful for some interpolators, such as the
  /// live-wire interpolator to see the shape of the contour that will be placed
  /// as you move the mouse cursor.
  vtkSetMacro( FollowCursor, vtkTypeBool );
  vtkGetMacro( FollowCursor, vtkTypeBool );
  vtkBooleanMacro( FollowCursor, vtkTypeBool );

  /// Convenient method to remap the horizonbtal axes constrain key.
  vtkSetMacro( HorizontalActiveKeyCode, const char );
  vtkGetMacro( HorizontalActiveKeyCode, const char );

  /// Convenient method to remap the vertical axes constrain key.
  vtkSetMacro( VerticalActiveKeyCode, const char );
  vtkGetMacro( VerticalActiveKeyCode, const char );

  /// Convenient method to remap the depth axes constrain key.
  vtkSetMacro( DepthActiveKeyCode, const char );
  vtkGetMacro( DepthActiveKeyCode, const char );

  /// Initialize the contour widget from a user supplied set of points. The
  /// state of the widget decides if you are still defining the widget, or
  /// if you've finished defining (added the last point) are manipulating
  /// it. Note that if the polydata supplied is closed, the state will be
  /// set to manipulate.
  /// State: Define = 0, Manipulate = 1.
  virtual void Initialize( vtkPolyData * poly, int state = 1);
  virtual void Initialize()
    {this->Initialize(nullptr);}

  /// The state of the widget
  enum {Start,Define,Manipulate};

protected:
  vtkSlicerAbstractWidget();
  ~vtkSlicerAbstractWidget() VTK_OVERRIDE;

  vtkSmartPointer<vtkSlicerAbstractRepresentation> WidgetRepresentation;

  int WidgetState;
  int CurrentHandle;
  vtkTypeBool FollowCursor;

  // helper methods for cursor management
  void SetCursor(int state) override;

  // Callback interface to capture events when
  // placing the widget.
  static void PickAction(vtkAbstractWidget*);
  static void ShiftAction(vtkAbstractWidget*);
  static void RotateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void EndAction(vtkAbstractWidget*);
  static void ResetAction(vtkAbstractWidget*);

  // Internal helper methods
  virtual void AddPointToRepresentation() = 0;
  virtual void AddPointToRepresentationFromWorldCoordinate(double worldCoordinates [3]) = 0;

  // Manual axis constrain
  char HorizontalActiveKeyCode;
  char VerticalActiveKeyCode;
  char DepthActiveKeyCode;
  int KeyCount;
  vtkCallbackCommand *KeyEventCallbackCommand;
  static void ProcessKeyEvents(vtkObject *, unsigned long, void *, void *);

private:
  vtkSlicerAbstractWidget(const vtkSlicerAbstractWidget&) = delete;
  void operator=(const vtkSlicerAbstractWidget&) = delete;
};

#endif
