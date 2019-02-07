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

#include "vtkSlicerCurveWidget.h"
#include "vtkSlicerCurveRepresentation3D.h"
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
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::AltModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Rotate,
                                          this, vtkSlicerAbstractWidget::RotateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::ControlModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::AddPoint,
                                          this, vtkSlicerCurveWidget::AddPointOnCurveAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkEvent::NoModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Pick,
                                          this, vtkSlicerAbstractWidget::PickAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkEvent::AltModifier, 0, 0, nullptr,
                                          vtkWidgetEvent::Scale,
                                          this, vtkSlicerAbstractWidget::ScaleAction);
}

//----------------------------------------------------------------------
vtkSlicerCurveWidget::~vtkSlicerCurveWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerCurveWidget::CreateDefaultRepresentation()
{
  vtkSlicerCurveRepresentation3D *rep = vtkSlicerCurveRepresentation3D::New();
  rep->SetRenderer(this->GetCurrentRenderer());
  this->SetRepresentation(rep);
}

//-------------------------------------------------------------------------
int vtkSlicerCurveWidget::AddPointToRepresentationFromWorldCoordinate(
  double worldCoordinates[3], bool persistence /*= false*/)
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return -1;
    }

  if (persistence)
    {
    if (this->WidgetState == vtkSlicerCurveWidget::Manipulate)
      {
      this->FollowCursor = false;
      rep->DeleteLastNode();
      }
    else if (this->FollowCursor)
      {
      rep->DeleteLastNode();
      }
    }
  else if (this->FollowCursor)
    {
    rep->DeleteLastNode();
    this->FollowCursor = false;
    }

  if (rep->AddNodeAtWorldPosition(worldCoordinates))
    {
    this->CurrentHandle = rep->GetActiveNode();
    if (this->WidgetState == vtkSlicerCurveWidget::Start)
      {
      this->InvokeEvent(vtkCommand::StartInteractionEvent, &this->CurrentHandle);
      }
    this->WidgetState = vtkSlicerCurveWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::PlacePointEvent, &this->CurrentHandle);
    this->ReleaseFocus();
    this->Render();
    if (!this->FollowCursor)
      {
      this->WidgetState = vtkSlicerCurveWidget::Manipulate;
      this->InvokeEvent(vtkCommand::EndInteractionEvent, &this->CurrentHandle);
      }
    else
      {
      rep->AddNodeAtWorldPosition(worldCoordinates);
      }
    }

  return this->CurrentHandle;
}

//----------------------------------------------------------------------
void vtkSlicerCurveWidget::AddPointOnCurveAction(vtkAbstractWidget *w)
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
      rep->SetCurrentOperationToPick();
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

