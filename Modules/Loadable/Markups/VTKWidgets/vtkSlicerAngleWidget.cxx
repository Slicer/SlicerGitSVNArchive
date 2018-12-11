/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerAngleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlicerAngleWidget.h"
#include "vtkSlicerLineRepresentation.h"
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
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkSlicerAngleWidget);

//----------------------------------------------------------------------
vtkSlicerAngleWidget::vtkSlicerAngleWidget()
{
  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::NoModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Select,
                                          this, vtkSlicerAngleWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::AddFinalPoint,
                                          this, vtkSlicerAngleWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSlicerAngleWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonDoubleClickEvent,
                                          vtkWidgetEvent::PickThree,
                                          this, vtkSlicerAbstractWidget::PickAction);

  this->CreateDefaultRepresentation();
}

//----------------------------------------------------------------------
vtkSlicerAngleWidget::~vtkSlicerAngleWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerAngleWidget::CreateDefaultRepresentation()
{
  if ( !this->WidgetRep )
  {
    vtkSlicerLineRepresentation *rep =
      vtkSlicerLineRepresentation::New();

    this->WidgetRep = rep;
  }
}

// The following methods are the callbacks that the widget responds to.
//-------------------------------------------------------------------------
void vtkSlicerAngleWidget::SelectAction( vtkAbstractWidget *w )
{
  vtkSlicerAngleWidget *self = reinterpret_cast<vtkSlicerAngleWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  switch ( self->WidgetState )
  {
    case vtkSlicerAngleWidget::Start:
    case vtkSlicerAngleWidget::Define:
    {
      // If we are following the cursor, let's add 2 nodes rightaway, on the
      // first click. The second node is the one that follows the cursor
      // around.
      if ( self->FollowCursor && (rep->GetNumberOfNodes() == 0) )
      {
        self->AddNode();
      }

      self->AddNode();
      break;
    }

    case vtkSlicerAngleWidget::Manipulate:
    {
      if ( rep->ComputeInteractionState( X, Y ) == vtkSlicerAbstractRepresentation::Nearby )
      {
        rep->ActivateNode( X, Y );
        self->GrabFocus(self->EventCallbackCommand);
        self->StartInteraction();
        self->CurrentHandle = rep->GetActiveNode();
        rep->SetCurrentOperationToTranslate();
        self->InvokeEvent( vtkCommand::StartInteractionEvent, &self->CurrentHandle );
        rep->StartWidgetInteraction( pos );
        self->EventCallbackCommand->SetAbortFlag( 1 );
      }
      break;
    }
  }

  if ( rep->GetNeedToRender() )
  {
    self->Render();
    rep->NeedToRenderOff();
  }
}

//------------------------------------------------------------------------
void vtkSlicerAngleWidget::AddNode()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // If the rep already has at least 2 nodes, check how close we are to
  // the first
  vtkSlicerAbstractRepresentation* rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
  this->CurrentHandle = rep->GetActiveNode();

  int numNodes = rep->GetNumberOfNodes();
  if ( numNodes > 2 )
  {
    this->WidgetState = vtkSlicerAngleWidget::Manipulate;
    this->ReleaseFocus();
    this->Render();
    this->EventCallbackCommand->SetAbortFlag( 1 );
    this->InvokeEvent( vtkCommand::EndInteractionEvent, &this->CurrentHandle );
    this->Interactor->MouseWheelForwardEvent();
    this->Interactor->MouseWheelBackwardEvent();
    return;
  }

  if ( rep->AddNodeAtDisplayPosition( X, Y ) )
  {
    this->GrabFocus(this->EventCallbackCommand);
    rep->ActivateNode( X, Y );
    this->CurrentHandle = rep->GetActiveNode();
    if ( this->WidgetState == vtkSlicerAngleWidget::Start )
    {
      this->InvokeEvent( vtkCommand::StartInteractionEvent, &this->CurrentHandle );
    }

    this->WidgetState = vtkSlicerAngleWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::PlacePointEvent, &this->CurrentHandle );
  }
}

//-------------------------------------------------------------------------
void vtkSlicerAngleWidget::ScaleAction(vtkAbstractWidget *w)
{
  vtkSlicerAngleWidget *self = reinterpret_cast<vtkSlicerAngleWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState == vtkSlicerAngleWidget::Manipulate)
  {
    vtkSlicerAbstractRepresentation *rep =
      reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    double pos[2];
    pos[0] = X;
    pos[1] = Y;

    if ( rep->ComputeInteractionState( X, Y ) == vtkSlicerAbstractRepresentation::Nearby )
    {
      rep->ActivateNode( X, Y );
      self->GrabFocus(self->EventCallbackCommand);
      self->StartInteraction();
      self->CurrentHandle = rep->GetActiveNode();
      rep->SetCurrentOperationToScale();
      self->InvokeEvent( vtkCommand::StartInteractionEvent, &self->CurrentHandle );
      rep->StartWidgetInteraction( pos );
      self->EventCallbackCommand->SetAbortFlag( 1 );
    }
  }

  if ( rep->GetNeedToRender() )
  {
    self->Render();
    rep->NeedToRenderOff();
  }
}

//-------------------------------------------------------------------------
void vtkSlicerAngleWidget::MoveAction( vtkAbstractWidget *w )
{
  vtkSlicerAngleWidget *self = reinterpret_cast<vtkSlicerAngleWidget*>(w);

  if ( self->WidgetState == vtkSlicerAngleWidget::Start )
  {
    return;
  }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ( self->WidgetState == vtkSlicerAngleWidget::Define )
  {
    if ( self->FollowCursor )
    {
      // Have the last node follow the mouse in this case...
      const int numNodes = rep->GetNumberOfNodes();
      if ( numNodes > 0 )
      {
      rep->SetNthNodeDisplayPosition( numNodes-1, X, Y );
      }
    }
    else
    {
      return;
    }
  }

  if ( rep->GetCurrentOperation() == vtkSlicerAbstractRepresentation::Inactive )
  {
    int state = rep->ComputeInteractionState( X, Y );
    rep->ActivateNode( X, Y );
    if ( state == vtkSlicerAbstractRepresentation::Nearby )
    {
      rep->Highlight(1);
      self->CurrentHandle = rep->GetActiveNode();
      self->InvokeEvent( vtkCommand::InteractionEvent, &self->CurrentHandle );
    }
    else
    {
      rep->Highlight(0);
    }
  }
  else
  {
    if ( rep->GetInteractionState() == vtkSlicerAbstractRepresentation::Nearby )
    {
      self->CurrentHandle = rep->GetActiveNode();
      double pos[2];
      pos[0] = X;
      pos[1] = Y;
      self->WidgetRep->WidgetInteraction( pos );
      if ( rep->GetCurrentOperation() != vtkSlicerAbstractRepresentation::Pick )
      {
        self->InvokeEvent( vtkCommand::InteractionEvent, &self->CurrentHandle );
      }
    }
  }

  if ( self->WidgetRep->GetNeedToRender() )
  {
    self->Render();
    self->WidgetRep->NeedToRenderOff();
  }
}
