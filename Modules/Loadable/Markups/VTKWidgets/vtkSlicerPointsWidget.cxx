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
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Pick,
                                          this, vtkSlicerAbstractWidget::PickAction);
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

//-------------------------------------------------------------------------
int vtkSlicerPointsWidget::AddPointToRepresentationFromWorldCoordinate(
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
    if (this->WidgetState == vtkSlicerPointsWidget::Manipulate)
      {
      this->FollowCursor = false;
      rep->DeleteLastNode();
      }
    else if (this->FollowCursor)
      {
      rep->DeleteLastNode();
      }
    else
      {
      this->FollowCursor = true;
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
    if (this->WidgetState == vtkSlicerPointsWidget::Start)
      {
      this->InvokeEvent(vtkCommand::StartInteractionEvent, &this->CurrentHandle);
      }
    this->WidgetState = vtkSlicerPointsWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::PlacePointEvent, &this->CurrentHandle);
    this->ReleaseFocus();
    this->Render();
    if (!this->FollowCursor)
      {
      this->WidgetState = vtkSlicerPointsWidget::Manipulate;
      this->InvokeEvent(vtkCommand::EndInteractionEvent, &this->CurrentHandle);
      }
    else
      {
      rep->AddNodeAtWorldPosition(worldCoordinates);
      }
    }

  return this->CurrentHandle;
}
