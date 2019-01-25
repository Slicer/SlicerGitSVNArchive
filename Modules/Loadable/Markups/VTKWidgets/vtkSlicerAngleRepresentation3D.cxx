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

#include "vtkSlicerAngleRepresentation3D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLActor.h"
#include "vtkActor2D.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkGlyph3D.h"
#include "vtkCursor2D.h"
#include "vtkCylinderSource.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkCamera.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkFocalPlanePointPlacer.h"
#include "vtkBezierSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkPropPicker.h"
#include "vtkPickingManager.h"
#include "vtkAppendPolyData.h"
#include "vtkStringArray.h"
#include "vtkTubeFilter.h"
#include "vtkOpenGLTextActor.h"
#include "cmath"
#include "vtkArcSource.h"
#include "vtkTextProperty.h"
#include "vtkMRMLMarkupsDisplayNode.h"

vtkStandardNewMacro(vtkSlicerAngleRepresentation3D);

//----------------------------------------------------------------------
vtkSlicerAngleRepresentation3D::vtkSlicerAngleRepresentation3D()
{
  this->Line = vtkPolyData::New();
  this->Arc = vtkArcSource::New();
  this->Arc->SetResolution(30);

  this->TubeFilter = vtkTubeFilter::New();
  this->TubeFilter->SetInputData(this->Line);
  this->TubeFilter->SetNumberOfSides(20);
  this->TubeFilter->SetRadius(1);

  this->ArcTubeFilter = vtkTubeFilter::New();
  this->ArcTubeFilter->SetInputConnection(this->Arc->GetOutputPort());
  this->ArcTubeFilter->SetNumberOfSides(20);
  this->ArcTubeFilter->SetRadius(1);

  this->LineMapper = vtkOpenGLPolyDataMapper::New();
  this->LineMapper->SetInputConnection(this->TubeFilter->GetOutputPort());
  this->LineMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->LineMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
  this->LineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
  this->LineMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->ArcMapper = vtkOpenGLPolyDataMapper::New();
  this->ArcMapper->SetInputConnection(this->ArcTubeFilter->GetOutputPort());
  this->ArcMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->ArcMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
  this->ArcMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
  this->ArcMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->LineActor = vtkOpenGLActor::New();
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->SetProperty(this->Property);

  this->ArcActor = vtkOpenGLActor::New();
  this->ArcActor->SetMapper(this->ArcMapper);
  this->ArcActor->SetProperty(this->Property);

  this->TextActor = vtkOpenGLTextActor::New();
  this->TextActor->SetInput("0");
  this->TextActor->SetTextProperty(this->TextProperty);

  this->LabelFormat = new char[8];
  snprintf(this->LabelFormat,8,"%s","%-#6.3g");

  //Manage the picking
  this->LinePicker = vtkPropPicker::New();
  this->LinePicker->PickFromListOn();
  this->LinePicker->InitializePickList();
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->AddPickList(this->ArcActor);

  this->appendActors = vtkAppendPolyData::New();
  this->appendActors->AddInputData(this->CursorShape);
  this->appendActors->AddInputData(this->SelectedCursorShape);
  this->appendActors->AddInputData(this->ActiveCursorShape);
  this->appendActors->AddInputData(this->TubeFilter->GetOutput());
  this->appendActors->AddInputData(this->ArcTubeFilter->GetOutput());
}

//----------------------------------------------------------------------
vtkSlicerAngleRepresentation3D::~vtkSlicerAngleRepresentation3D()
{
  this->Line->Delete();
  this->Arc->Delete();
  this->LineMapper->Delete();
  this->ArcMapper->Delete();
  this->LineActor->Delete();
  this->ArcActor->Delete();
  this->LinePicker->Delete();
  this->TubeFilter->Delete();
  this->ArcTubeFilter->Delete();
  this->appendActors->Delete();

  this->TextActor->Delete();
  delete [] this->LabelFormat;
  this->LabelFormat = nullptr;
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::TranslateWidget(double eventPos[2])
{
  // If any node is locked return
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (this->GetNthNodeLocked(i))
      {
      return;
      }
    }

  this->Superclass::TranslateWidget(eventPos);
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::ScaleWidget(double eventPos[2])
{
  if (!this->MarkupsNode || this->GetNumberOfNodes() < 3)
    {
    return;
    }

  double ref[3] = {0.};
  double displayPos[2] = {0.};
  double worldPos[3], worldOrient[9];

  displayPos[0] = this->LastEventPosition[0];
  displayPos[1] = this->LastEventPosition[1];

  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                              displayPos, ref, worldPos,
                                              worldOrient))
    {
    for (int i = 0; i < 3; i++)
      {
      ref[i] = worldPos[i];
      }
    }
  else
    {
    return;
    }

  displayPos[0] = eventPos[0];
  displayPos[1] = eventPos[1];

  double centralPointWorldPos[3];
  this->GetNthNodeWorldPosition(1, centralPointWorldPos);

  double r2 = vtkMath::Distance2BetweenPoints(ref, centralPointWorldPos);

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient))
    {
    return;
    }
  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centralPointWorldPos);
  if (d2 < 0.0000001)
    {
    return;
    }

  double ratio = sqrt(d2 / r2);

  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (this->GetNthNodeLocked(i))
      {
      continue;
      }

    this->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
      {
      worldPos[j] = centralPointWorldPos[j] + ratio * (ref[j] - centralPointWorldPos[j]);
      }

    this->SetNthNodeWorldPosition(i, worldPos);
    }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::RotateWidget(double eventPos[2])
{
  if (!this->MarkupsNode || this->GetNumberOfNodes() < 3)
    {
    return;
    }

  // If center node is locked rotate. Anyway, it will not move
  if (this->GetNthNodeLocked(0) || this->GetNthNodeLocked(2))
    {
    return;
    }

  double ref[3] = {0.};
  double displayPos[2] = {0.};
  double lastWorldPos[3] = {0.};
  double worldPos[3], worldOrient[9];

  displayPos[0] = this->LastEventPosition[0];
  displayPos[1] = this->LastEventPosition[1];

  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                              displayPos, ref, lastWorldPos,
                                              worldOrient))
    {
    for (int i = 0; i < 3; i++)
      {
      ref[i] = worldPos[i];
      }
    }
  else
    {
    return;
    }

  displayPos[0] = eventPos[0];
  displayPos[1] = eventPos[1];

  double centralPointWorldPos[3];
  this->GetNthNodeWorldPosition(1, centralPointWorldPos);

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient))
    {
    return;
    }

  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centralPointWorldPos);
  if (d2 < 0.0000001)
    {
    return;
    }

  for (int i = 0; i < 3; i++)
    {
    lastWorldPos[i] -= centralPointWorldPos[i];
    worldPos[i] -= centralPointWorldPos[i];
    }
  double angle = -vtkMath::DegreesFromRadians
                 (vtkMath::AngleBetweenVectors(lastWorldPos, worldPos));

  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (i == 1)
      {
      continue;
      }
    this->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
      {
      ref[j] -= centralPointWorldPos[j];
      }
    vtkNew<vtkTransform> RotateTransform;
    RotateTransform->RotateY(angle);
    RotateTransform->TransformPoint(ref, worldPos);

    for (int j = 0; j < 3; j++)
      {
      worldPos[j] += centralPointWorldPos[j];
      }

    this->SetNthNodeWorldPosition(i, worldPos);
    }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::BuildLines()
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> Line;

  int i, j;
  vtkIdType index = 0;

  int numberOfNodes = this->GetNumberOfNodes();
  int count = numberOfNodes;
  for (i = 0; i < numberOfNodes; i++)
    {
    count += this->GetNumberOfIntermediatePoints(i);
    }

  points->SetNumberOfPoints(count);
  vtkIdType numLine = count;
  if (numLine > 0)
    {
    vtkIdType *lineIndices = new vtkIdType[numLine];

    double pos[3];
    for (i = 0; i < numberOfNodes; i++)
      {
      // Add the node
      this->GetNthNodeWorldPosition(i, pos);
      points->InsertPoint(index, pos);
      lineIndices[index] = index;
      index++;

      int numIntermediatePoints = this->GetNumberOfIntermediatePoints(i);

      for (j = 0; j < numIntermediatePoints; j++)
        {
        this->GetIntermediatePointWorldPosition(i, j, pos);
        points->InsertPoint(index, pos);
        lineIndices[index] = index;
        index++;
        }
      }

    Line->InsertNextCell(numLine, lineIndices);
    delete [] lineIndices;
    }

  this->Line->SetPoints(points);
  this->Line->SetLines(Line);

  // Build Arc
  if (this->GetNumberOfNodes() != 3)
    {
    return;
    }

  double p1[3], p2[3], c[3], vector2[3], vector1[3];
  double l1 = 0.0, l2 = 0.0;
  this->GetNthNodeWorldPosition(0, p1);
  this->GetNthNodeWorldPosition(1, c);
  this->GetNthNodeWorldPosition(2, p2);

  // Compute the angle (only if necessary since we don't want
  // fluctuations in angle value as the camera moves, etc.)
  if (fabs(p1[0]-c[0]) < 0.001 || fabs(p2[0]-c[0]) < 0.001)
    {
    return;
    }

  vector1[0] = p1[0] - c[0];
  vector1[1] = p1[1] - c[1];
  vector1[2] = p1[2] - c[2];
  vector2[0] = p2[0] - c[0];
  vector2[1] = p2[1] - c[1];
  vector2[2] = p2[2] - c[2];
  l1 = vtkMath::Normalize(vector1);
  l2 = vtkMath::Normalize(vector2);
  double angle = acos(vtkMath::Dot(vector1, vector2));

  // Place the label and place the arc
  const double length = l1 < l2 ? l1 : l2;
  const double anglePlacementRatio = 0.5;
  const double l = length * anglePlacementRatio;
  double arcp1[3] = {l * vector1[0] + c[0],
                     l * vector1[1] + c[1],
                     l * vector1[2] + c[2]};
  double arcp2[3] = {l * vector2[0] + c[0],
                     l * vector2[1] + c[1],
                     l * vector2[2] + c[2]};

  this->Arc->SetPoint1(arcp1);
  this->Arc->SetPoint2(arcp2);
  this->Arc->SetCenter(c);
  this->Arc->Update();

  char string[512];
  snprintf(string, sizeof(string), this->LabelFormat, vtkMath::DegreesFromRadians(angle));
  this->TextActor->SetInput(string);

  double textPos[3], vector3[3];
  vector3[0] = vector1[0] + vector2[0];
  vector3[1] = vector1[1] + vector2[1];
  vector3[2] = vector1[2] + vector2[2];
  vtkMath::Normalize(vector3);
  textPos[0] = c[0] + vector3[0] * length * 0.6;
  textPos[1] = c[1] + vector3[1] * length * 0.6;
  textPos[2] = c[2] + vector3[2] * length * 0.6;
  this->Renderer->SetWorldPoint(textPos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(textPos);

  int X = static_cast<int>(textPos[0]);
  int Y = static_cast<int>(textPos[1]);
  this->TextActor->SetDisplayPosition(X,Y);
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  this->UpdateWidget(true);

  if (this->MarkupsNode == nullptr)
    {
    return;
    }

  vtkMRMLMarkupsDisplayNode* display = vtkMRMLMarkupsDisplayNode::SafeDownCast
    (this->MarkupsNode->GetDisplayNode());
  if (display == nullptr)
    {
    return;
    }

  if (display->GetTextVisibility())
    {
    LabelsActor->VisibilityOn();
    SelectedLabelsActor->VisibilityOn();
    ActiveLabelsActor->VisibilityOn();
    TextActor->VisibilityOn();
    }
  else
    {
    LabelsActor->VisibilityOff();
    SelectedLabelsActor->VisibilityOff();
    ActiveLabelsActor->VisibilityOff();
    TextActor->VisibilityOff();
    }

  if (this->AlwaysOnTop)
    {
    // max value 65536 so we subtract 66000 to make sure we are
    // zero or negative
    this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->SelectedMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->SelectedMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->SelectedMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->LineMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->LineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->LineMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->ArcMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->ArcMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->ArcMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    }
  else
    {
    this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->SelectedMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->SelectedMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    this->SelectedMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->LineMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->LineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    this->LineMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->ArcMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->ArcMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -1);
    this->ArcMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    }

  this->Glypher->SetScaleFactor(this->HandleSize);
  this->SelectedGlypher->SetScaleFactor(this->HandleSize);
  this->ActiveGlypher->SetScaleFactor(this->HandleSize);
  this->TubeFilter->SetRadius(this->HandleSize * 0.125);
  this->ArcTubeFilter->SetRadius(this->HandleSize * 0.125);
  this->BuildRepresentationPointsAndLabels();

  bool lineVisibility = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (!this->GetNthNodeVisibility(ii))
      {
      lineVisibility = false;
      break;
      }
    }

  this->LineActor->SetVisibility(lineVisibility);
  this->ArcActor->SetVisibility(lineVisibility && this->GetNumberOfNodes() == 3);
  this->TextActor->SetVisibility(lineVisibility && this->GetNumberOfNodes() == 3);

  if (this->GetActiveNode() == -2)
    {
    this->LineActor->SetProperty(this->ActiveProperty);
    this->ArcActor->SetProperty(this->ActiveProperty);
    this->TextActor->SetTextProperty(this->ActiveTextProperty);
    }
  else if (!this->GetNthNodeSelected(0) ||
           (this->GetNumberOfNodes() > 1 && !this->GetNthNodeSelected(1)) ||
           (this->GetNumberOfNodes() > 2 && !this->GetNthNodeSelected(2)))
    {
    this->LineActor->SetProperty(this->Property);
    this->ArcActor->SetProperty(this->Property);
    this->TextActor->SetTextProperty(this->TextProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->SelectedProperty);
    this->ArcActor->SetProperty(this->SelectedProperty);
    this->TextActor->SetTextProperty(this->SelectedTextProperty);
    }
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerAngleRepresentation3D::GetWidgetRepresentationAsPolyData()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput();
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::GetActors(vtkPropCollection *pc)
{
  this->Superclass::GetActors(pc);
  this->LineActor->GetActors(pc);
  this->ArcActor->GetActors(pc);
  this->TextActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->LineActor->ReleaseGraphicsResources(win);
  this->ArcActor->ReleaseGraphicsResources(win);
  this->TextActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerAngleRepresentation3D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  count = this->Superclass::RenderOverlay(viewport);
  if (this->LineActor->GetVisibility())
    {
    count +=  this->LineActor->RenderOverlay(viewport);
    }
  if (this->ArcActor->GetVisibility())
    {
    count +=  this->ArcActor->RenderOverlay(viewport);
    }
  if (this->TextActor->GetVisibility())
    {
    count +=  this->TextActor->RenderOverlay(viewport);
    }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerAngleRepresentation3D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count=0;
  count = this->Superclass::RenderOpaqueGeometry(viewport);
  if (this->LineActor->GetVisibility())
    {
    count += this->LineActor->RenderOpaqueGeometry(viewport);
    }
  if (this->ArcActor->GetVisibility())
    {
    count += this->ArcActor->RenderOpaqueGeometry(viewport);
    }
  if (this->TextActor->GetVisibility())
    {
    count += this->TextActor->RenderOpaqueGeometry(viewport);
    }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerAngleRepresentation3D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  count = this->Superclass::RenderTranslucentPolygonalGeometry(viewport);
  if (this->LineActor->GetVisibility())
    {
    count += this->LineActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  if (this->ArcActor->GetVisibility())
    {
    count += this->ArcActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  if (this->TextActor->GetVisibility())
    {
    count += this->TextActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerAngleRepresentation3D::HasTranslucentPolygonalGeometry()
{
  int result=0;
  result |= this->Superclass::HasTranslucentPolygonalGeometry();
  if (this->LineActor->GetVisibility())
    {
    result |= this->LineActor->HasTranslucentPolygonalGeometry();
    }
  if (this->ArcActor->GetVisibility())
    {
    result |= this->ArcActor->HasTranslucentPolygonalGeometry();
    }
  if (this->TextActor->GetVisibility())
    {
    result |= this->TextActor->HasTranslucentPolygonalGeometry();
    }
  return result;
}

//----------------------------------------------------------------------
double *vtkSlicerAngleRepresentation3D::GetBounds()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput()->GetPoints() ?
              this->appendActors->GetOutput()->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
int vtkSlicerAngleRepresentation3D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
{
  if (!this->MarkupsNode || this->MarkupsNode->GetLocked())
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
    return this->InteractionState;
    }

  this->MarkupsNode->DisableModifiedEventOn();
  if (this->Superclass::Superclass::ActivateNode(X, Y))
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::OnControlPoint;
    }
  else if (this->GetAssemblyPath(X, Y, 0, this->LinePicker)) //few flickeris, seems independent from the number of objects
  //else if (this->LinePicker->Pick(X, Y, 0, this->Renderer)) //slighty better perfomances, but flickery largly increase with the number of widgets
    {
    this->SetActiveNode(-2);
    this->InteractionState = vtkSlicerAbstractRepresentation::OnLine;
    }
  else
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
    }
  this->MarkupsNode->DisableModifiedEventOff();

  this->NeedToRenderOn();
  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
    {
    return;
    }
  pm->AddPicker(this->LinePicker, this);
}

//-----------------------------------------------------------------------------
void vtkSlicerAngleRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->LineActor)
    {
    os << indent << "Line Visibility: " << this->LineActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Line Visibility: (none)\n";
    }

  if (this->ArcActor)
    {
    os << indent << "Arc Visibility: " << this->ArcActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Arc Visibility: (none)\n";
    }

  if (this->TextActor)
    {
    os << indent << "Text Visibility: " << this->TextActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Text Visibility: (none)\n";
    }

  os << indent << "Label Format: ";
  if ( this->LabelFormat )
    {
    os << this->LabelFormat << "\n";
    }
  else
    {
    os << "(none)\n";
    }
}
