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

#include "vtkSlicerAbstractRepresentation.h"
#include "vtkSlicerAbstractRepresentation2D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
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
#include "vtkSlicerLineInterpolator.h"
#include "vtkBezierSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkBox.h"
#include "vtkIntArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointPlacer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWindow.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkStringArray.h"
#include "vtkLabelHierarchy.h"
#include "vtkTextProperty.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkCoordinate.h"
#include <set>
#include <algorithm>
#include <iterator>

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation2D::vtkSlicerAbstractRepresentation2D()
{
  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Mapper = vtkOpenGLPolyDataMapper2D::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());
  this->Mapper->ScalarVisibilityOff();
  vtkNew<vtkCoordinate> coordinate;
  coordinate->SetViewport(this->Renderer);
  // World coordinates of the node can not be used to build the actors of the representation
  // Because the worls coorinate in the node are the 3D ones.
  coordinate->SetCoordinateSystemToDisplay();
  this->Mapper->SetTransformCoordinate(coordinate);

  this->SelectedMapper = vtkOpenGLPolyDataMapper2D::New();
  this->SelectedMapper->SetInputConnection(this->SelectedGlypher->GetOutputPort());
  this->SelectedMapper->ScalarVisibilityOff();
  this->SelectedMapper->SetTransformCoordinate(coordinate);

  this->ActiveMapper = vtkOpenGLPolyDataMapper2D::New();
  this->ActiveMapper->SetInputConnection(this->ActiveGlypher->GetOutputPort());
  this->ActiveMapper->ScalarVisibilityOff();
  this->ActiveMapper->SetTransformCoordinate(coordinate);

  this->Actor = vtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  this->SelectedActor = vtkActor2D::New();
  this->SelectedActor->SetMapper(this->SelectedMapper);
  this->SelectedActor->SetProperty(this->SelectedProperty);

  this->ActiveActor = vtkActor2D::New();
  this->ActiveActor->SetMapper(this->ActiveMapper);
  this->ActiveActor->SetProperty(this->ActiveProperty);

  this->pointsVisibilityOnSlice = vtkIntArray::New();
  pointsVisibilityOnSlice->SetName("pointsVisibilityOnSlice");
  pointsVisibilityOnSlice->SetNumberOfValues(100);
  pointsVisibilityOnSlice->SetNumberOfValues(1);
  pointsVisibilityOnSlice->SetValue(0,0);

  this->SliceNode = nullptr;

  // Labels
  this->LabelsFocalPoint = vtkPoints::New();
  this->LabelsFocalPoint->SetNumberOfPoints(100);
  this->LabelsFocalPoint->SetNumberOfPoints(1);
  this->LabelsFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *normals = vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(100);
  normals->SetNumberOfTuples(1);
  double n[3] = {0, 0, 0};
  normals->SetTuple(0, n);

  this->SelectedLabelsFocalPoint = vtkPoints::New();
  this->SelectedLabelsFocalPoint->SetNumberOfPoints(100);
  this->SelectedLabelsFocalPoint->SetNumberOfPoints(1);
  this->SelectedLabelsFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *selectedNormals = vtkDoubleArray::New();
  selectedNormals->SetNumberOfComponents(3);
  selectedNormals->SetNumberOfTuples(100);
  selectedNormals->SetNumberOfTuples(1);
  selectedNormals->SetTuple(0, n);

  this->ActiveLabelsFocalPoint = vtkPoints::New();
  this->ActiveLabelsFocalPoint->SetNumberOfPoints(100);
  this->ActiveLabelsFocalPoint->SetNumberOfPoints(1);
  this->ActiveLabelsFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *activeNormals = vtkDoubleArray::New();
  activeNormals->SetNumberOfComponents(3);
  activeNormals->SetNumberOfTuples(100);
  activeNormals->SetNumberOfTuples(1);
  activeNormals->SetTuple(0, n);

  this->LabelsFocalData = vtkPolyData::New();
  this->LabelsFocalData->SetPoints(this->LabelsFocalPoint);
  this->LabelsFocalData->GetPointData()->SetNormals(normals);
  normals->Delete();

  this->SelectedLabelsFocalData = vtkPolyData::New();
  this->SelectedLabelsFocalData->SetPoints(this->SelectedLabelsFocalPoint);
  this->SelectedLabelsFocalData->GetPointData()->SetNormals(selectedNormals);
  selectedNormals->Delete();

  this->ActiveLabelsFocalData = vtkPolyData::New();
  this->ActiveLabelsFocalData->SetPoints(this->ActiveLabelsFocalPoint);
  this->ActiveLabelsFocalData->GetPointData()->SetNormals(activeNormals);
  activeNormals->Delete();

  this->Labels = vtkStringArray::New();
  this->Labels->SetName("labels");
  this->Labels->SetNumberOfValues(100);
  this->Labels->SetNumberOfValues(1);
  this->Labels->SetValue(0, "F");
  this->LabelsPriority = vtkStringArray::New();
  this->LabelsPriority->SetName("priority");
  this->LabelsPriority->SetNumberOfValues(100);
  this->LabelsPriority->SetNumberOfValues(1);
  this->LabelsPriority->SetValue(0, "1");
  this->LabelsFocalData->GetPointData()->AddArray(this->Labels);
  this->LabelsFocalData->GetPointData()->AddArray(this->LabelsPriority);
  this->PointSetToLabelHierarchyFilter = vtkPointSetToLabelHierarchy::New();
  this->PointSetToLabelHierarchyFilter->SetTextProperty(this->TextProperty);
  this->PointSetToLabelHierarchyFilter->SetLabelArrayName("labels");
  this->PointSetToLabelHierarchyFilter->SetPriorityArrayName("priority");
  this->PointSetToLabelHierarchyFilter->SetInputData(this->LabelsFocalData);
  this->LabelsMapper = vtkLabelPlacementMapper::New();
  this->LabelsMapper->SetInputConnection(this->PointSetToLabelHierarchyFilter->GetOutputPort());
  // Here it will be the best to use Display coorinate system
  // in that way we would not need the addtional copy of the polydata in LabelFocalData (in Viewport coordinates)
  // However the LabelsMapper seems not working with the Display coordiantes.
  this->LabelsMapper->GetAnchorTransform()->SetCoordinateSystemToNormalizedViewport();
  this->LabelsActor = vtkActor2D::New();
  this->LabelsActor->SetMapper(this->LabelsMapper);

  this->SelectedLabels = vtkStringArray::New();
  this->SelectedLabels->SetName("labels");
  this->SelectedLabels->SetNumberOfValues(100);
  this->SelectedLabels->SetNumberOfValues(1);
  this->SelectedLabels->SetValue(0, "F");
  this->SelectedLabelsPriority = vtkStringArray::New();
  this->SelectedLabelsPriority->SetName("priority");
  this->SelectedLabelsPriority->SetNumberOfValues(100);
  this->SelectedLabelsPriority->SetNumberOfValues(1);
  this->SelectedLabelsPriority->SetValue(0, "1");
  this->SelectedLabelsFocalData->GetPointData()->AddArray(this->SelectedLabels);
  this->SelectedLabelsFocalData->GetPointData()->AddArray(this->SelectedLabelsPriority);
  this->SelectedPointSetToLabelHierarchyFilter = vtkPointSetToLabelHierarchy::New();
  this->SelectedPointSetToLabelHierarchyFilter->SetTextProperty(this->SelectedTextProperty);
  this->SelectedPointSetToLabelHierarchyFilter->SetLabelArrayName("labels");
  this->SelectedPointSetToLabelHierarchyFilter->SetPriorityArrayName("priority");
  this->SelectedPointSetToLabelHierarchyFilter->SetInputData(this->SelectedLabelsFocalData);
  this->SelectedLabelsMapper = vtkLabelPlacementMapper::New();
  this->SelectedLabelsMapper->SetInputConnection(this->SelectedPointSetToLabelHierarchyFilter->GetOutputPort());
  this->SelectedLabelsMapper->GetAnchorTransform()->SetCoordinateSystemToNormalizedViewport();
  this->SelectedLabelsActor = vtkActor2D::New();
  this->SelectedLabelsActor->SetMapper(this->SelectedLabelsMapper);

  this->ActiveLabels = vtkStringArray::New();
  this->ActiveLabels->SetName("labels");
  this->ActiveLabels->SetNumberOfValues(100);
  this->ActiveLabels->SetNumberOfValues(1);
  this->ActiveLabels->SetValue(0, "F");
  this->ActiveLabelsPriority = vtkStringArray::New();
  this->ActiveLabelsPriority->SetName("priority");
  this->ActiveLabelsPriority->SetNumberOfValues(100);
  this->ActiveLabelsPriority->SetNumberOfValues(1);
  this->ActiveLabelsPriority->SetValue(0, "1");
  this->ActiveLabelsFocalData->GetPointData()->AddArray(this->ActiveLabels);
  this->ActiveLabelsFocalData->GetPointData()->AddArray(this->ActiveLabelsPriority);
  this->ActivePointSetToLabelHierarchyFilter = vtkPointSetToLabelHierarchy::New();
  this->ActivePointSetToLabelHierarchyFilter->SetTextProperty(this->ActiveTextProperty);
  this->ActivePointSetToLabelHierarchyFilter->SetLabelArrayName("labels");
  this->ActivePointSetToLabelHierarchyFilter->SetPriorityArrayName("priority");
  this->ActivePointSetToLabelHierarchyFilter->SetInputData(this->ActiveLabelsFocalData);
  this->ActiveLabelsMapper = vtkLabelPlacementMapper::New();
  this->ActiveLabelsMapper->SetInputConnection(this->ActivePointSetToLabelHierarchyFilter->GetOutputPort());
  this->ActiveLabelsMapper->GetAnchorTransform()->SetCoordinateSystemToNormalizedViewport();
  this->ActiveLabelsActor = vtkActor2D::New();
  this->ActiveLabelsActor->SetMapper(this->ActiveLabelsMapper);

  this->HandleSize = 0.05;
}

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation2D::~vtkSlicerAbstractRepresentation2D()
{
  this->Mapper->Delete();
  this->Actor->Delete();

  this->SelectedMapper->Delete();
  this->SelectedActor->Delete();

  this->ActiveMapper->Delete();
  this->ActiveActor->Delete();

  this->Property->Delete();
  this->SelectedProperty->Delete();
  this->ActiveProperty->Delete();

  this->TextProperty->Delete();
  this->SelectedTextProperty->Delete();
  this->ActiveTextProperty->Delete();

  this->SetCursorShape(nullptr);
  this->SetSelectedCursorShape(nullptr);
  this->SetActiveCursorShape(nullptr);

  this->Glypher->Delete();
  this->SelectedGlypher->Delete();
  this->ActiveGlypher->Delete();

  this->LabelsFocalPoint->Delete();
  this->LabelsFocalData->Delete();

  this->SelectedLabelsFocalPoint->Delete();
  this->SelectedLabelsFocalData->Delete();

  this->ActiveLabelsFocalPoint->Delete();
  this->ActiveLabelsFocalData->Delete();

  this->Labels->Delete();
  this->LabelsPriority->Delete();
  this->LabelsActor->Delete();
  this->LabelsMapper->Delete();
  this->PointSetToLabelHierarchyFilter->Delete();

  this->SelectedLabels->Delete();
  this->SelectedLabelsPriority->Delete();
  this->SelectedLabelsActor->Delete();
  this->SelectedLabelsMapper->Delete();
  this->SelectedPointSetToLabelHierarchyFilter->Delete();

  this->ActiveLabels->Delete();
  this->ActiveLabelsPriority->Delete();
  this->ActiveLabelsActor->Delete();
  this->ActiveLabelsMapper->Delete();
  this->ActivePointSetToLabelHierarchyFilter->Delete();

  this->pointsVisibilityOnSlice->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::GetSliceToWorldCoordinates(double slicePos[2],
                                                                   double worldPos[3])
{
  if (this->Renderer == nullptr ||
      this->Renderer->GetRenderWindow() == nullptr ||
      this->Renderer->GetRenderWindow()->GetInteractor() == nullptr ||
      this->SliceNode == nullptr)
    {
    return;
    }

  double rasw[4], xyzw[4];
  xyzw[0] = slicePos[0] - this->Renderer->GetOrigin()[0];
  xyzw[1] = slicePos[1] - this->Renderer->GetOrigin()[1];
  xyzw[2] = 0.;
  xyzw[3] = 1.0;

  this->SliceNode->GetXYToRAS()->MultiplyPoint(xyzw, rasw);

  worldPos[0] = rasw[0]/rasw[3];
  worldPos[1] = rasw[1]/rasw[3];
  worldPos[2] = rasw[2]/rasw[3];
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::CreateDefaultProperties()
{
  this->Glypher = vtkGlyph2D::New();
  this->Glypher->SetInputData(this->FocalData);
  this->Glypher->SetScaleFactor(1.0);

  this->SelectedGlypher = vtkGlyph2D::New();
  this->SelectedGlypher->SetInputData(this->SelectedFocalData);
  this->SelectedGlypher->SetScaleFactor(1.0);

  this->ActiveGlypher = vtkGlyph2D::New();
  this->ActiveGlypher->SetInputData(this->ActiveFocalData);
  this->ActiveGlypher->SetScaleFactor(1.0);

  // By default the Points are rendered as spheres
  vtkNew<vtkSphereSource> ss;
  ss->SetRadius(0.5);
  ss->Update();
  this->SetCursorShape(ss->GetOutput());
  this->SetSelectedCursorShape(ss->GetOutput());
  this->SetActiveCursorShape(ss->GetOutput());

  this->Property = vtkProperty2D::New();
  this->Property->SetColor(0.4, 1.0, 1.0);
  this->Property->SetPointSize(10.);
  this->Property->SetLineWidth(2.);
  this->Property->SetOpacity(1.);

  this->SelectedProperty = vtkProperty2D::New();
  this->SelectedProperty->SetColor(1.0, 0.5, 0.5);
  this->SelectedProperty->SetPointSize(10.);
  this->SelectedProperty->SetLineWidth(2.);
  this->SelectedProperty->SetOpacity(1.);

  this->ActiveProperty = vtkProperty2D::New();
  // bright green
  this->ActiveProperty->SetColor(0.4, 1.0, 0.);
  this->ActiveProperty->SetPointSize(10.);
  this->ActiveProperty->SetLineWidth(2.);
  this->ActiveProperty->SetOpacity(1.);

  this->TextProperty = vtkTextProperty::New();
  this->TextProperty->SetJustificationToRight();
  this->TextProperty->SetFontSize(15);
  this->TextProperty->SetFontFamily(vtkTextProperty::GetFontFamilyFromString("Arial"));
  this->TextProperty->SetColor(0.4, 1.0, 1.0);
  this->TextProperty->SetOpacity(1.);

  this->SelectedTextProperty = vtkTextProperty::New();
  this->SelectedTextProperty->SetJustificationToRight();
  this->SelectedTextProperty->SetFontSize(15);
  this->SelectedTextProperty->SetFontFamily(vtkTextProperty::GetFontFamilyFromString("Arial"));
  this->SelectedTextProperty->SetColor(1.0, 0.5, 0.5);
  this->SelectedTextProperty->SetOpacity(1.);

  this->ActiveTextProperty = vtkTextProperty::New();
  this->ActiveTextProperty->SetJustificationToRight();
  this->ActiveTextProperty->SetFontSize(15);
  this->ActiveTextProperty->SetFontFamily(vtkTextProperty::GetFontFamilyFromString("Arial"));
  // bright green
  this->ActiveTextProperty->SetColor(0.4, 1.0, 0.);
  this->ActiveTextProperty->SetOpacity(1.);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::SetCursorShape(vtkPolyData *shape)
{
  if (shape != this->CursorShape)
    {
    if (this->CursorShape)
      {
      this->CursorShape->Delete();
      }
    this->CursorShape = shape;
    if (this->CursorShape)
      {
      this->CursorShape->Register(this);
      }
    if (this->CursorShape)
      {
      this->Glypher->SetSourceData(this->CursorShape);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerAbstractRepresentation2D::GetCursorShape()
{
  return this->CursorShape;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::SetSelectedCursorShape(vtkPolyData *shape)
{
  if (shape != this->SelectedCursorShape)
    {
    if (this->SelectedCursorShape)
      {
      this->SelectedCursorShape->Delete();
      }
    this->SelectedCursorShape = shape;
    if (this->SelectedCursorShape)
      {
      this->SelectedCursorShape->Register(this);
      }
    if (this->SelectedCursorShape)
      {
      this->SelectedGlypher->SetSourceData(this->SelectedCursorShape);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerAbstractRepresentation2D::GetSelectedCursorShape()
{
  return this->SelectedCursorShape;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::SetActiveCursorShape(
  vtkPolyData *shape)
{
  if (shape != this->ActiveCursorShape)
    {
    if (this->ActiveCursorShape)
      {
      this->ActiveCursorShape->Delete();
      }
    this->ActiveCursorShape = shape;
    if (this->ActiveCursorShape)
      {
      this->ActiveCursorShape->Register(this);
      }
    if (this->ActiveCursorShape)
      {
      this->ActiveGlypher->SetSourceData(this->ActiveCursorShape);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerAbstractRepresentation2D::GetActiveCursorShape()
{
  return this->ActiveCursorShape;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::SetSliceNode(vtkMRMLSliceNode *sliceNode)
{
  if (sliceNode == nullptr || this->SliceNode == sliceNode)
    {
    return;
    }

  this->SliceNode = sliceNode;
}

//----------------------------------------------------------------------
vtkMRMLSliceNode *vtkSlicerAbstractRepresentation2D::GetSliceNode()
{
  return this->SliceNode;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::GetNthNodeDisplayPosition(int n, double slicePos[2])
{
  if (!this->NodeExists(n) || !this->SliceNode)
    {
    return 0;
    }

  double sliceCoordinates[4], worldCoordinates[4];
  ControlPoint* node = this->GetNthNode(n);
  worldCoordinates[0] = node->WorldPosition.GetX();
  worldCoordinates[1] = node->WorldPosition.GetY();
  worldCoordinates[2] = node->WorldPosition.GetZ();
  worldCoordinates[3] = 1.0;

  vtkNew<vtkMatrix4x4> rasToxyMatrix;
  this->SliceNode->GetXYToRAS()->Invert(this->SliceNode->GetXYToRAS(), rasToxyMatrix.GetPointer());

  rasToxyMatrix->MultiplyPoint(worldCoordinates, sliceCoordinates);

  slicePos[0] = sliceCoordinates[0];
  slicePos[1] = sliceCoordinates[1];

  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::GetIntermediatePointDisplayPosition(int n, int idx, double displayPos[2])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  if (idx < 0 ||
      static_cast<unsigned int>(idx) >= this->GetNthNode(n)->intermadiatePoints.size())
    {
    return 0;
    }

  double sliceCoordinates[4], worldCoordinates[4];
  ControlPoint* node = this->GetNthNode(n);
  worldCoordinates[0] = node->intermadiatePoints[static_cast<unsigned int> (idx)].GetX();
  worldCoordinates[1] = node->intermadiatePoints[static_cast<unsigned int> (idx)].GetY();
  worldCoordinates[2] = node->intermadiatePoints[static_cast<unsigned int> (idx)].GetZ();
  worldCoordinates[3] = 1.0;

  vtkNew<vtkMatrix4x4> rasToxyMatrix;
  this->SliceNode->GetXYToRAS()->Invert(this->SliceNode->GetXYToRAS(), rasToxyMatrix.GetPointer());

  rasToxyMatrix->MultiplyPoint(worldCoordinates, sliceCoordinates);

  displayPos[0] = sliceCoordinates[0];
  displayPos[1] = sliceCoordinates[1];
  return 1;


}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::SetNthNodeDisplayPosition(int n, double slicePos[2])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  double worldPos[3];

  this->GetSliceToWorldCoordinates(slicePos, worldPos);
  this->SetNthNodeWorldPositionInternal(n, worldPos);

  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::AddNodeAtDisplayPosition(double slicePos[2])
{
  double worldPos[3];

  this->GetSliceToWorldCoordinates(slicePos, worldPos);
  this->AddNodeAtPositionInternal(worldPos);

  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::SetNthPointSliceVisibility(int n, bool visibility)
{
  this->pointsVisibilityOnSlice->InsertValue(n, visibility);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::BuildLocator()
{
  if (!this->RebuildLocator && !this->NeedToRender)
    {
    return;
    }

  vtkIdType size = this->GetNumberOfNodes();
  if (size < 1)
    {
    return;
    }

  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(size);

  double pos[3] = {0,0,0};
  for(int i = 0; i < size; i++)
    {
    this->GetNthNodeDisplayPosition(i, pos);
    points->InsertPoint(i,pos);
    }

  vtkPolyData *tmp = vtkPolyData::New();
  tmp->SetPoints(points);
  this->Locator->SetDataSet(tmp);
  tmp->FastDelete();
  points->FastDelete();

  //we fully updated the display locator
  this->RebuildLocator = false;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::AddNodeAtPositionInternal(double worldPos[3])
{
  if (!this->MarkupsNode)
    {
    return;
    }

  // Add a new point at this position
  vtkVector3d pos(worldPos[0], worldPos[1], worldPos[2]);
  this->MarkupsNode->DisableModifiedEventOn();
  this->MarkupsNode->AddControlPoint(pos);
  this->MarkupsNode->DisableModifiedEventOff();

  this->UpdateLines(this->GetNumberOfNodes() - 1);

  if (this->GetNumberOfNodes() - 1 >= this->pointsVisibilityOnSlice->GetNumberOfValues())
    {
    this->pointsVisibilityOnSlice->InsertValue(this->GetNumberOfNodes() - 1, 1);
    }
  else
    {
    this->pointsVisibilityOnSlice->InsertNextValue(1);
    }

  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  this->UpdateWidget();

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
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
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
  else
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
    }
  this->MarkupsNode->DisableModifiedEventOff();

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in slice coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the slice coordinates
// of the widget.
void vtkSlicerAbstractRepresentation2D::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Select)
    {
    this->TranslateNode(eventPos);
    }
  else if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Translate)
    {
    this->TranslateWidget(eventPos);
    }
  else if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Scale)
    {
    this->ScaleWidget(eventPos);
    }
  else if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Rotate)
    {
    this->RotateWidget(eventPos);
    }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::TranslateNode(double eventPos[2])
{
  if (this->GetActiveNodeLocked() || !this->MarkupsNode)
    {
    return;
    }

  double worldPos[3];
  this->GetSliceToWorldCoordinates(eventPos, worldPos);

  if (this->RestrictFlag != vtkSlicerAbstractRepresentation::RestrictNone)
    {
    double oldWorldPos[3];
    this->GetActiveNodeWorldPosition(oldWorldPos);
    for (int i = 0; i < 3; ++i)
      {
      worldPos[i] = (this->RestrictFlag == (i + 1)) ? worldPos[i] : oldWorldPos[i];
      }
    }
  this->MarkupsNode->DisableModifiedEventOn();
  this->SetActiveNodeToWorldPosition(worldPos);
  this->MarkupsNode->DisableModifiedEventOff();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::TranslateWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
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

  this->GetSliceToWorldCoordinates(slicePos, worldPos);

  double vector[3];
  vector[0] = worldPos[0] - ref[0];
  vector[1] = worldPos[1] - ref[1];
  vector[2] = worldPos[2] - ref[2];

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
      if (this->RestrictFlag != vtkSlicerAbstractRepresentation::RestrictNone)
        {
        worldPos[j] = (this->RestrictFlag == (j + 1)) ? ref[j] + vector[j] : ref[j];
        }
      else
        {
        worldPos[j] = ref[j] + vector[j];
        }
      }

    this->SetNthNodeWorldPosition(i, worldPos);
    }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::ScaleWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
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

  double centroid[3];
  ComputeCentroid(centroid);

  double r2 = vtkMath::Distance2BetweenPoints(ref, centroid);

  this->GetSliceToWorldCoordinates(slicePos, worldPos);

  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centroid);
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
      worldPos[j] = centroid[j] + ratio * (ref[j] - centroid[j]);
      }

    this->SetNthNodeWorldPosition(i, worldPos);
    }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::RotateWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
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

   double centroid[3];
   ComputeCentroid(centroid);

   this->GetSliceToWorldCoordinates(slicePos, worldPos);

   double d2 = vtkMath::Distance2BetweenPoints(worldPos, centroid);
   if (d2 < 0.0000001)
     {
     return;
     }

   for (int i = 0; i < 3; i++)
     {
     lastWorldPos[i] -= centroid[i];
     worldPos[i] -= centroid[i];
     }
   double angle = -vtkMath::DegreesFromRadians
                  (vtkMath::AngleBetweenVectors(lastWorldPos, worldPos));

   this->MarkupsNode->DisableModifiedEventOn();
   for (int i = 0; i < this->GetNumberOfNodes(); i++)
     {
     this->GetNthNodeWorldPosition(i, ref);
     for (int j = 0; j < 3; j++)
       {
       ref[j] -= centroid[j];
       }
     vtkNew<vtkTransform> RotateTransform;
     RotateTransform->RotateY(angle);
     RotateTransform->TransformPoint(ref, worldPos);

     for (int j = 0; j < 3; j++)
       {
       worldPos[j] += centroid[j];
       }

     this->SetNthNodeWorldPosition(i, worldPos);
     }
   this->MarkupsNode->DisableModifiedEventOff();
   this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
  this->SelectedActor->GetActors(pc);
  this->ActiveActor->GetActors(pc);
  this->LabelsActor->GetActors(pc);
  this->SelectedLabelsActor->GetActors(pc);
  this->ActiveLabelsActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->SelectedActor->ReleaseGraphicsResources(win);
  this->ActiveActor->ReleaseGraphicsResources(win);
  this->LabelsActor->ReleaseGraphicsResources(win);
  this->SelectedLabelsActor->ReleaseGraphicsResources(win);
  this->ActiveLabelsActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  if (this->Actor->GetVisibility())
    {
    count +=  this->Actor->RenderOverlay(viewport);
    }
  if (this->SelectedActor->GetVisibility())
    {
    count +=  this->SelectedActor->RenderOverlay(viewport);
    }
  if (this->ActiveActor->GetVisibility())
    {
    count +=  this->ActiveActor->RenderOverlay(viewport);
    }
  if (this->LabelsActor->GetVisibility())
    {
    count += this->LabelsActor->RenderOverlay(viewport);
    }
  if (this->SelectedLabelsActor->GetVisibility())
    {
    count += this->SelectedLabelsActor->RenderOverlay(viewport);
    }
  if (this->ActiveLabelsActor->GetVisibility())
    {
    count += this->ActiveLabelsActor->RenderOverlay(viewport);
    }

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count=0;
  if (this->Actor->GetVisibility())
    {
    count += this->Actor->RenderOpaqueGeometry(viewport);
    }
  if (this->SelectedActor->GetVisibility())
    {
    count += this->SelectedActor->RenderOpaqueGeometry(viewport);
    }
  if (this->ActiveActor->GetVisibility())
    {
    count += this->ActiveActor->RenderOpaqueGeometry(viewport);
    }
  if (this->LabelsActor->GetVisibility())
    {
    count += this->LabelsActor->RenderOpaqueGeometry(viewport);
    }
  if (this->SelectedLabelsActor->GetVisibility())
    {
    count += this->SelectedLabelsActor->RenderOpaqueGeometry(viewport);
    }
  if (this->ActiveLabelsActor->GetVisibility())
    {
    count += this->ActiveLabelsActor->RenderOpaqueGeometry(viewport);
    }

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  if (this->Actor->GetVisibility())
    {
    count += this->Actor->RenderTranslucentPolygonalGeometry(viewport);
    }
  if (this->SelectedActor->GetVisibility())
    {
    count += this->SelectedActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  if (this->ActiveActor->GetVisibility())
    {
    count += this->ActiveActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  if (this->LabelsActor->GetVisibility())
    {
    count += this->LabelsActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  if (this->SelectedLabelsActor->GetVisibility())
    {
    count += this->SelectedLabelsActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  if (this->ActiveLabelsActor->GetVisibility())
    {
    count += this->ActiveLabelsActor->RenderTranslucentPolygonalGeometry(viewport);
    }

  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerAbstractRepresentation2D::HasTranslucentPolygonalGeometry()
{
  int result=0;
  if (this->Actor->GetVisibility())
    {
    result |= this->Actor->HasTranslucentPolygonalGeometry();
    }
  if (this->SelectedActor->GetVisibility())
    {
    result |= this->SelectedActor->HasTranslucentPolygonalGeometry();
    }
  if (this->ActiveActor->GetVisibility())
    {
    result |= this->ActiveActor->HasTranslucentPolygonalGeometry();
    }
  if (this->LabelsActor->GetVisibility())
    {
    result |= this->LabelsActor->HasTranslucentPolygonalGeometry();
    }
  if (this->SelectedLabelsActor->GetVisibility())
    {
    result |= this->SelectedLabelsActor->HasTranslucentPolygonalGeometry();
    }
  if (this->ActiveLabelsActor->GetVisibility())
    {
    result |= this->ActiveLabelsActor->HasTranslucentPolygonalGeometry();
    }

  return result;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->Actor)
    {
     os << indent << "Points Visibility: " << this->Actor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Points: (none)\n";
    }

  if (this->SelectedActor)
    {
     os << indent << "Selected Points Visibility: " << this->SelectedActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Selected Points: (none)\n";
    }

  if (this->ActiveActor)
    {
    os << indent << "Active Points Visibility: " << this->ActiveActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Active Points: (none)\n";
    }

  if (this->LabelsActor)
    {
    os << indent << "Labels Visibility: " << this->LabelsActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Labels Points: (none)\n";
    }

  if (this->SelectedLabelsActor)
    {
    os << indent << "Selected Labels Visibility: " << this->LabelsActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Selected Labels Points: (none)\n";
    }

  if (this->ActiveLabelsActor)
    {
    os << indent << "Active Labels Visibility: " << this->LabelsActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Active Labels Points: (none)\n";
    }

  if (this->Property)
    {
    os << indent << "Property: " << this->Property << "\n";
    }
  else
    {
    os << indent << "Property: (none)\n";
    }

  if (this->SelectedProperty)
    {
    os << indent << "Selected Property: " << this->SelectedProperty << "\n";
    }
  else
    {
    os << indent << "Selected Property: (none)\n";
    }

  if (this->ActiveProperty)
    {
    os << indent << "Active Property: " << this->ActiveProperty << "\n";
    }
  else
    {
    os << indent << "Active Property: (none)\n";
    }
}
