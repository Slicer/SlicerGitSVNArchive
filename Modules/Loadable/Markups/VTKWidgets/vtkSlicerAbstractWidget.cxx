/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerAbstractWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlicerAbstractWidget.h"
#include "vtkSlicerAbstractRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkSphereSource.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"

//----------------------------------------------------------------------
vtkSlicerAbstractWidget::vtkSlicerAbstractWidget()
{
  this->ManagesCursor    = 1;
  this->WidgetState      = vtkSlicerAbstractWidget::Start;
  this->CurrentHandle    = 0;
  this->FollowCursor     = 1;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::AltModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Rotate,
                                          this, vtkSlicerAbstractWidget::RotateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Translate,
                                          this, vtkSlicerAbstractWidget::ShiftAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Scale,
                                          this, vtkSlicerAbstractWidget::ScaleAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkSlicerAbstractWidget::EndAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
                                          vtkWidgetEvent::EndTranslate,
                                          this, vtkSlicerAbstractWidget::EndAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
                                          vtkWidgetEvent::EndScale,
                                          this, vtkSlicerAbstractWidget::EndAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonDoubleClickEvent,
                                          vtkWidgetEvent::PickOne,
                                          this, vtkSlicerAbstractWidget::PickAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonDoubleClickEvent,
                                          vtkWidgetEvent::PickTwo,
                                          this, vtkSlicerAbstractWidget::PickAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonDoubleClickEvent,
                                          vtkWidgetEvent::PickThree,
                                          this, vtkSlicerAbstractWidget::PickAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::ShiftModifier, 127, 1, "Delete",
                                          vtkWidgetEvent::Reset,
                                          this, vtkSlicerAbstractWidget::ResetAction);

  this->HorizontalActiveKeyCode = 'x';
  this->VerticalActiveKeyCode = 'y';
  this->DepthActiveKeyCode = 'z';
  this->KeyCount = 0;
  this->KeyEventCallbackCommand = vtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkSlicerAbstractWidget::ProcessKeyEvents);
  this->Enabled = 1;
}

//----------------------------------------------------------------------
vtkSlicerAbstractWidget::~vtkSlicerAbstractWidget()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetCursor(int state)
{
  if ( !this->ManagesCursor )
    {
    return;
    }

  switch (state)
    {
    case 0:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
      break;
    case 1:
      this->RequestCursorShape(VTK_CURSOR_HAND);
      break;
    default:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::CloseLoop()
{
  if ( !this->WidgetRepresentation )
    {
    return;
    }

  if ( !this->WidgetRepresentation->GetClosedLoop() && this->WidgetRepresentation->GetNumberOfNodes() > 1 )
    {
    this->WidgetState = vtkSlicerAbstractWidget::Manipulate;
    this->WidgetRepresentation->ClosedLoopOn();
    this->Render();
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetEnabled( int enabling )
{
  if ( ! this->Interactor || !this->WidgetRepresentation )
    {
    return;
    }

  if ( enabling )
    {
    int X=this->Interactor->GetEventPosition()[0];
    int Y=this->Interactor->GetEventPosition()[1];

    if ( ! this->CurrentRenderer )
      {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X,Y));

      if (this->CurrentRenderer == nullptr)
        {
        return;
        }
      }

    // We're ready to enable
    this->Enabled = 1;

    // listen for the events found in the EventTranslator
    if ( ! this->Parent )
      {
      this->EventTranslator->AddEventsToInteractor(this->Interactor,
        this->EventCallbackCommand,this->Priority);
      }
    else
      {
      this->EventTranslator->AddEventsToParent(this->Parent,
        this->EventCallbackCommand,this->Priority);
      }

    this->WidgetRepresentation->SetRenderer(this->CurrentRenderer);
    this->WidgetRepresentation->RegisterPickers();
    if ( this->WidgetState == vtkSlicerAbstractWidget::Start )
      {
    this->WidgetRepresentation->VisibilityOff();
      }
    else
      {
    this->WidgetRepresentation->VisibilityOn();
      }
    this->CurrentRenderer->AddViewProp(this->WidgetRepresentation);

    if (this->Parent)
      {
      this->Parent->AddObserver(vtkCommand::KeyPressEvent,
                                this->KeyEventCallbackCommand,
                                this->Priority);
      this->Parent->AddObserver(vtkCommand::KeyReleaseEvent,
                                this->KeyEventCallbackCommand,
                                this->Priority);
      }
    else
      {
      this->Interactor->AddObserver(vtkCommand::KeyPressEvent,
                                    this->KeyEventCallbackCommand,
                                    this->Priority);
      this->Interactor->AddObserver(vtkCommand::KeyReleaseEvent,
                                    this->KeyEventCallbackCommand,
                                    this->Priority);
      }

    this->InvokeEvent(vtkCommand::EnableEvent,nullptr);
    }
  else
    {
    this->Enabled = 0;

    // don't listen for events any more
    if ( ! this->Parent )
      {
      this->Interactor->RemoveObserver(this->EventCallbackCommand);
      }
    else
      {
      this->Parent->RemoveObserver(this->EventCallbackCommand);
      }

    if (this->CurrentRenderer)
      {
      this->CurrentRenderer->RemoveViewProp(this->WidgetRepresentation);
      }

    if (this->WidgetRepresentation)
      {
      this->WidgetRepresentation->UnRegisterPickers();
      this->WidgetRepresentation->VisibilityOff();
      }

    if (this->Parent)
      {
      this->Parent->RemoveObserver(this->KeyEventCallbackCommand);
      }
    else
      {
      this->Interactor->RemoveObserver(this->KeyEventCallbackCommand);
      }

    this->InvokeEvent(vtkCommand::DisableEvent,nullptr);
    this->SetCurrentRenderer(nullptr);
    }

  this->Superclass::SetEnabled( enabling );
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetRepresentation(vtkSlicerAbstractRepresentation *rep)
{
  if ( rep == this->WidgetRepresentation || rep == nullptr)
    {
    return;
    }

  int enabled=0;
  if ( this->Enabled )
    {
    enabled = 1;
    this->SetEnabled(0);
    }

  this->WidgetRepresentation = rep;
  this->WidgetRep = rep;

  if ( this->WidgetRepresentation )
    {
    this->WidgetRepresentation->Register(this);
    }

  this->Modified();

  if ( enabled )
    {
    this->SetEnabled(1);
    }
}

//-------------------------------------------------------------------------
vtkSlicerAbstractRepresentation* vtkSlicerAbstractWidget::GetRepresentation()
{
  return this->WidgetRepresentation;
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::BuildRepresentation()
{
  if ( !this->WidgetRepresentation )
    {
    return;
    }

  this->WidgetRepresentation->BuildRepresentation();
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::BuildLocator()
{
  if ( !this->WidgetRepresentation )
    {
    return;
    }

  this->WidgetRepresentation->SetRebuildLocator(true);
}

// The following methods are the callbacks that the contour widget responds to.
//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::PickAction(vtkAbstractWidget *w)
{    
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  if ( self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
       !self->WidgetRepresentation )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  int state = self->WidgetRepresentation->ComputeInteractionState( X, Y );
  int active = self->WidgetRepresentation->ActivateNode( X, Y );
  self->SetCursor( active );
  if ( state == vtkSlicerAbstractRepresentation::Nearby && active )
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->StartInteraction();
    self->WidgetRepresentation->Highlight(1);
    self->CurrentHandle = self->WidgetRepresentation->GetActiveNode();
    self->WidgetRepresentation->SetCurrentOperationToPick();
    self->InvokeEvent( vtkCommand::StartInteractionEvent, &self->CurrentHandle );
    self->WidgetRepresentation->StartWidgetInteraction( pos );
    self->EventCallbackCommand->SetAbortFlag( 1 );
    }

  if ( self->WidgetRepresentation->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRepresentation->NeedToRenderOff();
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::RotateAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  if ( self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
       !self->WidgetRepresentation )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  int state = self->WidgetRepresentation->ComputeInteractionState( X, Y );
  int active = self->WidgetRepresentation->ActivateNode( X, Y );
  self->SetCursor( active );
  if ( state == vtkSlicerAbstractRepresentation::Nearby && active )
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->StartInteraction();
    self->WidgetRepresentation->Highlight(1);
    self->CurrentHandle = self->WidgetRepresentation->GetActiveNode();
    if ( self->CurrentHandle == -2 )
      {
      self->WidgetRepresentation->SetCurrentOperationToPick();
      }
    else
      {
      self->WidgetRepresentation->SetCurrentOperationToRotate();
      }
    self->InvokeEvent( vtkCommand::StartInteractionEvent, &self->CurrentHandle );
    self->WidgetRepresentation->StartWidgetInteraction( pos );
    self->EventCallbackCommand->SetAbortFlag( 1 );
    }

  if ( self->WidgetRepresentation->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRepresentation->NeedToRenderOff();
  }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ScaleAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);

  if ( self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
       !self->WidgetRepresentation )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  int state = self->WidgetRepresentation->ComputeInteractionState( X, Y );
  int active = self->WidgetRepresentation->ActivateNode( X, Y );
  self->SetCursor( active );
  if ( state == vtkSlicerAbstractRepresentation::Nearby && active )
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->StartInteraction();
    self->WidgetRepresentation->Highlight(1);
    self->CurrentHandle = self->WidgetRepresentation->GetActiveNode();
    self->WidgetRepresentation->SetCurrentOperationToScale();
    self->InvokeEvent( vtkCommand::StartInteractionEvent, &self->CurrentHandle );
    self->WidgetRepresentation->StartWidgetInteraction( pos );
    self->EventCallbackCommand->SetAbortFlag( 1 );
    }

  self->EventCallbackCommand->SetAbortFlag( 1 );

  if ( self->WidgetRepresentation->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRepresentation->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ShiftAction( vtkAbstractWidget *w )
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);

  if ( self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
       !self->WidgetRepresentation )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  int state = self->WidgetRepresentation->ComputeInteractionState( X, Y );
  int active = self->WidgetRepresentation->ActivateNode( X, Y );
  self->SetCursor( active );
  if ( state == vtkSlicerAbstractRepresentation::Nearby && active )
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->StartInteraction();
    self->WidgetRepresentation->Highlight(1);
    self->CurrentHandle = self->WidgetRepresentation->GetActiveNode();
    self->WidgetRepresentation->SetCurrentOperationToShift();
    self->InvokeEvent( vtkCommand::StartInteractionEvent, &self->CurrentHandle );
    self->WidgetRepresentation->StartWidgetInteraction( pos );
    self->EventCallbackCommand->SetAbortFlag( 1 );
    }

  if ( self->WidgetRepresentation->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRepresentation->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::EndAction( vtkAbstractWidget *w )
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  if ( !self->WidgetRepresentation )
    {
    return;
    }

  if ( self->WidgetRepresentation->GetCurrentOperation() != vtkSlicerAbstractRepresentation::Inactive )
    {
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    self->WidgetRepresentation->ActivateNode( X, Y );
    self->SetCursor( 0 );
    self->WidgetRepresentation->Highlight(self->WidgetRepresentation->ComputeInteractionState( X, Y ));
    self->WidgetRepresentation->SetCurrentOperationToInactive();
    self->ReleaseFocus();
    self->EventCallbackCommand->SetAbortFlag( 1 );
    self->EndInteraction();
    self->CurrentHandle = self->WidgetRepresentation->GetActiveNode();
    self->InvokeEvent( vtkCommand::EndInteractionEvent, &self->CurrentHandle );
    }

  self->Render();
  self->SelectionButton = vtkAbstractWidget::None;
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ResetAction( vtkAbstractWidget *w )
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  self->Initialize( nullptr );
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ProcessKeyEvents(vtkObject* , unsigned long vtkNotUsed(event),
                                               void* clientdata, void* )
{
  vtkSlicerAbstractWidget *self = static_cast<vtkSlicerAbstractWidget*>(clientdata);
  vtkRenderWindowInteractor *iren = self->GetInteractor();
  if ( !self->WidgetRepresentation || !iren )
    {
    return;
    }

  if ( iren->GetKeyCode() == self->HorizontalActiveKeyCode)
    {
    if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToY ||
         self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToZ )
      {
      return;
      }

    if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictNone && self->KeyCount == 1)
      {
      self->WidgetRepresentation->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToX);
      self->KeyCount = 0;
      }
    else if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToX && self->KeyCount == 1 )
      {
      self->WidgetRepresentation->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictNone);
      self->KeyCount = 0;
      }
    else
      {
      self->KeyCount++;
      }
    }
  else if (iren->GetKeyCode() == self->VerticalActiveKeyCode )
    {
    if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToX ||
         self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToZ )
      {
      return;
      }

    if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictNone && self->KeyCount == 1)
      {
      self->WidgetRepresentation->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToY);
      self->KeyCount = 0;
      }
    else if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToY && self->KeyCount == 1 )
      {
      self->WidgetRepresentation->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictNone);
      self->KeyCount = 0;
      }
    else
      {
      self->KeyCount++;
      }
    }
  else if ( iren->GetKeyCode() == self->DepthActiveKeyCode )
    {
    if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToX ||
         self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToY )
      {
      return;
      }

    if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictNone && self->KeyCount == 1)
      {
      self->WidgetRepresentation->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToZ);
      self->KeyCount = 0;
      }
    else if ( self->WidgetRepresentation->GetRestrictFlag() == vtkSlicerAbstractRepresentation::RestrictToZ && self->KeyCount == 1 )
      {
      self->WidgetRepresentation->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictNone);
      self->KeyCount = 0;
      }
    else
      {
      self->KeyCount++;
      }
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::Initialize( vtkPolyData * pd, int state)
{
  if ( !this->GetEnabled() )
    {
    vtkErrorMacro(<<"Enable widget before initializing");
    }

  if ( pd == nullptr )
    {
    return;
    }

  this->CreateDefaultRepresentation();
  this->GetRepresentation()->Initialize( pd );
  this->WidgetState = ( this->GetRepresentation()->GetClosedLoop() || state == 1 ) ?
              vtkSlicerAbstractWidget::Manipulate : vtkSlicerAbstractWidget::Define;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WidgetState: " << this->WidgetState << endl;
  os << indent << "CurrentHandle: " << this->CurrentHandle << endl;
  os << indent << "FollowCursor: " << (this->FollowCursor ? "On" : "Off") << endl;
}
