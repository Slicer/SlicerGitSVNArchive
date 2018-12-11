/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerCurveWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlicerCurveWidget.h"
#include "vtkSlicerCurveRepresentation.h"
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

vtkStandardNewMacro(vtkSlicerCurveWidget);

//----------------------------------------------------------------------
vtkSlicerCurveWidget::vtkSlicerCurveWidget()
{
  this->ClosedLoop = 1;
  this->ForceLoopOpen = 0;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::NoModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Select,
                                          this, vtkSlicerCurveWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::AddFinalPoint,
                                          this, vtkSlicerCurveWidget::AddFinalPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSlicerCurveWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonDoubleClickEvent,
                                          vtkWidgetEvent::AddPoint,
                                          this, vtkSlicerCurveWidget::AddPointAction);

  this->CreateDefaultRepresentation();
}

//----------------------------------------------------------------------
vtkSlicerCurveWidget::~vtkSlicerCurveWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerCurveWidget::CreateDefaultRepresentation()
{
  if ( !this->WidgetRep )
  {
    vtkSlicerCurveRepresentation *rep =
      vtkSlicerCurveRepresentation::New();

    this->WidgetRep = rep;
  }
}

// The following methods are the callbacks that the widget responds to.
//-------------------------------------------------------------------------
void vtkSlicerCurveWidget::SelectAction( vtkAbstractWidget *w )
{
  vtkSlicerCurveWidget *self = reinterpret_cast<vtkSlicerCurveWidget*>(w);
  vtkSlicerCurveRepresentation *rep =
    reinterpret_cast<vtkSlicerCurveRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  switch ( self->WidgetState )
  {
    case vtkSlicerCurveWidget::Start:
    case vtkSlicerCurveWidget::Define:
    {
      // If we are following the cursor, let's add 2 nodes rightaway, on the
      // first click. The second node is the one that follows the cursor
      // around.
      if ( self->FollowCursor && (rep->GetNumberOfNodes() == 0) )
      {
        self->AddNode();
      }

      self->AddNode();

      if ((self->ClosedLoop && !self->FollowCursor && rep->GetNumberOfNodes() > 1) ||
          (self->ClosedLoop && self->FollowCursor && rep->GetNumberOfNodes() > 2))
      {
        rep->ClosedLoopOn();
      }
      break;
    }

    case vtkSlicerCurveWidget::Manipulate:
    {
      if ( rep->ComputeInteractionState( X, Y ) == vtkSlicerAbstractRepresentation::Nearby )
      {
        rep->ActivateNode( X, Y );
        self->GrabFocus(self->EventCallbackCommand);
        self->StartInteraction();
        self->CurrentHandle = rep->GetActiveNode();
        if (self->CurrentHandle == -2)
        {
          rep->SetCurrentOperationToPick();
        }
        else
        {
          rep->SetCurrentOperationToTranslate();
        }
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

//-------------------------------------------------------------------------
void vtkSlicerCurveWidget::AddFinalPointAction(vtkAbstractWidget *w)
{
  vtkSlicerCurveWidget *self = reinterpret_cast<vtkSlicerCurveWidget*>(w);
  vtkSlicerCurveRepresentation *rep =
    reinterpret_cast<vtkSlicerCurveRepresentation*>(self->WidgetRep);

  if ( self->WidgetState != vtkSlicerCurveWidget::Manipulate &&
       rep->GetNumberOfNodes() >= 1 )
  {
    if (self->ClosedLoop && self->FollowCursor && rep->GetNumberOfNodes() < 3)
    {
      return;
    }

    if (self->ClosedLoop && !self->FollowCursor && rep->GetNumberOfNodes() < 2)
    {
      return;
    }

    // In follow cursor mode, the "extra" node
    // has already been added for us.
    if ( !self->FollowCursor )
    {
      self->AddNode();
    }

    self->ReleaseFocus();
    self->WidgetState = vtkSlicerCurveWidget::Manipulate;
    self->EventCallbackCommand->SetAbortFlag( 1 );
    self->CurrentHandle = rep->GetActiveNode();
    self->InvokeEvent( vtkCommand::EndInteractionEvent, &self->CurrentHandle );
    self->Interactor->MouseWheelForwardEvent();
    self->Interactor->MouseWheelBackwardEvent();
  }

  if (self->WidgetState == vtkSlicerCurveWidget::Manipulate)
  {
    vtkSlicerCurveRepresentation *rep =
      reinterpret_cast<vtkSlicerCurveRepresentation*>(self->WidgetRep);

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
      if (self->CurrentHandle == -2)
      {
        rep->SetCurrentOperationToPick();
      }
      else
      {
        rep->SetCurrentOperationToScale();
      }
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

//------------------------------------------------------------------------
void vtkSlicerCurveWidget::AddNode()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // If the rep already has at least 2 nodes, check how close we are to
  // the first
  vtkSlicerCurveRepresentation* rep =
    reinterpret_cast<vtkSlicerCurveRepresentation*>(this->WidgetRep);
  this->CurrentHandle = rep->GetActiveNode();

  if ( rep->AddNodeAtDisplayPosition( X, Y ) )
  {
    rep->ActivateNode( X, Y );
    this->CurrentHandle = rep->GetActiveNode();
    if ( this->WidgetState == vtkSlicerCurveWidget::Start )
    {
      this->InvokeEvent( vtkCommand::StartInteractionEvent, &this->CurrentHandle );
    }

    this->WidgetState = vtkSlicerCurveWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::PlacePointEvent, &this->CurrentHandle );
  }
}

//-------------------------------------------------------------------------
void vtkSlicerCurveWidget::MoveAction( vtkAbstractWidget *w )
{
  vtkSlicerCurveWidget *self = reinterpret_cast<vtkSlicerCurveWidget*>(w);

  if ( self->WidgetState == vtkSlicerCurveWidget::Start )
  {
    return;
  }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  vtkSlicerCurveRepresentation *rep =
    reinterpret_cast<vtkSlicerCurveRepresentation*>(self->WidgetRep);

  if ( self->WidgetState == vtkSlicerCurveWidget::Define )
  {
    if ( self->FollowCursor )
    {
      // Have the last node follow the mouse in this case...
      const int numNodes = rep->GetNumberOfNodes();

      // First check if the last node is near the first node, if so, we intend
      // closing the loop.
      if ( numNodes > 1 )
      {
        double displayPos[2];
        int pixelTolerance  = rep->GetPixelTolerance();
        int pixelTolerance2 = pixelTolerance * pixelTolerance;

        rep->GetNthNodeDisplayPosition( 0, displayPos );

        int distance2 = static_cast<int>((X - displayPos[0]) * (X - displayPos[0]) +
                                         (Y - displayPos[1]) * (Y - displayPos[1]));

        const bool mustCloseLoop = ( distance2 < pixelTolerance2 && numNodes > 2 );
        if ( mustCloseLoop != ( rep->GetClosedLoop() == 1 ) && !self->ClosedLoop )
        {
          if ( rep->GetClosedLoop() )
          {
            // We need to open the closed loop.
            // We do this by adding a node at (X,Y). If by chance the point
            // placer says that (X,Y) is invalid, we'll add it at the location
            // of the first control point (which we know is valid).

            if ( !rep->AddNodeAtDisplayPosition( X, Y ) )
            {
              double closedLoopPoint[3];
              rep->GetNthNodeWorldPosition( 0, closedLoopPoint );
              rep->AddNodeAtWorldPosition( closedLoopPoint );
            }
            rep->ClosedLoopOff();
            rep->UpdateCentroidPoint();
            rep->ActivateNode( X, Y );
            self->CurrentHandle = rep->GetActiveNode();
            self->InvokeEvent( vtkCommand::PlacePointEvent, &self->CurrentHandle );
          }
          else if ( !self->ForceLoopOpen )
          {
            // We need to close the open loop. Delete the node that's following
            // the mouse cursor and close the loop between the previous node and
            // the first node.
            rep->DeleteLastNode();
            rep->ClosedLoopOn();
            rep->UpdateCentroidPoint();
          }
        }
        else if ( rep->GetClosedLoop() == 0 )
        {
          // If we aren't changing the loop topology, simply update the position
          // of the latest node to follow the mouse cursor position (X,Y).
          rep->SetNthNodeDisplayPosition( numNodes-1, X, Y );
        }
        else if (numNodes > 2 && self->ClosedLoop)
        {
         rep->SetNthNodeDisplayPosition( numNodes-1, X, Y );
         rep->UpdateCentroidPoint();
        }
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
      if ( rep->GetCurrentOperation() != vtkSlicerCurveRepresentation::Pick )
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

//-------------------------------------------------------------------------
void vtkSlicerCurveWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkSlicerCurveWidget *self = reinterpret_cast<vtkSlicerCurveWidget*>(w);
  if ( self->WidgetState != vtkSlicerCurveWidget::Manipulate)
  {
    return;
  }

  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  if ( rep->AddNodeOnWidget( X, Y ) )
  {
    if ( rep->ActivateNode( X, Y ) )
    {
      self->GrabFocus(self->EventCallbackCommand);
      self->StartInteraction();
      rep->StartWidgetInteraction( pos );
      self->CurrentHandle = rep->GetActiveNode();
      rep->SetCurrentOperationToTranslate();
      self->InvokeEvent( vtkCommand::PlacePointEvent, &self->CurrentHandle );
      self->EventCallbackCommand->SetAbortFlag( 1 );
    }
  }

  if ( rep->GetNeedToRender() )
  {
    self->Render();
    rep->NeedToRenderOff();
  }
}
