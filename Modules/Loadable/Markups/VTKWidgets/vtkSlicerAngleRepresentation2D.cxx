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

#include "vtkSlicerAngleRepresentation2D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkGlyph2D.h"
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
#include "vtkLinearSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkPropPicker.h"
#include "vtkAppendPolyData.h"
#include "vtkTubeFilter.h"
#include "vtkStringArray.h"
#include "vtkPickingManager.h"
#include "vtkVectorText.h"
#include "vtkOpenGLTextActor.h"
#include "vtkArcSource.h"
#include "cmath"
#include "vtkMRMLMarkupsDisplayNode.h"

vtkStandardNewMacro(vtkSlicerAngleRepresentation2D);

//----------------------------------------------------------------------
vtkSlicerAngleRepresentation2D::vtkSlicerAngleRepresentation2D()
{
  this->LineInterpolator = vtkLinearSlicerLineInterpolator::New();

  this->Line = vtkPolyData::New();
  this->Arc = vtkArcSource::New();
  this->Arc->SetResolution(30);

  this->TubeFilter = vtkTubeFilter::New();
  this->TubeFilter->SetInputData(Line);
  this->TubeFilter->SetNumberOfSides(20);
  this->TubeFilter->SetRadius(1);

  this->ArcTubeFilter = vtkTubeFilter::New();
  this->ArcTubeFilter->SetInputConnection(this->Arc->GetOutputPort());
  this->ArcTubeFilter->SetNumberOfSides(20);
  this->ArcTubeFilter->SetRadius(1);

  this->LineMapper = vtkOpenGLPolyDataMapper2D::New();
  this->LineMapper->SetInputConnection(this->TubeFilter->GetOutputPort());

  this->ArcMapper = vtkOpenGLPolyDataMapper2D::New();
  this->ArcMapper->SetInputConnection(this->ArcTubeFilter->GetOutputPort());

  this->LineActor = vtkActor2D::New();
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->SetProperty(this->Property);

  this->ArcActor = vtkActor2D::New();
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
}

//----------------------------------------------------------------------
vtkSlicerAngleRepresentation2D::~vtkSlicerAngleRepresentation2D()
{
  this->LineInterpolator->Delete();

  this->Line->Delete();
  this->Arc->Delete();
  this->LineMapper->Delete();
  this->ArcMapper->Delete();
  this->LineActor->Delete();
  this->ArcActor->Delete();
  this->LinePicker->Delete();
  this->TubeFilter->Delete();
  this->ArcTubeFilter->Delete();

  this->TextActor->Delete();
  delete [] this->LabelFormat;
  this->LabelFormat = nullptr;
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation2D::TranslateWidget(double eventPos[2])
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
void vtkSlicerAngleRepresentation2D::ScaleWidget(double eventPos[2])
{
  if (!this->MarkupsNode || this->GetNumberOfNodes() < 3)
    {
    return;
    }

  double ref[3] = {0.};
  double slicePos[2] = {0.};
  double worldPos[3] = {0.};

  slicePos[0] = this->LastEventPosition[0];
  slicePos[1] = this->LastEventPosition[1];

  this->GetSliceToWorldCoordinates(slicePos, ref);

  slicePos[0] = eventPos[0];
  slicePos[1] = eventPos[1];

  double centralPointWorldPos[3];
  this->GetNthNodeWorldPosition(1, centralPointWorldPos);

  double r2 = vtkMath::Distance2BetweenPoints(ref, centralPointWorldPos);

  this->GetSliceToWorldCoordinates(slicePos, worldPos);

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
void vtkSlicerAngleRepresentation2D::RotateWidget(double eventPos[2])
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
  double slicePos[2] = {0.};
  double worldPos[3] = {0.};
  double lastWorldPos[3] = {0.};

  slicePos[0] = this->LastEventPosition[0];
  slicePos[1] = this->LastEventPosition[1];

  this->GetSliceToWorldCoordinates(slicePos, lastWorldPos);

  slicePos[0] = eventPos[0];
  slicePos[1] = eventPos[1];

  double centralPointWorldPos[3];
  this->GetNthNodeWorldPosition(1, centralPointWorldPos);

  this->GetSliceToWorldCoordinates(slicePos, worldPos);

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
void vtkSlicerAngleRepresentation2D::BuildLines()
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> line;

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

    double pos[3] = { 0.0 };
    for (i = 0; i < numberOfNodes; i++)
      {
      // Add the node
      this->GetNthNodeDisplayPosition(i, pos);
      points->InsertPoint(index, pos);
      lineIndices[index] = index;
      index++;

      int numIntermediatePoints = this->GetNumberOfIntermediatePoints(i);

      for (j = 0; j < numIntermediatePoints; j++)
        {
        this->GetIntermediatePointDisplayPosition(i, j, pos);
        points->InsertPoint(index, pos);
        lineIndices[index] = index;
        index++;
        }
      }

    line->InsertNextCell(numLine, lineIndices);
    delete [] lineIndices;
    }

  this->Line->SetPoints(points);
  this->Line->SetLines(line);

  // Build Arc
  if (this->GetNumberOfNodes() != 3)
    {
    return;
    }

  double p1[2], p2[2], c[2], vector2[3], vector1[3];
  double l1 = 0.0, l2 = 0.0;
  this->GetNthNodeDisplayPosition(0, p1);
  this->GetNthNodeDisplayPosition(1, c);
  this->GetNthNodeDisplayPosition(2, p2);

  // Compute the angle (only if necessary since we don't want
  // fluctuations in angle value as the camera moves, etc.)
  if (fabs(p1[0]-c[0]) < 0.001 || fabs(p2[0]-c[0]) < 0.001)
    {
    return;
    }

  vector1[0] = p1[0] - c[0];
  vector1[1] = p1[1] - c[1];
  vector1[2] = 0.;
  vector2[0] = p2[0] - c[0];
  vector2[1] = p2[1] - c[1];
  vector2[2] = 0.;
  l1 = vtkMath::Normalize(vector1);
  l2 = vtkMath::Normalize(vector2);
  double angle = acos(vtkMath::Dot(vector1, vector2));

  // Place the label and place the arc
  const double length = l1 < l2 ? l1 : l2;
  const double anglePlacementRatio = 0.5;
  const double l = length * anglePlacementRatio;
  double arcp1[3] = {l * vector1[0] + c[0],
                     l * vector1[1] + c[1],
                     0.};
  double arcp2[3] = {l * vector2[0] + c[0],
                     l * vector2[1] + c[1],
                     0.};
  double arcc[3] = {c[0], c[1], 0.};

  this->Arc->SetPoint1(arcp1);
  this->Arc->SetPoint2(arcp2);
  this->Arc->SetCenter(arcc);
  this->Arc->Update();

  char string[512];
  snprintf(string, sizeof(string), this->LabelFormat, vtkMath::DegreesFromRadians(angle));
  this->TextActor->SetInput( string );

  double textPosDisplay[2], vector3[3];
  vector3[0] = vector1[0] + vector2[0];
  vector3[1] = vector1[1] + vector2[1];
  vector3[2] = vector1[2] + vector2[2];
  vtkMath::Normalize(vector3);
  textPosDisplay[0] = c[0] + vector3[0] * length * 0.6;
  textPosDisplay[1] = c[1] + vector3[1] * length * 0.6;

  int X = static_cast<int>(textPosDisplay[0]);
  int Y = static_cast<int>(textPosDisplay[1]);
  this->TextActor->SetDisplayPosition(X,Y);
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation2D::BuildRepresentation()
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

  double scale = this->CalculateViewScaleFactor();
  this->Glypher->SetScaleFactor(scale * this->HandleSize);
  this->SelectedGlypher->SetScaleFactor(scale * this->HandleSize);
  this->ActiveGlypher->SetScaleFactor(scale * this->HandleSize);
  this->TubeFilter->SetRadius(scale * this->HandleSize * 0.125);
  this->ArcTubeFilter->SetRadius(scale * this->HandleSize * 0.125);
  this->BuildRepresentationPointsAndLabels(scale * this->HandleSize);

  bool lineVisibility = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (!this->pointsVisibilityOnSlice->GetValue(ii) ||
        !this->GetNthNodeVisibility(ii))
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
int vtkSlicerAngleRepresentation2D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
{
  if (!this->MarkupsNode || this->MarkupsNode->GetLocked())
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
    return this->InteractionState;
    }

  int oldActiveNode = this->GetActiveNode();

  this->MarkupsNode->DisableModifiedEventOn();
  if (this->ActivateNode(X, Y))
    {
    if (this->pointsVisibilityOnSlice->GetValue(this->GetActiveNode()))
      {
      this->InteractionState = vtkSlicerAbstractRepresentation::OnControlPoint;
      }
    else
      {
      this->SetActiveNode(-1);
      this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
      }
    }
  else if (this->GetAssemblyPath(X, Y, 0, this->LinePicker)) // poor perfomances when widgets > 5
  //else if (this->LinePicker->Pick(X, Y, 0, this->Renderer)) // produce many rendering flickering when < 10
    {
    this->SetActiveNode(-2);
    this->InteractionState = vtkSlicerAbstractRepresentation::OnLine;
    }
  else
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
    }
  this->MarkupsNode->DisableModifiedEventOff();

  if (oldActiveNode != this->GetActiveNode())
    {
    this->MarkupsNode->Modified();
    }

  // This additional render is need only because of the flickering bug due to the vtkPropPicker
  // remove once it is fixed
  this->NeedToRenderOn();
  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation2D::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
    {
    return;
    }
  pm->AddPicker(this->LinePicker, this);
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation2D::GetActors(vtkPropCollection *pc)
{
  this->LineActor->GetActors(pc);
  this->ArcActor->GetActors(pc);
  this->TextActor->GetActors(pc);
  this->Superclass::GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation2D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->LineActor->ReleaseGraphicsResources(win);
  this->ArcActor->ReleaseGraphicsResources(win);
  this->TextActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerAngleRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
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
  count += this->Superclass::RenderOverlay(viewport);

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerAngleRepresentation2D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count=0;
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
  count += this->Superclass::RenderOpaqueGeometry(viewport);

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerAngleRepresentation2D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
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
  count += this->Superclass::RenderTranslucentPolygonalGeometry(viewport);

  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerAngleRepresentation2D::HasTranslucentPolygonalGeometry()
{
  int result=0;
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
  result |= this->Superclass::HasTranslucentPolygonalGeometry();

  return result;
}

//----------------------------------------------------------------------
double *vtkSlicerAngleRepresentation2D::GetBounds()
{
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkSlicerAngleRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->LineActor)
    {
    os << indent << "Line Actor Visibility: " << this->LineActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Line Actor: (none)\n";
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
