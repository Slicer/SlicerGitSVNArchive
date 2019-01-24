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
  this->FollowCursor     = false;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::NoModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Select,
                                          this, vtkSlicerAbstractWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::AltModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Rotate,
                                          this, vtkSlicerAbstractWidget::RotateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Translate,
                                          this, vtkSlicerAbstractWidget::TranslateAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSlicerAbstractWidget::MoveAction);

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
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 127, 1, "Delete",
                                          vtkWidgetEvent::Delete,
                                          this, vtkSlicerAbstractWidget::DeleteAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 8, 1, "BackSpace",
                                          vtkWidgetEvent::Delete,
                                          this, vtkSlicerAbstractWidget::DeleteAction);

  this->HorizontalActiveKeyCode = 'x';
  this->VerticalActiveKeyCode = 'y';
  this->DepthActiveKeyCode = 'z';
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
  if (!this->ManagesCursor)
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
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return;
    }

  if (!rep->GetClosedLoop() && rep->GetNumberOfNodes() > 1)
    {
    this->WidgetState = vtkSlicerAbstractWidget::Manipulate;
    rep->ClosedLoopOn();
    this->Render();
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetEnabled(int enabling)
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!this->Interactor || !rep)
    {
    return;
    }

  if (enabling)
    {
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];

    if (!this->CurrentRenderer)
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
    if (!this->Parent)
      {
      this->EventTranslator->AddEventsToInteractor(this->Interactor,
        this->EventCallbackCommand,this->Priority);
      }
    else
      {
      this->EventTranslator->AddEventsToParent(this->Parent,
        this->EventCallbackCommand,this->Priority);
      }

    rep->SetRenderer(this->CurrentRenderer);
    if (this->WidgetState == vtkSlicerAbstractWidget::Start)
      {
      rep->VisibilityOff();
      }
    else
      {
      rep->VisibilityOn();
      }
    this->CurrentRenderer->AddViewProp(rep);

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
    if (!this->Parent)
      {
      this->Interactor->RemoveObserver(this->EventCallbackCommand);
      }
    else
      {
      this->Parent->RemoveObserver(this->EventCallbackCommand);
      }

    if (this->CurrentRenderer)
      {
      this->CurrentRenderer->RemoveViewProp(rep);
      }

    if (rep)
      {
      rep->UnRegisterPickers();
      rep->VisibilityOff();
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

  this->Superclass::SetEnabled(enabling);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetRepresentation(vtkSlicerAbstractRepresentation *rep)
{
  if (rep == nullptr || rep == this->WidgetRep)
    {
    return;
    }

  int enabled=0;
  if (this->Enabled)
    {
    enabled = 1;
    this->SetEnabled(0);
    }

  if (this->WidgetRep)
  {
    this->WidgetRep->Delete();
  }
  this->WidgetRep = rep;
  if (this->WidgetRep)
  {
    this->WidgetRep->Register(this);
  }
  this->Modified();

  if (enabled)
    {
    this->SetEnabled(1);
    }
}

//-------------------------------------------------------------------------
vtkSlicerAbstractRepresentation* vtkSlicerAbstractWidget::GetRepresentation()
{     
  return reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::BuildRepresentation()
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return;
    }

  rep->BuildRepresentation();
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::BuildLocator()
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return;
    }

  rep->SetRebuildLocator(true);
}

// The following methods are the callbacks that the contour widget responds to.
//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState == vtkSlicerAbstractWidget::Start ||
      !rep)
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  if (self->WidgetState == vtkSlicerAbstractWidget::Define &&
      self->FollowCursor && rep->GetNumberOfNodes() > 0)
    {
    rep->SetNthNodeDisplayPosition(rep->GetNumberOfNodes()-1, X, Y);
    }

  if (self->WidgetState == vtkSlicerAbstractWidget::Manipulate)
    {
    if (rep->GetCurrentOperation() == vtkSlicerAbstractRepresentation::Inactive)
      {
      int state = rep->ComputeInteractionState(X, Y);
      self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
      if (state == vtkSlicerAbstractRepresentation::OnControlPoint)
        {
        self->CurrentHandle = rep->GetActiveNode();
        self->InvokeEvent(vtkCommand::InteractionEvent, &self->CurrentHandle);
        }
      else if (state == vtkSlicerAbstractRepresentation::OnLine)
        {
        self->CurrentHandle = -1;
        self->InvokeEvent(vtkCommand::InteractionEvent, &self->CurrentHandle);
        }
      }
    else
      {
      double pos[2];
      pos[0] = X;
      pos[1] = Y;
      rep->WidgetInteraction(pos);
      if (rep->GetCurrentOperation() != vtkSlicerAbstractRepresentation::Pick)
        {
        if (rep->GetActiveNode() > -1)
          {
          self->CurrentHandle = rep->GetActiveNode();
          }
        else if (rep->GetInteractionState() == vtkSlicerAbstractRepresentation::OnLine)
          {
          self->CurrentHandle = -1;
          }
        self->InvokeEvent(vtkCommand::InteractionEvent, &self->CurrentHandle);
        }
      }
    }

  if (rep->GetNeedToRender())
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
      !rep)
    {
    return;
    }

  int state = rep->GetInteractionState();
  self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  if (state == vtkSlicerAbstractRepresentation::OnControlPoint)
    {
    self->CurrentHandle = rep->GetActiveNode();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else if (state == vtkSlicerAbstractRepresentation::OnLine)
    {
    self->CurrentHandle = -1;
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else
    {
    return;
    }

  self->GrabFocus(self->EventCallbackCommand);
  self->StartInteraction();
  rep->SetCurrentOperationToSelect();
  double pos[2];
  pos[0] = self->Interactor->GetEventPosition()[0];
  pos[1] = self->Interactor->GetEventPosition()[1];
  rep->StartWidgetInteraction(pos);
  self->EventCallbackCommand->SetAbortFlag(1);

  if (rep->GetNeedToRender())
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::PickAction(vtkAbstractWidget *w)
{    
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
      !rep)
    {
    return;
    }

  int state = rep->GetInteractionState();
  self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  if (state == vtkSlicerAbstractRepresentation::OnControlPoint)
    {
    self->CurrentHandle = rep->GetActiveNode();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else if (state == vtkSlicerAbstractRepresentation::OnLine)
    {
    self->CurrentHandle = -1;
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else
    {
    return;
    }

  self->GrabFocus(self->EventCallbackCommand);
  self->StartInteraction();
  rep->SetCurrentOperationToPick();
  double pos[2];
  pos[0] = self->Interactor->GetEventPosition()[0];
  pos[1] = self->Interactor->GetEventPosition()[1];
  rep->StartWidgetInteraction(pos);
  self->EventCallbackCommand->SetAbortFlag(1);

  if (rep->GetNeedToRender())
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::RotateAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
      !rep)
    {
    return;
    }

  int state = rep->GetInteractionState();
  self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  if (state == vtkSlicerAbstractRepresentation::OnControlPoint)
    {
    self->CurrentHandle = rep->GetActiveNode();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else if (state == vtkSlicerAbstractRepresentation::OnLine)
    {
    self->CurrentHandle = -1;
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else
    {
    return;
    }

  self->GrabFocus(self->EventCallbackCommand);
  self->StartInteraction();
  rep->SetCurrentOperationToRotate();
  double pos[2];
  pos[0] = self->Interactor->GetEventPosition()[0];
  pos[1] = self->Interactor->GetEventPosition()[1];
  rep->StartWidgetInteraction(pos);
  self->EventCallbackCommand->SetAbortFlag(1);

  if (rep->GetNeedToRender())
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ScaleAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
      !rep)
    {
    return;
    }

  int state = rep->GetInteractionState();
  self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  if (state == vtkSlicerAbstractRepresentation::OnControlPoint)
    {
    self->CurrentHandle = rep->GetActiveNode();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else if (state == vtkSlicerAbstractRepresentation::OnLine)
    {
    self->CurrentHandle = -1;
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else
    {
    return;
    }

  self->GrabFocus(self->EventCallbackCommand);
  self->StartInteraction();
  rep->SetCurrentOperationToScale();
  double pos[2];
  pos[0] = self->Interactor->GetEventPosition()[0];
  pos[1] = self->Interactor->GetEventPosition()[1];
  rep->StartWidgetInteraction(pos);
  self->EventCallbackCommand->SetAbortFlag(1);

  if (rep->GetNeedToRender())
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::TranslateAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState != vtkSlicerAbstractWidget::Manipulate ||
      !rep)
    {
    return;
    }

  int state = rep->GetInteractionState();
  self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  if (state == vtkSlicerAbstractRepresentation::OnControlPoint)
    {
    self->CurrentHandle = rep->GetActiveNode();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else if (state == vtkSlicerAbstractRepresentation::OnLine)
    {
    self->CurrentHandle = -1;
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &self->CurrentHandle);
    }
  else
    {
    return;
    }

  self->GrabFocus(self->EventCallbackCommand);
  self->StartInteraction();
  rep->SetCurrentOperationToTranslate();
  double pos[2];
  pos[0] = self->Interactor->GetEventPosition()[0];
  pos[1] = self->Interactor->GetEventPosition()[1];
  rep->StartWidgetInteraction(pos);
  self->EventCallbackCommand->SetAbortFlag(1);

  if (rep->GetNeedToRender())
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::EndAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (!rep)
    {
    return;
    }

  if (rep->GetCurrentOperation() != vtkSlicerAbstractRepresentation::Inactive)
    {
    rep->SetActiveNode(-1);
    self->SetCursor(0);
    rep->SetCurrentOperationToInactive();
    self->ReleaseFocus();
    self->EventCallbackCommand->SetAbortFlag(1);
    self->EndInteraction();
    self->CurrentHandle = rep->GetActiveNode();
    self->InvokeEvent(vtkCommand::EndInteractionEvent, &self->CurrentHandle);
    }

  self->Render();
  self->SelectionButton = vtkAbstractWidget::None;
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ResetAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  self->Initialize(nullptr);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::DeleteAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState == vtkSlicerAbstractWidget::Start ||
      !rep)
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  switch (self->WidgetState)
    {
    case vtkSlicerAbstractWidget::Define:
      {
      self->SetCursor(0);
      int n = rep->GetNumberOfNodes() - 2;
      rep->DeleteNthNode(n);
      self->InvokeEvent(vtkCommand::DeletePointEvent, &n);
      break;
      }
    case vtkSlicerAbstractWidget::Manipulate:
      {
      if (rep->ActivateNode(X, Y))
        {
        self->SetCursor(0);
        self->CurrentHandle = rep->GetActiveNode();
        rep->DeleteActiveNode();
        self->InvokeEvent(vtkCommand::DeletePointEvent, &self->CurrentHandle);
        }
      break;
      }
    }

  rep->ActivateNode(X, Y);

  if (rep->GetNeedToRender())
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}


//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ProcessKeyEvents(vtkObject* , unsigned long vtkNotUsed(event),
                                               void* clientdata, void*)
{
  vtkSlicerAbstractWidget *self = static_cast<vtkSlicerAbstractWidget*>(clientdata);
  vtkRenderWindowInteractor *iren = self->GetInteractor();
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (!rep || !iren)
    {
    return;
    }

  if (iren->GetKeyCode() == self->HorizontalActiveKeyCode)
    {
    rep->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToX);
    }
  else if (iren->GetKeyCode() == self->VerticalActiveKeyCode)
    {
    // RAS coordinates (Y <-> Z), this should take care also of transform?
    rep->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToZ);
    }
  else if (iren->GetKeyCode() == self->DepthActiveKeyCode)
    {
    // RAS coordinates (Y <-> Z)
    rep->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToY);
    }
  else if (iren->GetKeyCode() == 'q')
    {
    rep->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictNone);
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::Initialize(vtkPolyData * pd, int state)
{
  if (!this->GetEnabled())
    {
    vtkErrorMacro(<<"Enable widget before initializing");
    }

  if (pd == nullptr)
    {
    return;
    }

  this->CreateDefaultRepresentation();
  this->GetRepresentation()->Initialize(pd);
  this->WidgetState = (this->GetRepresentation()->GetClosedLoop() || state == 1) ?
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
