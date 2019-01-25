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

#include "vtkSlicerLineRepresentation3D.h"
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
#include "vtkMRMLMarkupsDisplayNode.h"

vtkStandardNewMacro(vtkSlicerLineRepresentation3D);

//----------------------------------------------------------------------
vtkSlicerLineRepresentation3D::vtkSlicerLineRepresentation3D()
{
  this->Line = vtkPolyData::New();
  this->TubeFilter = vtkTubeFilter::New();
  this->TubeFilter->SetInputData(Line);
  this->TubeFilter->SetNumberOfSides(20);
  this->TubeFilter->SetRadius(1);

  this->LineMapper = vtkOpenGLPolyDataMapper::New();
  this->LineMapper->SetInputConnection(this->TubeFilter->GetOutputPort());
  this->LineMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->LineMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
  this->LineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
  this->LineMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->LineActor = vtkOpenGLActor::New();
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
vtkSlicerLineRepresentation3D::~vtkSlicerLineRepresentation3D()
{
  this->Line->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();
  this->LinePicker->Delete();
  this->TubeFilter->Delete();
  this->appendActors->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::TranslateWidget(double eventPos[2])
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
void vtkSlicerLineRepresentation3D::ScaleWidget(double eventPos[2])
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
void vtkSlicerLineRepresentation3D::RotateWidget(double eventPos[2])
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
void vtkSlicerLineRepresentation3D::BuildLines()
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
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerLineRepresentation3D::GetWidgetRepresentationAsPolyData()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput();
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::GetActors(vtkPropCollection *pc)
{
  this->Superclass::GetActors(pc);
  this->LineActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->LineActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerLineRepresentation3D::RenderOverlay(vtkViewport *viewport)
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
int vtkSlicerLineRepresentation3D::RenderOpaqueGeometry(
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
int vtkSlicerLineRepresentation3D::RenderTranslucentPolygonalGeometry(
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
vtkTypeBool vtkSlicerLineRepresentation3D::HasTranslucentPolygonalGeometry()
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
double *vtkSlicerLineRepresentation3D::GetBounds()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput()->GetPoints() ?
              this->appendActors->GetOutput()->GetBounds() : nullptr;
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::BuildRepresentation()
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
    }

  this->Glypher->SetScaleFactor(this->HandleSize);
  this->SelectedGlypher->SetScaleFactor(this->HandleSize);
  this->ActiveGlypher->SetScaleFactor(this->HandleSize);
   this->TubeFilter->SetRadius(this->HandleSize * 0.125);

  int numPoints = this->GetNumberOfNodes();
  int ii;

  this->FocalPoint->SetNumberOfPoints(0);
  this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(0);
  this->Labels->SetNumberOfValues(0);
  this->LabelsPriority->SetNumberOfValues(0);

  for (ii = 0; ii < numPoints; ii++)
    {
    if (!this->GetNthNodeVisibility(ii) ||
         ii == this->GetActiveNode() ||
         this->GetNthNodeSelected(ii))
      {
      continue;
      }

    double worldPos[3], worldOrient[9] = {0}, orientation[4] = {0};
    this->GetNthNodeWorldPosition(ii, worldPos);
    bool skipPoint = false;
    for (int jj = 0; jj < this->FocalPoint->GetNumberOfPoints(); jj++)
      {
      double* pos = this->FocalPoint->GetPoint(jj);
      double eps = 0.001;
      if (fabs(pos[0] - worldPos[0]) < eps &&
           fabs(pos[1] - worldPos[1]) < eps &&
           fabs(pos[2] - worldPos[2]) < eps)
        {
        skipPoint = true;
        break;
        }
      }

    if (skipPoint)
      {
      continue;
      }

    this->FocalPoint->InsertNextPoint(worldPos);
    this->GetNthNodeOrientation(ii, orientation);
    this->FromOrientationQuaternionToWorldOrient(orientation, worldOrient);
    this->FocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);
    this->Labels->InsertNextValue(this->GetNthNodeLabel(ii));
    this->LabelsPriority->InsertNextValue(std::to_string (ii));
    }

  this->FocalPoint->Modified();
  this->FocalData->GetPointData()->GetNormals()->Modified();
  this->FocalData->Modified();

  this->SelectedFocalPoint->SetNumberOfPoints(0);
  this->SelectedFocalData->GetPointData()->GetNormals()->SetNumberOfTuples(0);
  this->SelectedLabels->SetNumberOfValues(0);
  this->SelectedLabelsPriority->SetNumberOfValues(0);

  for (ii = 0; ii < numPoints; ii++)
    {
    if (!this->GetNthNodeVisibility(ii) ||
         ii == this->GetActiveNode() ||
         !this->GetNthNodeSelected(ii))
      {
      continue;
      }

    double worldPos[3], worldOrient[9] = {0}, orientation[4] = {0};
    this->GetNthNodeWorldPosition(ii, worldPos);
    bool skipPoint = false;
    for (int jj = 0; jj < this->SelectedFocalPoint->GetNumberOfPoints(); jj++)
      {
      double* pos = this->SelectedFocalPoint->GetPoint(jj);
      double eps = 0.001;
      if (fabs(pos[0] - worldPos[0]) < eps &&
           fabs(pos[1] - worldPos[1]) < eps &&
           fabs(pos[2] - worldPos[2]) < eps)
        {
        skipPoint = true;
        break;
        }
      }

    if (skipPoint)
      {
      continue;
      }

    this->SelectedFocalPoint->InsertNextPoint(worldPos);
    this->GetNthNodeOrientation(ii, orientation);
    this->FromOrientationQuaternionToWorldOrient(orientation, worldOrient);
    this->SelectedFocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);
    this->SelectedLabels->InsertNextValue(this->GetNthNodeLabel(ii));
    this->SelectedLabelsPriority->InsertNextValue(std::to_string(ii));
    }

  this->SelectedFocalPoint->Modified();
  this->SelectedFocalData->GetPointData()->GetNormals()->Modified();
  this->SelectedFocalData->Modified();

  if (this->GetActiveNode() >= 0 &&
      this->GetActiveNode() < this->GetNumberOfNodes() &&
      this->GetNthNodeVisibility(this->GetActiveNode()))
    {
    double worldPos[3], worldOrient[9] = {0}, orientation[4] = {0};
    this->GetActiveNodeWorldPosition(worldPos);
    this->ActiveFocalPoint->SetPoint(0, worldPos);
    this->GetActiveNodeOrientation(orientation);
    this->FromOrientationQuaternionToWorldOrient(orientation, worldOrient);
    this->ActiveFocalData->GetPointData()->GetNormals()->SetTuple(0, worldOrient + 6);

    this->ActiveFocalPoint->Modified();
    this->ActiveFocalData->GetPointData()->GetNormals()->Modified();
    this->ActiveFocalData->Modified();

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
    if (!this->GetNthNodeVisibility(ii))
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
  else if (this->FocalPoint->GetNumberOfPoints() == numPoints)
    {
    this->LineActor->SetProperty(this->Property);
    }
  else
    {
    this->LineActor->SetProperty(this->SelectedProperty);
    }
}

//-----------------------------------------------------------------------------
int vtkSlicerLineRepresentation3D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
{
  if (!this->MarkupsNode || this->MarkupsNode->GetLocked())
    {
    // both points are not selected, do not perfom the picking and no active
    this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
    return this->InteractionState;
    }

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

  this->NeedToRenderOn();
  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
    {
    return;
    }
  pm->AddPicker(this->LinePicker, this);
}

//-----------------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
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
}
