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

#include "vtkSlicerPointsWidget.h"
#include "vtkSlicerPointsRepresentation3D.h"
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

vtkStandardNewMacro(vtkSlicerPointsWidget);

//----------------------------------------------------------------------
vtkSlicerPointsWidget::vtkSlicerPointsWidget()
{
  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::NoModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Select,
                                          this, vtkSlicerPointsWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSlicerPointsWidget::MoveAction);


  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 127, 1, "Delete",
                                          vtkWidgetEvent::Delete,
                                          this, vtkSlicerPointsWidget::DeleteAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 8, 1, "BackSpace",
                                          vtkWidgetEvent::Delete,
                                          this, vtkSlicerPointsWidget::DeleteAction);

  this->WidgetState = vtkSlicerPointsWidget::Manipulate;
}

//----------------------------------------------------------------------
vtkSlicerPointsWidget::~vtkSlicerPointsWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerPointsWidget::CreateDefaultRepresentation()
{
  vtkSlicerPointsRepresentation3D *rep = vtkSlicerPointsRepresentation3D::New();
  rep->SetRenderer(this->GetCurrentRenderer());
  this->SetRepresentation(rep);
}

// The following methods are the callbacks that the widget responds to.
//-------------------------------------------------------------------------
void vtkSlicerPointsWidget::SelectAction( vtkAbstractWidget *w )
{
  vtkSlicerPointsWidget *self = reinterpret_cast<vtkSlicerPointsWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  switch ( self->WidgetState )
    {
    case vtkSlicerPointsWidget::Define:
      {
      self->AddPointToRepresentation();
      break;
      }
    case vtkSlicerPointsWidget::Manipulate:
      {
      int active = self->WidgetRepresentation->ActivateNode( X, Y );
      self->SetCursor( active );
      if ( active )
        {
        self->GrabFocus(self->EventCallbackCommand);
        self->StartInteraction();
        self->CurrentHandle = self->WidgetRepresentation->GetActiveNode();
        self->WidgetRepresentation->SetCurrentOperationToTranslate();
        self->InvokeEvent( vtkCommand::StartInteractionEvent, &self->CurrentHandle );
        self->WidgetRepresentation->StartWidgetInteraction( pos );
        self->EventCallbackCommand->SetAbortFlag( 1 );
        }

      if ( self->WidgetRepresentation->GetNeedToRender() )
        {
        self->Render();
        self->WidgetRepresentation->NeedToRenderOff();
        }
      break;
      }
    }
}

//------------------------------------------------------------------------
void vtkSlicerPointsWidget::AddPointToRepresentation()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  if ( !this->WidgetRepresentation )
    {
    return;
    }

  this->CurrentHandle = this->WidgetRepresentation->GetActiveNode();

  if ( this->WidgetRepresentation->AddNodeAtDisplayPosition( X, Y ) )
    {
    this->CurrentHandle = this->WidgetRepresentation->GetActiveNode();
    if ( this->WidgetState == vtkSlicerPointsWidget::Start )
      {
      this->InvokeEvent( vtkCommand::StartInteractionEvent, &this->CurrentHandle );
      }
    this->WidgetState = vtkSlicerPointsWidget::Define;
    this->WidgetRepresentation->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::PlacePointEvent, &this->CurrentHandle );
    this->WidgetState = vtkSlicerPointsWidget::Manipulate;
    this->ReleaseFocus();
    this->Render();
    this->EventCallbackCommand->SetAbortFlag( 1 );
    this->InvokeEvent( vtkCommand::EndInteractionEvent, &this->CurrentHandle );
    this->Interactor->MouseWheelForwardEvent();
    this->Interactor->MouseWheelBackwardEvent();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerPointsWidget::AddPointToRepresentationFromWorldCoordinate(double worldCoordinates[3])
{
  if ( !this->WidgetRepresentation )
    {
    return;
    }

  this->CurrentHandle = this->WidgetRepresentation->GetActiveNode();

  if ( this->WidgetRepresentation->AddNodeAtWorldPosition( worldCoordinates ) )
    {
    this->CurrentHandle = this->WidgetRepresentation->GetActiveNode();
    if ( this->WidgetState == vtkSlicerPointsWidget::Start )
      {
      this->InvokeEvent( vtkCommand::StartInteractionEvent, &this->CurrentHandle );
      }
    this->WidgetState = vtkSlicerPointsWidget::Define;
    this->WidgetRepresentation->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::PlacePointEvent, &this->CurrentHandle );
    this->WidgetState = vtkSlicerPointsWidget::Manipulate;
    this->ReleaseFocus();
    this->Render();
    this->EventCallbackCommand->SetAbortFlag( 1 );
    this->InvokeEvent( vtkCommand::EndInteractionEvent, &this->CurrentHandle );
    this->Interactor->MouseWheelForwardEvent();
    this->Interactor->MouseWheelBackwardEvent();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerPointsWidget::MoveAction( vtkAbstractWidget *w )
{
  vtkSlicerPointsWidget *self = reinterpret_cast<vtkSlicerPointsWidget*>(w);

  if ( self->WidgetState == vtkSlicerPointsWidget::Start ||
       !self->WidgetRepresentation )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  if ( self->WidgetRepresentation->GetCurrentOperation() == vtkSlicerAbstractRepresentation::Inactive )
    {
    self->SetCursor( self->WidgetRepresentation->ActivateNode( X, Y ) );
    }

  self->CurrentHandle = self->WidgetRepresentation->GetActiveNode();
  double pos[2];
  pos[0] = X;
  pos[1] = Y;
  self->WidgetRepresentation->WidgetInteraction( pos );
  if ( self->WidgetRepresentation->GetCurrentOperation() != vtkSlicerAbstractRepresentation::Pick )
    {
    self->InvokeEvent( vtkCommand::InteractionEvent, &self->CurrentHandle );
    }

  if ( self->WidgetRepresentation->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRepresentation->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerPointsWidget::DeleteAction( vtkAbstractWidget *w )
{
  vtkSlicerPointsWidget *self = reinterpret_cast<vtkSlicerPointsWidget*>(w);

  if ( self->WidgetState != vtkSlicerPointsWidget::Manipulate ||
       !self->WidgetRepresentation )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  if ( self->WidgetRepresentation->ActivateNode( X, Y ) )
    {
    self->SetCursor( 0 );
    self->CurrentHandle = self->WidgetRepresentation->GetActiveNode();
    self->WidgetRepresentation->DeleteActiveNode();
    self->InvokeEvent( vtkCommand::DeletePointEvent, &self->CurrentHandle );
    }

  if ( self->WidgetRepresentation->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRepresentation->NeedToRenderOff();
    }
}
