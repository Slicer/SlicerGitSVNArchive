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

#include "vtkSlicerLineRepresentation2D.h"
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
#include "vtkMRMLMarkupsDisplayNode.h"

vtkStandardNewMacro(vtkSlicerLineRepresentation2D);

//----------------------------------------------------------------------
vtkSlicerLineRepresentation2D::vtkSlicerLineRepresentation2D()
{
  this->Line = vtkPolyData::New();
  this->TubeFilter = vtkTubeFilter::New();
  this->TubeFilter->SetInputData(Line);
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
  this->appendActors->AddInputData(this->Line);
}

//----------------------------------------------------------------------
vtkSlicerLineRepresentation2D::~vtkSlicerLineRepresentation2D()
{
  this->Line->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();
  this->LinePicker->Delete();
  this->TubeFilter->Delete();
  this->appendActors->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::TranslateWidget(double eventPos[2])
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
void vtkSlicerLineRepresentation2D::ScaleWidget(double eventPos[2])
{
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
void vtkSlicerLineRepresentation2D::RotateWidget(double eventPos[2])
{
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
void vtkSlicerLineRepresentation2D::BuildRepresentation()
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

  int numPoints = this->GetNumberOfNodes();
  int ii;

  this->FocalPoint->SetNumberOfPoints(0);
  this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(0);

  this->LabelsFocalPoint->SetNumberOfPoints(0);
  this->LabelsFocalData->GetPointData()->GetNormals()->SetNumberOfTuples(0);

  this->Labels->SetNumberOfValues(0);
  this->LabelsPriority->SetNumberOfValues(0);

  for (ii = 0; ii < numPoints; ii++)
    {
    if (ii == this->GetActiveNode() ||
        !this->pointsVisibilityOnSlice->GetValue(ii) ||
        !this->GetNthNodeVisibility(ii) ||
        this->GetNthNodeSelected(ii))
      {
      continue;
      }

    double slicePos[2] = {0}, worldOrient[9] = {0}, orientation[4] = {0};
    this->GetNthNodeDisplayPosition(ii, slicePos);
    bool skipPoint = false;
    for (int jj = 0; jj < this->FocalPoint->GetNumberOfPoints(); jj++)
      {
      double* pos = this->FocalPoint->GetPoint(jj);
      double eps = 0.001;
      if (fabs(pos[0] - slicePos[0]) < eps &&
           fabs(pos[1] - slicePos[1]) < eps)
        {
        skipPoint = true;
        break;
        }
      }

    if (skipPoint)
      {
      continue;
      }

    this->Renderer->SetDisplayPoint(slicePos);
    this->Renderer->DisplayToView();
    double viewPos[3];
    this->Renderer->GetViewPoint(viewPos);
    this->Renderer->ViewToNormalizedViewport(viewPos[0], viewPos[1], viewPos[2]);

    this->FocalPoint->InsertNextPoint(slicePos);
    this->LabelsFocalPoint->InsertNextPoint(viewPos);

    this->GetNthNodeOrientation(ii, orientation);
    this->FromOrientationQuaternionToWorldOrient(orientation, worldOrient);
    this->FocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);
    this->LabelsFocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);

    this->Labels->InsertNextValue(this->GetNthNodeLabel(ii));
    this->LabelsPriority->InsertNextValue(std::to_string (ii));
    }

  this->FocalPoint->Modified();
  this->FocalData->GetPointData()->GetNormals()->Modified();
  this->FocalData->Modified();

  this->LabelsFocalPoint->Modified();
  this->LabelsFocalData->GetPointData()->GetNormals()->Modified();
  this->LabelsFocalData->Modified();

  this->SelectedFocalPoint->SetNumberOfPoints(0);
  this->SelectedFocalData->GetPointData()->GetNormals()->SetNumberOfTuples(0);

  this->SelectedLabelsFocalPoint->SetNumberOfPoints(0);
  this->SelectedLabelsFocalData->GetPointData()->GetNormals()->SetNumberOfTuples(0);

  this->SelectedLabels->SetNumberOfValues(0);
  this->SelectedLabelsPriority->SetNumberOfValues(0);

  for (ii = 0; ii < numPoints; ii++)
    {
    if (ii == this->GetActiveNode() ||
        !this->pointsVisibilityOnSlice->GetValue(ii) ||
        !this->GetNthNodeVisibility(ii) ||
        !this->GetNthNodeSelected(ii))
      {
      continue;
      }

    double slicePos[3] = {0}, worldOrient[9] = {0}, orientation[4] = {0};
    this->GetNthNodeDisplayPosition(ii, slicePos);
    bool skipPoint = false;
    for (int jj = 0; jj < this->SelectedFocalPoint->GetNumberOfPoints(); jj++)
      {
      double* pos = this->SelectedFocalPoint->GetPoint(jj);
      double eps = 0.001;
      if (fabs(pos[0] - slicePos[0]) < eps &&
           fabs(pos[1] - slicePos[1]) < eps)
        {
        skipPoint = true;
        break;
        }
      }

    if (skipPoint)
      {
      continue;
      }

    this->Renderer->SetDisplayPoint(slicePos);
    this->Renderer->DisplayToView();
    double viewPos[3];
    this->Renderer->GetViewPoint(viewPos);
    this->Renderer->ViewToNormalizedViewport(viewPos[0], viewPos[1], viewPos[2]);

    this->SelectedFocalPoint->InsertNextPoint(slicePos);
    this->SelectedLabelsFocalPoint->InsertNextPoint(viewPos);

    this->GetNthNodeOrientation(ii, orientation);
    this->FromOrientationQuaternionToWorldOrient(orientation, worldOrient);
    this->SelectedFocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);
    this->SelectedLabelsFocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);

    this->SelectedLabels->InsertNextValue(this->GetNthNodeLabel(ii));
    this->SelectedLabelsPriority->InsertNextValue(std::to_string(ii));
    }

  this->SelectedFocalPoint->Modified();
  this->SelectedFocalData->GetPointData()->GetNormals()->Modified();
  this->SelectedFocalData->Modified();

  this->SelectedLabelsFocalPoint->Modified();
  this->SelectedLabelsFocalData->GetPointData()->GetNormals()->Modified();
  this->SelectedLabelsFocalData->Modified();

  if (this->GetActiveNode() >= 0 &&
      this->GetActiveNode() < this->GetNumberOfNodes() &&
      this->GetNthNodeVisibility(this->GetActiveNode()) &&
      this->pointsVisibilityOnSlice->GetValue(this->GetActiveNode()))
    {
    double slicePos[2] = {0}, worldOrient[9] = {0}, orientation[4] = {0};

    this->GetNthNodeDisplayPosition(this->GetActiveNode(), slicePos);
    this->ActiveFocalPoint->SetPoint(0, slicePos);

    this->Renderer->SetDisplayPoint(slicePos);
    this->Renderer->DisplayToView();
    double viewPos[3];
    this->Renderer->GetViewPoint(viewPos);
    this->Renderer->ViewToNormalizedViewport(viewPos[0], viewPos[1], viewPos[2]);
    this->ActiveLabelsFocalPoint->SetPoint(0, viewPos);

    this->GetNthNodeOrientation(0, orientation);
    this->FromOrientationQuaternionToWorldOrient(orientation, worldOrient);
    this->ActiveFocalData->GetPointData()->GetNormals()->SetTuple(0, worldOrient + 6);
    this->ActiveLabelsFocalData->GetPointData()->GetNormals()->SetTuple(0, worldOrient + 6);

    this->ActiveFocalPoint->Modified();
    this->ActiveFocalData->GetPointData()->GetNormals()->Modified();
    this->ActiveFocalData->Modified();

    this->ActiveLabelsFocalPoint->Modified();
    this->ActiveLabelsFocalData->GetPointData()->GetNormals()->Modified();
    this->ActiveLabelsFocalData->Modified();

    this->ActiveLabels->SetValue(0, this->GetActiveNodeLabel());
    this->ActiveLabelsPriority->SetValue(0, std::to_string(this->GetActiveNode()));

    this->ActiveActor->VisibilityOn();
    this->ActiveLabelsActor->VisibilityOn();
    }
  else
    {
    this->ActiveActor->VisibilityOff();
    this->ActiveLabelsActor->VisibilityOff();
    }

  bool lineVisibility = true;
  for (ii = 0; ii < numPoints; ii++)
    {
    if (!this->pointsVisibilityOnSlice->GetValue(ii) ||
        !this->GetNthNodeVisibility(ii))
      {
      lineVisibility = false;
      break;
      }
    }

  this->LineActor->SetVisibility(lineVisibility);

  if (this->GetActiveNode() == -2)
    {
    this->LineActor->SetProperty(this->ActiveProperty);
    }
  else if (!this->GetNthNodeSelected(0) || !this->GetNthNodeSelected(1))
    {
    this->LineActor->SetProperty(this->Property);
    }
  else
    {
    this->LineActor->SetProperty(this->SelectedProperty);
    }
}

//----------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
{
  if (!this->MarkupsNode || this->MarkupsNode->GetLocked())
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
    return this->InteractionState;
    }

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
  //else if (this->GetAssemblyPath(X, Y, 0, this->LinePicker)) // poor perfomances when widgets > 5
  else if (this->LinePicker->Pick(X, Y, 0, this->Renderer)) // produce many rendering flickering when < 10
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
void vtkSlicerLineRepresentation2D::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
    {
    return;
    }
  pm->AddPicker(this->LinePicker, this);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::BuildLines()
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

    Line->InsertNextCell(numLine, lineIndices);
    delete [] lineIndices;
    }

  this->Line->SetPoints(points);
  this->Line->SetLines(Line);
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerLineRepresentation2D::GetWidgetRepresentationAsPolyData()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput();
}


//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::GetActors(vtkPropCollection *pc)
{
  this->Superclass::GetActors(pc);
  this->LineActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->LineActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  count = this->Superclass::RenderOverlay(viewport);
  if (this->LineActor->GetVisibility())
    {
    count +=  this->LineActor->RenderOverlay(viewport);
    }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::RenderOpaqueGeometry(
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
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  count = this->Superclass::RenderTranslucentPolygonalGeometry(viewport);
  if (this->LineActor->GetVisibility())
    {
    count += this->LineActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerLineRepresentation2D::HasTranslucentPolygonalGeometry()
{
  int result=0;
  result |= this->Superclass::HasTranslucentPolygonalGeometry();
  if (this->LineActor->GetVisibility())
    {
    result |= this->LineActor->HasTranslucentPolygonalGeometry();
    }
  return result;
}

//----------------------------------------------------------------------
double *vtkSlicerLineRepresentation2D::GetBounds()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput()->GetPoints() ?
              this->appendActors->GetOutput()->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
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
