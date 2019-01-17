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

#include "vtkSlicerLineWidget.h"
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

vtkStandardNewMacro(vtkSlicerLineWidget);

//----------------------------------------------------------------------
vtkSlicerLineWidget::vtkSlicerLineWidget()
{
  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::NoModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Select,
                                          this, vtkSlicerLineWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::AddFinalPoint,
                                          this, vtkSlicerLineWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSlicerLineWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonDoubleClickEvent,
                                          vtkWidgetEvent::PickThree,
                                          this, vtkSlicerAbstractWidget::PickAction);

  this->CreateDefaultRepresentation();
}

//----------------------------------------------------------------------
vtkSlicerLineWidget::~vtkSlicerLineWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerLineWidget::CreateDefaultRepresentation()
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
void vtkSlicerLineWidget::SelectAction( vtkAbstractWidget *w )
{
  vtkSlicerLineWidget *self = reinterpret_cast<vtkSlicerLineWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  switch ( self->WidgetState )
  {
    case vtkSlicerLineWidget::Start:
    case vtkSlicerLineWidget::Define:
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

    case vtkSlicerLineWidget::Manipulate:
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

//-------------------------------------------------------------------------
void vtkSlicerLineWidget::ScaleAction(vtkAbstractWidget *w)
{
  vtkSlicerLineWidget *self = reinterpret_cast<vtkSlicerLineWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState == vtkSlicerLineWidget::Manipulate)
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

//------------------------------------------------------------------------
void vtkSlicerLineWidget::AddNode()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // If the rep already has at least 2 nodes, check how close we are to
  // the first
  vtkSlicerAbstractRepresentation* rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
  this->CurrentHandle = rep->GetActiveNode();

  int numNodes = rep->GetNumberOfNodes();
  if ( numNodes > 1 )
  {
    this->WidgetState = vtkSlicerLineWidget::Manipulate;
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
    rep->ActivateNode( X, Y );
    this->CurrentHandle = rep->GetActiveNode();
    if ( this->WidgetState == vtkSlicerLineWidget::Start )
    {
      this->InvokeEvent( vtkCommand::StartInteractionEvent, &this->CurrentHandle );
    }

    this->WidgetState = vtkSlicerLineWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::PlacePointEvent, &this->CurrentHandle );
  }
}

//-------------------------------------------------------------------------
void vtkSlicerLineWidget::MoveAction( vtkAbstractWidget *w )
{
  vtkSlicerLineWidget *self = reinterpret_cast<vtkSlicerLineWidget*>(w);

  if ( self->WidgetState == vtkSlicerLineWidget::Start )
  {
    return;
  }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ( self->WidgetState == vtkSlicerLineWidget::Define )
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
