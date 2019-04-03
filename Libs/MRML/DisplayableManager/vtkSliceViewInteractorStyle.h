/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSliceViewInteractorStyle.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSliceViewInteractorStyle_h
#define __vtkSliceViewInteractorStyle_h

// VTK includes
#include "vtkInteractorStyleUser.h"
#include "vtkWeakPointer.h"

// MRML includes
#include "vtkMRMLDisplayableManagerExport.h"

class vtkMRMLAbstractDisplayableManager;
class vtkMRMLCrosshairDisplayableManager;
class vtkMRMLSegmentationDisplayNode;
class vtkMRMLSliceLogic;
class vtkMRMLDisplayableManagerGroup;

/// \brief Provides customizable interaction routines.
///
/// Relies on vtkInteractorStyleUser, but with MouseWheelEvents.
/// and mapping to control the slicer slice logic (manipulates the
/// vtkMRMLSliceNode and vtkMRMLSliceCompositeNode.
/// TODO:
/// * Do we need Rotate Mode?  Probably better to just rely on the reformat widget
/// * Do we need to set the slice spacing on EnterEvent (I say no, nothing to do
///   with linked slices should go in here)
class VTK_MRML_DISPLAYABLEMANAGER_EXPORT vtkSliceViewInteractorStyle :
  public vtkInteractorStyleUser
{
public:
  static vtkSliceViewInteractorStyle *New();
  vtkTypeMacro(vtkSliceViewInteractorStyle,vtkInteractorStyleUser);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///
  /// Events are either handled here by changing the slice node
  /// and composite node (sometimes using the logic's methods) or
  /// they are passed to the vtkInteractorStyleUser, which conditionally
  /// passes them to observers if there are any.
  void OnMouseMove() override;
  void OnEnter() override;
  void OnLeave() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnMouseWheelForward() override;
  void OnMouseWheelBackward() override;
  void OnPinch() override;
  void OnRotate() override;
  void OnPan() override;
  void OnTap() override;
  void OnLongTap() override;

  /// Keyboard functions
  void OnChar() override;
  void OnKeyPress() override;
  void OnKeyRelease() override;

  /// These are more esoteric events, but are useful in some cases.
  void OnExpose() override;
  void OnConfigure() override;

  void SetDisplayableManagers(vtkMRMLDisplayableManagerGroup* displayableManagers);

  /// Give a chance to displayable managers to process the event.
  /// Return true if the event is processed.
  bool ForwardInteractionEventToDisplayableManagers(unsigned long event);

  /// Internal state management for multi-event sequences (like click-drag-release)

  /// Action State values and management
  enum
    {
    None = 0,
    Translate = 1,
    Zoom = 2,
    Rotate = 4, /* Rotate not currently used */
    Blend = 8, /* fg to bg, labelmap to bg */
    AdjustWindowLevelBackground = 16,
    AdjustWindowLevelForeground = 32,
    BrowseSlice = 64,
    ShowSlice = 128,
    AdjustLightbox = 256, /* not used */
    SelectVolume = 512,
    SetCursorPosition = 1024, /* adjust cursor position in crosshair node as mouse is moved */
    SetCrosshairPosition = 2048,
    TranslateSliceIntersection = 4096,
    RotateSliceIntersection = 8192,
    AllActionsMask = Translate | Zoom | Rotate | Blend | AdjustWindowLevelBackground | AdjustWindowLevelForeground
      | BrowseSlice | ShowSlice | AdjustLightbox | SelectVolume | SetCursorPosition | SetCrosshairPosition
      | TranslateSliceIntersection | RotateSliceIntersection
    };

  /// Enable/disable the specified action (Translate, Zoom, Blend, etc.).
  /// Multiple actions can be specified by providing the sum of action ids.
  /// Set the value to AllActionsMask to enable/disable all actions.
  /// All actions are enabled by default.
  void SetActionEnabled(int actionsMask, bool enable = true);
  /// Returns true if the specified action is allowed.
  /// If multiple actions are specified, the return value is true if all actions are enabled.
  bool GetActionEnabled(int actionsMask);

  ///
  /// Get/Set the SliceLogic
  void SetSliceLogic(vtkMRMLSliceLogic* SliceLogic);
  vtkGetObjectMacro(SliceLogic, vtkMRMLSliceLogic);

  vtkMRMLCrosshairDisplayableManager* GetCrosshairDisplayableManager();

protected:
  vtkSliceViewInteractorStyle();
  ~vtkSliceViewInteractorStyle() override;

  vtkMRMLSliceLogic *SliceLogic;

  bool MouseMovedSinceButtonDown;

  bool EnableCursorUpdate;

  vtkWeakPointer<vtkMRMLDisplayableManagerGroup> DisplayableManagers;
  vtkMRMLAbstractDisplayableManager* FocusedDisplayableManager;

private:
  vtkSliceViewInteractorStyle(const vtkSliceViewInteractorStyle&) = delete;
  void operator=(const vtkSliceViewInteractorStyle&) = delete;
};

#endif
