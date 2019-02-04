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

#include "vtkSlicerCurveRepresentation2D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkAssemblyPath.h"
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
#include "vtkBezierSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkPropPicker.h"
#include "vtkAppendPolyData.h"
#include "vtkTubeFilter.h"
#include "vtkStringArray.h"
#include "vtkPickingManager.h"
#include "vtkVectorText.h"
#include "vtkOpenGLTextActor.h"
#include "cmath"
#include "vtkMRMLMarkupsDisplayNode.h"

vtkStandardNewMacro(vtkSlicerCurveRepresentation2D);

//----------------------------------------------------------------------
vtkSlicerCurveRepresentation2D::vtkSlicerCurveRepresentation2D()
{
  this->LineInterpolator = vtkBezierSlicerLineInterpolator::New();

  this->Line = vtkPolyData::New();
  this->TubeFilter = vtkTubeFilter::New();
  this->TubeFilter->SetInputData(this->Line);
  this->TubeFilter->SetNumberOfSides(20);
  this->TubeFilter->SetRadius(1);

  this->LineMapper = vtkOpenGLPolyDataMapper2D::New();
  this->LineMapper->SetInputConnection(this->TubeFilter->GetOutputPort());

  this->LineActor = vtkActor2D::New();
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->SetProperty(this->Property);

  //Manage the picking
  this->LinePicker = vtkPropPicker::New();
  this->LinePicker->PickFromListOn();
  this->LinePicker->InitializePickList();
  this->LinePicker->AddPickList(this->LineActor);

  this->appendActors = vtkAppendPolyData::New();
  this->appendActors->AddInputData(this->CursorShape);
  this->appendActors->AddInputData(this->SelectedCursorShape);
  this->appendActors->AddInputData(this->ActiveCursorShape);
  this->appendActors->AddInputData(this->TubeFilter->GetOutput());
}

//----------------------------------------------------------------------
vtkSlicerCurveRepresentation2D::~vtkSlicerCurveRepresentation2D()
{
  this->LineInterpolator->Delete();

  this->Line->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();
  this->LinePicker->Delete();
  this->TubeFilter->Delete();
  this->appendActors->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::TranslateNode(double eventPos[2])
{
  this->Superclass::TranslateNode(eventPos);

  if (this->ClosedLoop)
    {
    this->UpdateCentroid();
    }
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::TranslateWidget(double eventPos[2])
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

  if (this->ClosedLoop)
    {
    this->UpdateCentroid();
    }
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::ScaleWidget(double eventPos[2])
{
  if (this->GetActiveNode() == -3)
    {
    return;
    }

  // If any node is locked return
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (this->GetNthNodeLocked(i))
      {
      return;
      }
    }

  this->Superclass::ScaleWidget(eventPos);
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::RotateWidget(double eventPos[2])
{
  if (this->GetActiveNode() == -3)
    {
    return;
    }

  // If any node is locked return
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (this->GetNthNodeLocked(i))
      {
      return;
      }
    }

  this->Superclass::RotateWidget(eventPos);
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::BuildLines()
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

  if (this->ClosedLoop)
    {
    count++;
    }

  points->SetNumberOfPoints(count);
  vtkIdType numLine = count;
  if (numLine > 0)
    {
    vtkIdType *lineIndices = new vtkIdType[numLine];

    double pos[2];
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

    if (this->ClosedLoop)
      {
      this->GetNthNodeWorldPosition(0, pos);
      points->InsertPoint(index, pos);
      lineIndices[index] = 0;
      }

    line->InsertNextCell(numLine, lineIndices);
    delete [] lineIndices;
    }

  this->Line->SetPoints(points);
  this->Line->SetLines(line);
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::BuildRepresentation()
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
    }
  else
    {
    LabelsActor->VisibilityOff();
    SelectedLabelsActor->VisibilityOff();
    ActiveLabelsActor->VisibilityOff();
    }

  double scale = this->CalculateViewScaleFactor();
  this->Glypher->SetScaleFactor(scale * this->HandleSize);
  this->SelectedGlypher->SetScaleFactor(scale * this->HandleSize);
  this->ActiveGlypher->SetScaleFactor(scale * this->HandleSize);
  this->TubeFilter->SetRadius(scale * this->HandleSize * 0.125);
  this->BuildRepresentationPointsAndLabels(scale * this->HandleSize);

  bool allNodeVisibile = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (!this->pointsVisibilityOnSlice->GetValue(ii) ||
        !this->GetNthNodeVisibility(ii))
      {
      allNodeVisibile = false;
      break;
      }
    }

  this->LineActor->SetVisibility(allNodeVisibile);

  bool allNodeSelected = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (!this->GetNthNodeSelected(ii))
      {
      allNodeSelected = false;
      break;
      }
    }

  if (this->GetActiveNode() == -2)
    {
    this->LineActor->SetProperty(this->ActiveProperty);
    }
  else if (allNodeSelected)
    {
    this->LineActor->SetProperty(this->SelectedProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->Property);
    }

  bool allNodeNotVisibile = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (this->pointsVisibilityOnSlice->GetValue(ii) &&
        this->GetNthNodeVisibility(ii))
      {
      allNodeNotVisibile = false;
      break;
      }
    }

  if (this->ClosedLoop && this->GetNumberOfNodes() > 2 &&
      this->GetActiveNode() != -3 && !allNodeNotVisibile && this->centroidVisibilityOnSlice)
    {
    double centroidPosWorld[3], centroidPosDisplay[3], orient[3] = {0};
    this->MarkupsNode->GetCentroidPosition(centroidPosWorld);
    this->GetWorldToSliceCoordinates(centroidPosWorld, centroidPosDisplay);

    if (allNodeSelected)
      {

      this->SelectedFocalPoint->InsertNextPoint(centroidPosDisplay);
      this->SelectedFocalData->GetPointData()->GetNormals()->InsertNextTuple(orient);

      this->SelectedFocalPoint->Modified();
      this->SelectedFocalData->GetPointData()->GetNormals()->Modified();
      this->SelectedFocalData->Modified();
      }
    else
      {
      this->FocalPoint->InsertNextPoint(centroidPosDisplay);
      this->FocalData->GetPointData()->GetNormals()->InsertNextTuple(orient);

      this->FocalPoint->Modified();
      this->FocalData->GetPointData()->GetNormals()->Modified();
      this->FocalData->Modified();
      }
    }
  else if (this->ClosedLoop && this->GetActiveNode() == -3 &&
           !allNodeNotVisibile && this->centroidVisibilityOnSlice)
    {
    double centroidPosWorld[3], centroidPosDisplay[3], orient[3] = {0};
    this->MarkupsNode->GetCentroidPosition(centroidPosWorld);
    this->GetWorldToSliceCoordinates(centroidPosWorld, centroidPosDisplay);

    this->ActiveFocalPoint->SetPoint(0, centroidPosDisplay);
    this->ActiveFocalData->GetPointData()->GetNormals()->SetTuple(0, orient);

    this->ActiveFocalPoint->Modified();
    this->ActiveFocalData->GetPointData()->GetNormals()->Modified();
    this->ActiveFocalData->Modified();

    this->ActiveActor->VisibilityOn();
    this->ActiveLabelsActor->VisibilityOff();
    }
}

//----------------------------------------------------------------------
int vtkSlicerCurveRepresentation2D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
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
void vtkSlicerCurveRepresentation2D::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
    {
    return;
    }
  pm->AddPicker(this->LinePicker, this);
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerCurveRepresentation2D::GetWidgetRepresentationAsPolyData()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput();
}


//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::GetActors(vtkPropCollection *pc)
{
  this->LineActor->GetActors(pc);
  this->Superclass::GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->LineActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerCurveRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  if (this->LineActor->GetVisibility())
    {
    count +=  this->LineActor->RenderOverlay(viewport);
    }
  count += this->Superclass::RenderOverlay(viewport);

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerCurveRepresentation2D::RenderOpaqueGeometry(
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
  count += this->Superclass::RenderOpaqueGeometry(viewport);

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerCurveRepresentation2D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  if (this->LineActor->GetVisibility())
    {
    count += this->LineActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  count += this->Superclass::RenderTranslucentPolygonalGeometry(viewport);

  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerCurveRepresentation2D::HasTranslucentPolygonalGeometry()
{
  int result=0;
  if (this->LineActor->GetVisibility())
    {
    result |= this->LineActor->HasTranslucentPolygonalGeometry();
    }
  result |= this->Superclass::HasTranslucentPolygonalGeometry();

  return result;
}

//----------------------------------------------------------------------
double *vtkSlicerCurveRepresentation2D::GetBounds()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput()->GetPoints() ?
              this->appendActors->GetOutput()->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
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
}
