/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerPointsWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSlicerPointsWidget
 * @brief   create a set of points
 *
 * The vtkSlicerPointsWidget is used to select a set of points.
 * The widget handles all processing of widget
 * events (that are triggered by VTK events). The vtkSlicerAbstractRepresentation is
 * responsible for all placement of the points, and
 * points manipulation. This is done through a main helper class:
 * vtkPointPlacer. The representation is also
 * responsible for drawing the points.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - triggers a Select event
 *   Alt + LeftButtonPressEvent - triggers a Rotate event
 *   MiddleButtonPressEvent - triggers a Shift event
 *   RightButtonPressEvent - triggers a Scale event
 *
 *   MouseMoveEvent - triggers a Move event
 *
 *   LeftButtonReleaseEvent - triggers an EndSelect event
 *   MiddleButtonReleaseEvent - triggers an EndShift event
 *   RightButtonReleaseEvent - triggers an EndScale event
 *
 *   LeftButtonDoubleClickEvent - triggers an PickOne event
 *   MiddleButtonDoubleClickEvent - triggers an PickTwo event
 *   RightButtonDoubleClickEvent - triggers an PickThree event
 *
 *
 *   Delete key event - triggers a Delete event
 *   Shift + Delete key event - triggers a Reset event
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkSlicerPointsWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select
 *        widget state is:
 *            Start or
 *            Define: Add a node at this (X,Y) location.
 *            Manipulate: If this (X,Y) location activates a node, then
 *                 set the current operation to Translate.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::PickPoint
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node. The node or line
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
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Move
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is Translate, Shift or Scale, then invoke
 *                  WidgetInteraction() on the representation. If our
 *                  operation is Inactive, then just attempt to activate
 *                  a node at this (X,Y) location.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::EndSelect
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is not Inactive, set it to
 *                  Inactive.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::EndTranslate
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is not Inactive, set it to
 *                  Inactive.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::EndScale
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
 *            Define: Remove the last point.
 *            Manipulate: Attempt to activate a node at (X,Y). If
 *                   we do activate a node, delete it. If we now
 *                   have less than 3 nodes, go back to Define state.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Reset
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Remove all points.
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
 *   Note: handle id conuter start from 0.
 * </pre>
 *
*/

#ifndef vtkSlicerPointsWidget_h
#define vtkSlicerPointsWidget_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractWidget.h"

class vtkSlicerAbstractRepresentation;
class vtkPolyData;
class vtkIdList;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerPointsWidget : public vtkSlicerAbstractWidget
{
public:
  /// Instantiate this class.
  static vtkSlicerPointsWidget *New();

  /// Standard methods for a VTK class.
  vtkTypeMacro(vtkSlicerPointsWidget,vtkSlicerAbstractWidget);

  /// Create the default widget representation if one is not set.
  /// NOTE: the representation needs also a Markup object from the MRMLMarkupsNode
  void CreateDefaultRepresentation() VTK_OVERRIDE;

  /// Add a point to the current active Markup at the current X and Y display coordiantes of the interactor.
  void AddPointToRepresentation() VTK_OVERRIDE;

  /// Add a point to the current active Markup at input World coordiantes.
  void AddPointToRepresentationFromWorldCoordinate(double worldCoordinates [3]) VTK_OVERRIDE;

protected:
  vtkSlicerPointsWidget();
  ~vtkSlicerPointsWidget() VTK_OVERRIDE;

  // Callback interface to capture evts when
  // placing the widget.
  static void SelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void DeleteAction(vtkAbstractWidget*);

private:
  vtkSlicerPointsWidget(const vtkSlicerPointsWidget&) = delete;
  void operator=(const vtkSlicerPointsWidget&) = delete;
};

#endif
