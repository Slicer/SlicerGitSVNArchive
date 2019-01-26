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
#include "vtkCleanPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
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
#include "vtkSlicerLineInterpolator.h"
#include "vtkLinearSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkBox.h"
#include "vtkIntArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointPlacer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWindow.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkStringArray.h"
#include "vtkLabelHierarchy.h"
#include "vtkTextProperty.h"

#include <set>
#include <algorithm>
#include <iterator>

vtkCxxSetObjectMacro(vtkSlicerAbstractRepresentation, PointPlacer, vtkPointPlacer);
vtkCxxSetObjectMacro(vtkSlicerAbstractRepresentation, LineInterpolator, vtkSlicerLineInterpolator);

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation::vtkSlicerAbstractRepresentation()
{
  this->Tolerance                = 0.5;
  this->PixelTolerance           = 1;
  this->PointPlacer              = nullptr;
  this->LineInterpolator         = nullptr;
  this->Locator                  = nullptr;
  this->RebuildLocator           = false;
  this->NeedToRender             = 0;
  this->ClosedLoop               = 0;
  this->CurrentOperation         = vtkSlicerAbstractRepresentation::Inactive;

  this->ResetLocator();

  // Initialize state
  this->InteractionState = vtkSlicerAbstractRepresentation::Outside;

  this->CursorShape = nullptr;
  this->SelectedCursorShape = nullptr;
  this->ActiveCursorShape = nullptr;

  this->PointPlacer = vtkFocalPlanePointPlacer::New();

  this->TextProperty = vtkTextProperty::New();
  this->TextProperty->SetFontSize(15);
  this->TextProperty->SetFontFamily(vtkTextProperty::GetFontFamilyFromString("Arial"));
  this->TextProperty->SetColor(0.4, 1.0, 1.0);
  this->TextProperty->SetOpacity(1.);

  this->SelectedTextProperty = vtkTextProperty::New();
  this->SelectedTextProperty->SetFontSize(15);
  this->SelectedTextProperty->SetFontFamily(vtkTextProperty::GetFontFamilyFromString("Arial"));
  this->SelectedTextProperty->SetColor(1.0, 0.5, 0.5);
  this->SelectedTextProperty->SetOpacity(1.);

  this->ActiveTextProperty = vtkTextProperty::New();
  this->ActiveTextProperty->SetFontSize(15);
  this->ActiveTextProperty->SetFontFamily(vtkTextProperty::GetFontFamilyFromString("Arial"));
  // bright green
  this->ActiveTextProperty->SetColor(0.4, 1.0, 0.);
  this->ActiveTextProperty->SetOpacity(1.);

  this->FocalPoint = vtkPoints::New();
  this->FocalPoint->SetNumberOfPoints(100);
  this->FocalPoint->SetNumberOfPoints(1);
  this->FocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *normals = vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(100);
  normals->SetNumberOfTuples(1);
  double n[3] = {0, 0, 0};
  normals->SetTuple(0, n);

  this->SelectedFocalPoint = vtkPoints::New();
  this->SelectedFocalPoint->SetNumberOfPoints(100);
  this->SelectedFocalPoint->SetNumberOfPoints(1);
  this->SelectedFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *selectedNormals = vtkDoubleArray::New();
  selectedNormals->SetNumberOfComponents(3);
  selectedNormals->SetNumberOfTuples(100);
  selectedNormals->SetNumberOfTuples(1);
  selectedNormals->SetTuple(0, n);

  this->ActiveFocalPoint = vtkPoints::New();
  this->ActiveFocalPoint->SetNumberOfPoints(100);
  this->ActiveFocalPoint->SetNumberOfPoints(1);
  this->ActiveFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *activeNormals = vtkDoubleArray::New();
  activeNormals->SetNumberOfComponents(3);
  activeNormals->SetNumberOfTuples(100);
  activeNormals->SetNumberOfTuples(1);
  activeNormals->SetTuple(0, n);

  this->FocalData = vtkPolyData::New();
  this->FocalData->SetPoints(this->FocalPoint);
  this->FocalData->GetPointData()->SetNormals(normals);
  normals->Delete();

  this->SelectedFocalData = vtkPolyData::New();
  this->SelectedFocalData->SetPoints(this->SelectedFocalPoint);
  this->SelectedFocalData->GetPointData()->SetNormals(selectedNormals);
  selectedNormals->Delete();

  this->ActiveFocalData = vtkPolyData::New();
  this->ActiveFocalData->SetPoints(this->ActiveFocalPoint);
  this->ActiveFocalData->GetPointData()->SetNormals(activeNormals);
  activeNormals->Delete();

  // Labels
  this->LabelsFocalPoint = vtkPoints::New();
  this->LabelsFocalPoint->SetNumberOfPoints(100);
  this->LabelsFocalPoint->SetNumberOfPoints(1);
  this->LabelsFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *normalsLabels = vtkDoubleArray::New();
  normalsLabels->SetNumberOfComponents(3);
  normalsLabels->SetNumberOfTuples(100);
  normalsLabels->SetNumberOfTuples(1);
  normalsLabels->SetTuple(0, n);

  this->SelectedLabelsFocalPoint = vtkPoints::New();
  this->SelectedLabelsFocalPoint->SetNumberOfPoints(100);
  this->SelectedLabelsFocalPoint->SetNumberOfPoints(1);
  this->SelectedLabelsFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *selectedNormalsLabels = vtkDoubleArray::New();
  selectedNormalsLabels->SetNumberOfComponents(3);
  selectedNormalsLabels->SetNumberOfTuples(100);
  selectedNormalsLabels->SetNumberOfTuples(1);
  selectedNormalsLabels->SetTuple(0, n);

  this->ActiveLabelsFocalPoint = vtkPoints::New();
  this->ActiveLabelsFocalPoint->SetNumberOfPoints(100);
  this->ActiveLabelsFocalPoint->SetNumberOfPoints(1);
  this->ActiveLabelsFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  vtkDoubleArray *activeNormalsLabels = vtkDoubleArray::New();
  activeNormalsLabels->SetNumberOfComponents(3);
  activeNormalsLabels->SetNumberOfTuples(100);
  activeNormalsLabels->SetNumberOfTuples(1);
  activeNormalsLabels->SetTuple(0, n);

  this->LabelsFocalData = vtkPolyData::New();
  this->LabelsFocalData->SetPoints(this->LabelsFocalPoint);
  this->LabelsFocalData->GetPointData()->SetNormals(normalsLabels);
  normalsLabels->Delete();

  this->SelectedLabelsFocalData = vtkPolyData::New();
  this->SelectedLabelsFocalData->SetPoints(this->SelectedLabelsFocalPoint);
  this->SelectedLabelsFocalData->GetPointData()->SetNormals(selectedNormalsLabels);
  selectedNormalsLabels->Delete();

  this->ActiveLabelsFocalData = vtkPolyData::New();
  this->ActiveLabelsFocalData->SetPoints(this->ActiveLabelsFocalPoint);
  this->ActiveLabelsFocalData->GetPointData()->SetNormals(activeNormalsLabels);
  activeNormalsLabels->Delete();

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

  this->AlwaysOnTop = 0;

  this->RestrictFlag = RestrictNone;

  this->MarkupsNode = nullptr;
}

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation::~vtkSlicerAbstractRepresentation()
{
  this->PointPlacer->Delete();

  if (this->Locator)
    {
    this->Locator->Delete();
    }

  this->FocalPoint->Delete();
  this->FocalData->Delete();

  this->SelectedFocalPoint->Delete();
  this->SelectedFocalData->Delete();

  this->ActiveFocalPoint->Delete();
  this->ActiveFocalData->Delete();

  this->LabelsFocalPoint->Delete();
  this->LabelsFocalData->Delete();

  this->Labels->Delete();
  this->LabelsPriority->Delete();
  this->PointSetToLabelHierarchyFilter->Delete();

  this->SelectedLabelsFocalPoint->Delete();
  this->SelectedLabelsFocalData->Delete();

  this->SelectedLabels->Delete();
  this->SelectedLabelsPriority->Delete();
  this->SelectedPointSetToLabelHierarchyFilter->Delete();

  this->ActiveLabelsFocalPoint->Delete();
  this->ActiveLabelsFocalData->Delete();

  this->ActiveLabels->Delete();
  this->ActiveLabelsPriority->Delete();
  this->ActivePointSetToLabelHierarchyFilter->Delete();

  this->TextProperty->Delete();
  this->SelectedTextProperty->Delete();
  this->ActiveTextProperty->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::ResetLocator()
{
  if (this->Locator)
    {
    this->Locator->Delete();
    }

  this->Locator = vtkIncrementalOctreePointLocator::New();
  this->Locator->SetBuildCubicOctree(1);
  this->RebuildLocator = true;
}

//----------------------------------------------------------------------
double vtkSlicerAbstractRepresentation::CalculateViewScaleFactor()
{
  double p1[4], p2[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(p1);
  p1[3] = 1.0;
  this->Renderer->SetWorldPoint(p1);
  this->Renderer->WorldToView();
  this->Renderer->GetViewPoint(p1);

  double depth = p1[2];
  double aspect[2];
  this->Renderer->ComputeAspect();
  this->Renderer->GetAspect(aspect);

  p1[0] = -aspect[0];
  p1[1] = -aspect[1];
  this->Renderer->SetViewPoint(p1);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(p1);

  p2[0] = aspect[0];
  p2[1] = aspect[1];
  p2[2] = depth;
  p2[3] = 1.0;
  this->Renderer->SetViewPoint(p2);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(p2);

  double distance = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));

  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double viewport[4];
  this->Renderer->GetViewport(viewport);

  double x, y, distance2;

  x = size[0] * (viewport[2] - viewport[0]);
  y = size[1] * (viewport[3] - viewport[1]);

  distance2 = sqrt(x * x + y * y);
  return distance2 / distance;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::ClearAllNodes()
{
  this->ResetLocator();
  this->BuildLines();
  this->BuildLocator();
  this->NeedToRender = 1;
  this->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::AddNodeAtPositionInternal(double worldPos[3])
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
  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::GetNodePolyData(vtkPolyData* poly)
{
  poly->Initialize();
  int count = this->GetNumberOfNodes();

  if (count == 0)
    {
    return;
    }

  vtkPoints *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();

  points->SetNumberOfPoints(count);
  vtkIdType numLines = count;

  if (this->ClosedLoop)
    {
    numLines++;
    }

  vtkIdType *lineIndices = new vtkIdType[numLines];

  int i;
  vtkIdType index = 0;
  double pos[3];

  for (i = 0; i < this->GetNumberOfNodes(); ++i)
    {
    // Add the node
    this->GetNthNodeWorldPosition(i, pos);
    points->InsertPoint(index, pos);
    lineIndices[index] = index;
    index++;
    }

  if (this->ClosedLoop)
    {
    lineIndices[index] = 0;
    }

  lines->InsertNextCell(numLines, lineIndices);
  delete [] lineIndices;

  poly->SetPoints(points);
  poly->SetLines(lines);

  points->Delete();
  lines->Delete();
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtWorldPosition(double worldPos[3])
{
  this->AddNodeAtPositionInternal(worldPos);
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtWorldPosition(
  double x, double y, double z)
{
  double worldPos[3] = {x, y, z};
  return this->AddNodeAtWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtDisplayPosition(double displayPos[2])
{
  double worldPos[3], worldOrient[9], orientation[4];
  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                                 displayPos, worldPos,
                                                 worldOrient))
    {
    return 0;
    }

  this->AddNodeAtPositionInternal(worldPos);
  this->FromWorldOrientToOrientationQuaternion(worldOrient, orientation);
  this->SetNthNodeOrientation(this->GetNumberOfNodes() - 1,  orientation);
  return 1;
}
//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtDisplayPosition(int displayPos[2])
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->AddNodeAtDisplayPosition(doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtDisplayPosition(int X, int Y)
{
  double displayPos[2];
  displayPos[0] = X;
  displayPos[1] = Y;
  return this->AddNodeAtDisplayPosition(displayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::ActivateNode(double displayPos[2])
{
  this->BuildLocator();

  double dPos[3] = {displayPos[0],displayPos[1],0};

  double scale = this->CalculateViewScaleFactor();
  this->PixelTolerance = (this->HandleSize + this->HandleSize * this->Tolerance) * scale;
  double closestDistance2 = VTK_DOUBLE_MAX;
  int closestNode = static_cast<int> (this->Locator->FindClosestPointWithinRadius(
    this->PixelTolerance, dPos, closestDistance2));

  if (closestNode != this->GetActiveNode())
    {
    this->SetActiveNode(closestNode);
    this->NeedToRender = 1;
    }
  return (this->GetActiveNode() >= 0);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::ActivateNode(int displayPos[2])
{
  double doubleDisplayPos[2];

  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->ActivateNode(doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::ActivateNode(int X, int Y)
{
  double doubleDisplayPos[2];

  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->ActivateNode(doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetActiveNodeToWorldPosition(double worldPos[3])
{
  if (this->GetActiveNode() < 0 ||
       this->GetActiveNode() >= this->GetNumberOfNodes())
    {
    return 0;
    }

  // Check if this is a valid location
  if (!this->PointPlacer->ValidateWorldPosition(worldPos))
    {
    return 0;
    }

  this->SetNthNodeWorldPositionInternal(this->GetActiveNode(),
                                        worldPos);
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetActiveNodeToDisplayPosition(double displayPos[2])
{
  if (this->GetActiveNode() < 0 ||
       this->GetActiveNode() >= this->GetNumberOfNodes())
    {
    return 0;
    }

  double worldPos[3], worldOrient[9];
  this->GetActiveNodeOrientation(worldOrient);

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, worldPos,
                                               worldOrient))
    {
    return 0;
    }

  this->SetNthNodeWorldPositionInternal(this->GetActiveNode(),
                                        worldPos);
  this->SetActiveNodeOrientation(worldOrient);
  return 1;
}
//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetActiveNodeToDisplayPosition(int displayPos[2])
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->SetActiveNodeToDisplayPosition(doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetActiveNodeToDisplayPosition(int X, int Y)
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->SetActiveNodeToDisplayPosition(doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetActiveNode()
{
  if (!this->MarkupsNode)
    {
    return -1;
    }

  return this->MarkupsNode->GetActiveControlPoint();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNode(int index)
{
  if (!this->MarkupsNode)
    {
    return;
    }

  this->MarkupsNode->SetActiveControlPoint(index);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetActiveNodeWorldPosition(double pos[3])
{
  return this->GetNthNodeWorldPosition(this->GetActiveNode(), pos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetActiveNodeDisplayPosition(double pos[2])
{
  return this->GetNthNodeDisplayPosition(this->GetActiveNode(), pos);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeVisibility(bool visibility)
{
  this->SetNthNodeVisibility(this->GetActiveNode(), visibility);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetActiveNodeVisibility()
{
  return this->GetNthNodeVisibility(this->GetActiveNode());
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeSelected(bool selected)
{
  this->SetNthNodeSelected(this->GetActiveNode(), selected);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetActiveNodeSelected()
{
  return this->GetNthNodeSelected(this->GetActiveNode());
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeLocked(bool locked)
{
  this->SetNthNodeLocked(this->GetActiveNode(), locked);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetActiveNodeLocked()
{
  return this->GetNthNodeLocked(this->GetActiveNode());
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeOrientation(double orientation[4])
{
  this->SetNthNodeOrientation(this->GetActiveNode(), orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::GetActiveNodeOrientation(double orientation[4])
{
  this->GetNthNodeOrientation(this->GetActiveNode(), orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeLabel(vtkStdString label)
{
  this->SetNthNodeLabel(this->GetActiveNode(), label);
}

//----------------------------------------------------------------------
vtkStdString vtkSlicerAbstractRepresentation::GetActiveNodeLabel()
{
  return this->GetNthNodeLabel(this->GetActiveNode());
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetNumberOfNodes()
{
  if (!this->MarkupsNode)
    {
    return 0;
    }

  return static_cast<int>(this->MarkupsNode->GetNumberOfControlPoints());
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetNumberOfIntermediatePoints(int n)
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  return static_cast<int> (this->MarkupsNode->GetNthControlPoint(n)->intermadiatePoints.size());
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetIntermediatePointWorldPosition(int n,
                                                                       int idx,
                                                                       double point[3])
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

  point[0] = this->GetNthNode(n)->intermadiatePoints[static_cast<unsigned int> (idx)].GetX();
  point[1] = this->GetNthNode(n)->intermadiatePoints[static_cast<unsigned int> (idx)].GetY();
  point[2] = this->GetNthNode(n)->intermadiatePoints[static_cast<unsigned int> (idx)].GetZ();

  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetIntermediatePointDisplayPosition(int n,
                                                                         int idx,
                                                                         double displayPos[2])
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

  double pos[4];
  ControlPoint* node = this->GetNthNode(n);
  pos[0] = node->intermadiatePoints[static_cast<unsigned int> (idx)].GetX();
  pos[1] = node->intermadiatePoints[static_cast<unsigned int> (idx)].GetY();
  pos[2] = node->intermadiatePoints[static_cast<unsigned int> (idx)].GetZ();
  pos[3] = 1.0;

  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  displayPos[0] = pos[0];
  displayPos[1] = pos[1];
  return 1;
}

//----------------------------------------------------------------------
// The display position for a given world position must be re-computed
// from the world positions... It should not be queried from the renderer
// whose camera position may have changed
int vtkSlicerAbstractRepresentation::GetNthNodeDisplayPosition(int n, double displayPos[2])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  double pos[4];
  ControlPoint* node = this->GetNthNode(n);
  pos[0] = node->WorldPosition.GetX();
  pos[1] = node->WorldPosition.GetY();
  pos[2] = node->WorldPosition.GetZ();
  pos[3] = 1.0;

  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  displayPos[0] = pos[0];
  displayPos[1] = pos[1];
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetNthNodeWorldPosition(int n, double worldPos[3])
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return 0;
    }

  this->MarkupsNode->GetNthControlPointPosition(n, worldPos);

  return 1;
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetNthNodeVisibility(int n)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return false;
    }

  return this->MarkupsNode->GetNthControlPointVisibility(n);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeVisibility(int n, bool visibility)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return;
    }

  return this->MarkupsNode->SetNthControlPointVisibility(n, visibility);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetNthNodeSelected(int n)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return false;
    }

  return this->MarkupsNode->GetNthControlPointSelected(n);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeSelected(int n, bool selected)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return;
    }

  return this->MarkupsNode->SetNthControlPointSelected(n, selected);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetNthNodeLocked(int n)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return false;
    }

  return this->MarkupsNode->GetNthControlPointLocked(n);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeLocked(int n, bool locked)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return;
    }

  return this->MarkupsNode->SetNthControlPointLocked(n, locked);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeOrientation(int n, double orientation[4])
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return;
    }

  this->MarkupsNode->SetNthControlPointOrientationFromArray(n, orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::GetNthNodeOrientation(int n, double orientation[4])
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return;
    }

  this->MarkupsNode->GetNthControlPointOrientation(n, orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeLabel(int n, vtkStdString label)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return;
    }

  this->MarkupsNode->SetNthControlPointLabel(n, label);
}

//----------------------------------------------------------------------
vtkStdString vtkSlicerAbstractRepresentation::GetNthNodeLabel(int n)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return nullptr;
    }

  return this->MarkupsNode->GetNthControlPointLabel(n);
}

//----------------------------------------------------------------------
ControlPoint* vtkSlicerAbstractRepresentation::GetNthNode(int n)
{
  if (!this->NodeExists(n))
    {
    return nullptr;
    }

  return this->MarkupsNode->GetNthControlPoint(n);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::NodeExists(int n)
{
  if (!this->MarkupsNode)
    {
    return false;
    }

  return this->MarkupsNode->ControlPointExists(n);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeWorldPositionInternal(int n, double worldPos[3])
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return;
    }

  this->MarkupsNode->SetNthControlPointPositionFromArray(n, worldPos);

  this->UpdateLines(n);
  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::FromWorldOrientToOrientationQuaternion(double worldOrient[9], double orientation[4])
{
  if (!worldOrient || !orientation)
    {
    return;
    }

  double worldOrientMatrix[3][3];
  worldOrientMatrix[0][0] = worldOrient[0];
  worldOrientMatrix[0][1] = worldOrient[1];
  worldOrientMatrix[0][2] = worldOrient[2];
  worldOrientMatrix[1][0] = worldOrient[3];
  worldOrientMatrix[1][1] = worldOrient[4];
  worldOrientMatrix[1][2] = worldOrient[5];
  worldOrientMatrix[2][0] = worldOrient[6];
  worldOrientMatrix[2][1] = worldOrient[7];
  worldOrientMatrix[2][2] = worldOrient[8];

  vtkMath::Matrix3x3ToQuaternion(worldOrientMatrix, orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::FromOrientationQuaternionToWorldOrient(double orientation[4], double worldOrient[9])
{
  if (!worldOrient || !orientation)
    {
    return;
    }

  double worldOrientMatrix[3][3];
  vtkMath::QuaternionToMatrix3x3(orientation, worldOrientMatrix);

  worldOrient[0] = worldOrientMatrix[0][0];
  worldOrient[1] = worldOrientMatrix[0][1];
  worldOrient[2] = worldOrientMatrix[0][2];
  worldOrient[3] = worldOrientMatrix[1][0];
  worldOrient[4] = worldOrientMatrix[1][1];
  worldOrient[5] = worldOrientMatrix[1][2];
  worldOrient[6] = worldOrientMatrix[2][0];
  worldOrient[7] = worldOrientMatrix[2][1];
  worldOrient[8] = worldOrientMatrix[2][2];
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetNthNodeWorldPosition(int n, double worldPos[3])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  // Check if this is a valid location
  if (!this->PointPlacer->ValidateWorldPosition(worldPos))
    {
    return 0;
    }

  this->SetNthNodeWorldPositionInternal(n, worldPos);
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetNthNodeDisplayPosition(int n, double displayPos[2])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  double worldPos[3], worldOrient[9], orientation[4];
  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                                 displayPos, worldPos,
                                                 worldOrient))
    {
    return 0;
    }

  this->FromWorldOrientToOrientationQuaternion(worldOrient, orientation);
  this->SetNthNodeOrientation(n,  orientation);
  return this->SetNthNodeWorldPosition(n, worldPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetNthNodeDisplayPosition(int n, int displayPos[2])
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->SetNthNodeDisplayPosition(n, doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetNthNodeDisplayPosition(int n, int X, int Y)
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->SetNthNodeDisplayPosition(n, doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::FindClosestPointOnWidget(int X, int Y,
                                                              double closestWorldPos[3],
                                                              int *idx)
{
  // Make a line out of this viewing ray
  double p1[4], p2[4], *p3 = nullptr, *p4 = nullptr;

  double tmp1[4], tmp2[4];
  tmp1[0] = X;
  tmp1[1] = Y;
  tmp1[2] = 0.0;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(p1);

  tmp1[2] = 1.0;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(p2);

  double closestDistance2 = VTK_DOUBLE_MAX;
  int closestNode=0;

  // compute a world tolerance based on pixel
  // tolerance on the focal plane
  double fp[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  fp[3] = 1.0;
  this->Renderer->SetWorldPoint(fp);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(tmp1);

  tmp1[0] = 0;
  tmp1[1] = 0;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(tmp2);

  tmp1[0] = this->PixelTolerance;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(tmp1);

  double wt2 = vtkMath::Distance2BetweenPoints(tmp1, tmp2);

  // Now loop through all lines and look for closest one within tolerance
  for(int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (!this->NodeExists(i))
      {
      continue;
      }
    for (unsigned int j = 0; j <= this->GetNthNode(i)->intermadiatePoints.size(); j++)
      {
      if (j == 0)
        {
        p3 = this->GetNthNode(i)->WorldPosition.GetData();
        if (!this->GetNthNode(i)->intermadiatePoints.empty())
          {
          p4 = this->GetNthNode(i)->intermadiatePoints[j].GetData();
          }
        else
          {
          if (i < this->GetNumberOfNodes() - 1)
            {
            p4 = this->GetNthNode(i + 1)->WorldPosition.GetData();
            }
          else if (this->ClosedLoop)
            {
            p4 = this->GetNthNode(0)->WorldPosition.GetData();
            }
          }
        }
      else if (j == this->GetNthNode(i)->intermadiatePoints.size())
        {
        p3 = this->GetNthNode(i)->intermadiatePoints[j-1].GetData();
        if (i < this->GetNumberOfNodes() - 1)
          {
          p4 = this->GetNthNode(i + 1)->WorldPosition.GetData();
          }
        else if (this->ClosedLoop)
          {
          p4 = this->GetNthNode(0)->WorldPosition.GetData();
          }
        else
          {
          // Shouldn't be able to get here (only if we don't have
          // a closed loop but we do have intermediate points after
          // the last node - contradictary conditions)
          continue;
          }
        }
      else
        {
        p3 = this->GetNthNode(i)->intermadiatePoints[j-1].GetData();
        p4 = this->GetNthNode(i)->intermadiatePoints[j].GetData();
        }

      // Now we have the four points - check closest intersection
      double u, v;

      if (vtkLine::Intersection(p1, p2, p3, p4, u, v))
        {
        double p5[3], p6[3];
        p5[0] = p1[0] + u*(p2[0]-p1[0]);
        p5[1] = p1[1] + u*(p2[1]-p1[1]);
        p5[2] = p1[2] + u*(p2[2]-p1[2]);

        p6[0] = p3[0] + v*(p4[0]-p3[0]);
        p6[1] = p3[1] + v*(p4[1]-p3[1]);
        p6[2] = p3[2] + v*(p4[2]-p3[2]);

        double d = vtkMath::Distance2BetweenPoints(p5, p6);

        if (d < wt2 && d < closestDistance2)
          {
          closestWorldPos[0] = p6[0];
          closestWorldPos[1] = p6[1];
          closestWorldPos[2] = p6[2];
          closestDistance2 = d;
          closestNode = static_cast<int>(i);
          }
        }
      else
        {
        double d = vtkLine::DistanceToLine(p3, p1, p2);
        if (d < wt2 && d < closestDistance2)
          {
          closestWorldPos[0] = p3[0];
          closestWorldPos[1] = p3[1];
          closestWorldPos[2] = p3[2];
          closestDistance2 = d;
          closestNode = static_cast<int>(i);
          }

        d = vtkLine::DistanceToLine(p4, p1, p2);
        if (d < wt2 && d < closestDistance2)
          {
          closestWorldPos[0] = p4[0];
          closestWorldPos[1] = p4[1];
          closestWorldPos[2] = p4[2];
          closestDistance2 = d;
          closestNode = static_cast<int>(i);
          }
        }
      }
    }

  if (closestDistance2 < VTK_DOUBLE_MAX)
    {
    if (closestNode < this->GetNumberOfNodes() -1)
      {
      *idx = closestNode+1;
      return 1;
      }
    else if (this->ClosedLoop)
      {
      *idx = 0;
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeOnWidget(int X, int Y)
{
  if (!this->MarkupsNode)
    {
    return 0;
    }

  int idx;
  double worldPos[3], worldOrient[9], displayPos[2], orientation[4];
  displayPos[0] = X;
  displayPos[1] = Y;

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, worldPos,
                                               worldOrient))
    {
    return 0;
    }

  double pos[3];
  if (!this->FindClosestPointOnWidget(X, Y, pos, &idx))
    {
    return 0;
    }

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos,
                                               pos,
                                               worldPos,
                                               worldOrient))
    {
    return 0;
    }

  // Add a new point at this position
  ControlPoint *node = new ControlPoint;
  this->MarkupsNode->InitControlPoint(node);

  node->WorldPosition.SetX(worldPos[0]);
  node->WorldPosition.SetY(worldPos[1]);
  node->WorldPosition.SetZ(worldPos[2]);

  this->MarkupsNode->DisableModifiedEventOn();
  this->MarkupsNode->InsertControlPoint(node, idx);
  this->FromWorldOrientToOrientationQuaternion(worldOrient, orientation);
  this->SetNthNodeOrientation(idx,  orientation);
  this->MarkupsNode->DisableModifiedEventOff();

  this->UpdateLines(idx);
  this->NeedToRender = 1;

  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::DeleteNthNode(int n)
{
  if (!this->MarkupsNode || !this->NodeExists(n))
    {
    return 0;
    }

  this->MarkupsNode->DisableModifiedEventOn();
  this->MarkupsNode->RemoveNthControlPoint(n);
  this->MarkupsNode->DisableModifiedEventOff();

  this->UpdateLines(n - 1);

  this->NeedToRender = 1;
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::DeleteActiveNode()
{
  return this->DeleteNthNode(this->GetActiveNode());
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::DeleteLastNode()
{
  if (!this->MarkupsNode)
    {
    return 0;
    }

  this->MarkupsNode->DisableModifiedEventOn();
  this->MarkupsNode->RemoveLastControlPoint();
  this->MarkupsNode->DisableModifiedEventOff();

  this->UpdateLines(this->GetNumberOfNodes());

  this->NeedToRender = 1;
  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::UpdateLines(int index)
{
  int indices[2];

  if (this->LineInterpolator)
    {
    vtkIntArray *arr = vtkIntArray::New();
    this->LineInterpolator->GetSpan(index, arr, this);

    int nNodes = static_cast<int> (arr->GetNumberOfTuples());
    for (int i = 0; i < nNodes; i++)
      {
      arr->GetTypedTuple(i, indices);
      this->UpdateLine(indices[0], indices[1]);
      }
    arr->Delete();
    }

  // A check to make sure that we have no line segments in
  // the last node if the loop is not closed
  if (!this->ClosedLoop && this->GetNumberOfNodes() > 0)
    {
    int idx = this->GetNumberOfNodes() - 1;
    this->GetNthNode(idx)->intermadiatePoints.clear();
    }

  this->BuildLines();
  this->RebuildLocator = true;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddIntermediatePointWorldPosition(int n,
                                                                       double pos[3])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  vtkVector3d point;
  point.Set(pos[0], pos[1], pos[2]);

  this->GetNthNode(n)->intermadiatePoints.push_back(point);
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetNthNodeSlope(int n, double slope[3])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  int idx1, idx2;

  if (n == 0 && !this->ClosedLoop)
    {
    idx1 = 0;
    idx2 = 1;
    }
  else if (n == this->GetNumberOfNodes() - 1 && !this->ClosedLoop)
    {
    idx1 = this->GetNumberOfNodes() - 2;
    idx2 = idx1+1;
    }
  else
    {
    idx1 = n - 1;
    idx2 = n + 1;

    if (idx1 < 0)
      {
      idx1 += this->GetNumberOfNodes();
      }
    if (idx2 >= this->GetNumberOfNodes())
      {
      idx2 -= this->GetNumberOfNodes();
      }
    }

  slope[0] =
    this->GetNthNode(idx2)->WorldPosition.GetX() -
    this->GetNthNode(idx1)->WorldPosition.GetX();
  slope[1] =
    this->GetNthNode(idx2)->WorldPosition.GetY() -
    this->GetNthNode(idx1)->WorldPosition.GetY();
  slope[2] =
    this->GetNthNode(idx2)->WorldPosition.GetZ() -
    this->GetNthNode(idx1)->WorldPosition.GetZ();

  vtkMath::Normalize(slope);
  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::UpdateLine(int idx1, int idx2)
{
  if (!this->LineInterpolator || !this->NodeExists(idx1))
    {
    return;
    }

  // Clear all the points at idx1
  this->GetNthNode(idx1)->intermadiatePoints.clear();

  this->LineInterpolator->InterpolateLine(this, idx1, idx2);
}

//---------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::UpdateWidget(bool force /*=false*/)
{
  if (!this->Locator || !this->PointPlacer)
    {
    return 0;
    }

  this->PointPlacer->UpdateInternalState();

  //even if just the camera has moved we need to mark the locator
  //as needing to be rebuilt
  if (this->Locator->GetMTime() < this->Renderer->GetActiveCamera()->GetMTime())
    {
    this->RebuildLocator = true;
    }

  if (this->WidgetBuildTime > this->PointPlacer->GetMTime() && !force)
    {
    // Widget does not need to be rebuilt
    return 0;
    }

  for(int i = 0; (i + 1) < this->GetNumberOfNodes(); i++)
    {
    this->UpdateLine(i, i + 1);
    }

  if (this->ClosedLoop)
    {
    this->UpdateLine(this->GetNumberOfNodes() - 1, 0);
    }
  this->BuildLines();
  this->RebuildLocator = true;

  this->WidgetBuildTime.Modified();

  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation
::GetRendererComputedDisplayPositionFromWorldPosition(double worldPos[3],
                                                      double displayPos[2])
{  
  double pos[4];
  pos[0] = worldPos[0];
  pos[1] = worldPos[1];
  pos[2] = worldPos[2];
  pos[3] = 1.0;

  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  displayPos[0] = static_cast<int>(pos[0]);
  displayPos[1] = static_cast<int>(pos[1]);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::Initialize(vtkPolyData * pd)
{
  if (!this->MarkupsNode)
    {
    return;
    }

  vtkPoints *points   = pd->GetPoints();
  vtkIdType nPoints = points->GetNumberOfPoints();
  if (nPoints <= 0)
    {
    return; // Yeah right.. build from nothing !
    }

  // Clear all existing nodes.
  this->MarkupsNode->DisableModifiedEventOn();
  this->MarkupsNode->RemoveAllControlPoints();
  this->MarkupsNode->DisableModifiedEventOff();

  vtkPolyData *tmpPoints = vtkPolyData::New();
  tmpPoints->DeepCopy(pd);
  this->Locator->SetDataSet(tmpPoints);
  tmpPoints->Delete();

  //reserver space in memory to speed up vector push_back
  this->MarkupsNode->GetControlPoints()->reserve(static_cast<unsigned int> (nPoints));
  vtkIdList *pointIds = pd->GetCell(0)->GetPointIds();

  // Get the worldOrient from the point placer
  double ref[3], displayPos[2], worldPos[3], worldOrient[9], orientation[4];
  ref[0] = 0.0; ref[1] = 0.0; ref[2] = 0.0;
  displayPos[0] = 0.0; displayPos[1] = 0.0;
  this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                          displayPos, ref, worldPos, worldOrient);

  // Add nodes without calling rebuild lines
  // to improve performance dramatically(~15x) on large datasets
  double *pos;
  for (vtkIdType i=0; i < nPoints; i++)
    {
    pos = points->GetPoint(i);
    // Check if this is a valid location
    if (!this->PointPlacer->ValidateWorldPosition(pos))
      {
      continue;
      }

    this->GetRendererComputedDisplayPositionFromWorldPosition(pos, displayPos);

    // Add a new point at this position

    this->MarkupsNode->DisableModifiedEventOn();
    vtkVector3d controlPointPos(pos[0], pos[1], pos[2]);
    int pointIndex = this->MarkupsNode->AddControlPoint(controlPointPos);
    this->FromWorldOrientToOrientationQuaternion(worldOrient, orientation);
    this->SetNthNodeOrientation(pointIndex,  orientation);
    this->MarkupsNode->DisableModifiedEventOff();
    }

  if (pointIds->GetNumberOfIds() > nPoints)
    {
    this->ClosedLoopOn();
    }

  // Update the widget representation from the nodes using the line interpolator
  for (int i = 1; i <= nPoints; i++)
    {
    this->UpdateLines(i);
    }
  this->BuildRepresentation();

  // Show the widget.
  this->VisibilityOn();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::BuildLocator()
{
  if ((!this->RebuildLocator && !this->NeedToRender) ||
       !this->MarkupsNode)
    {
    return;
    }

  int size = this->GetNumberOfNodes();
  if (size < 1)
    {
    return;
    }

  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(size);

  //setup up the matrixes needed to transform
  //world to display. We are going to do this manually
  // as calling the renderer will create a new matrix for each call
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  matrix->DeepCopy(this->Renderer->GetActiveCamera()
    ->GetCompositeProjectionTransformMatrix(
    this->Renderer->GetTiledAspectRatio(),0,1));

  //viewport info
  double viewPortRatio[2];
  int sizex,sizey;

  /* get physical window dimensions */
  if (this->Renderer->GetVTKWindow())
    {
    double *viewPort = this->Renderer->GetViewport();
    sizex = this->Renderer->GetVTKWindow()->GetSize()[0];
    sizey = this->Renderer->GetVTKWindow()->GetSize()[1];
    viewPortRatio[0] = (sizex * (viewPort[2] - viewPort[0])) / 2.0 +
        sizex*viewPort[0];
    viewPortRatio[1] = (sizey * (viewPort[3] - viewPort[1])) / 2.0 +
        sizey*viewPort[1];
    }
  else
    {
    //can't compute the locator without a vtk window
    return;
    }

  double view[4];
  double pos[3] = {0,0,0};
  double *wp;
  for (int i = 0; i < size; i++)
    {
    if (!this->MarkupsNode->ControlPointExists(i))
      {
      continue;
      }
    ControlPoint* node = this->GetNthNode(i);
    wp = node->WorldPosition.GetData();
    pos[0] = node->WorldPosition.GetX();
    pos[1] = node->WorldPosition.GetY();
    pos[2] = node->WorldPosition.GetZ();

    //convert from world to view
    view[0] = wp[0]*matrix->Element[0][0] + wp[1]*matrix->Element[0][1] +
      wp[2]*matrix->Element[0][2] + matrix->Element[0][3];
    view[1] = wp[0]*matrix->Element[1][0] + wp[1]*matrix->Element[1][1] +
      wp[2]*matrix->Element[1][2] + matrix->Element[1][3];
    view[2] = wp[0]*matrix->Element[2][0] + wp[1]*matrix->Element[2][1] +
      wp[2]*matrix->Element[2][2] + matrix->Element[2][3];
    view[3] = wp[0]*matrix->Element[3][0] + wp[1]*matrix->Element[3][1] +
      wp[2]*matrix->Element[3][2] + matrix->Element[3][3];
    if (view[3] != 0.0)
      {
      pos[0] = view[0] / view[3];
      pos[1] = view[1] / view[3];
      }

    //now from view to display
    pos[0] = (pos[0] + 1.0) * viewPortRatio[0];
    pos[1] = (pos[1] + 1.0) * viewPortRatio[1];
    pos[2] = 0;

    points->InsertPoint(i, pos);
    }

  matrix->Delete();
  vtkPolyData *tmp = vtkPolyData::New();
  tmp->SetPoints(points);
  this->Locator->SetDataSet(tmp);
  tmp->Delete();
  points->Delete();

  //we fully updated the display locator
  this->RebuildLocator = false;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkSlicerAbstractRepresentation::StartWidgetInteraction(double startEventPos[2])
{
  // How far is this in pixels from the position of this widget?
  // Maintain this during interaction such as translating (don't
  // force center of widget to snap to mouse position)

  // GetActiveNode position
  double pos[2];
  if (this->GetActiveNodeDisplayPosition(pos))
    {
    // save offset
    this->StartEventOffsetPosition[0] = startEventPos[0] - pos[0];
    this->StartEventOffsetPosition[1] = startEventPos[1] - pos[1];
    }
  else
    {
    this->StartEventOffsetPosition[0] = 0;
    this->StartEventOffsetPosition[1] = 0;
    }

  // save also the cursor pos
  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetInteractionState(int state)
{
  this->InteractionState = state;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetClosedLoop(vtkTypeBool val)
{
  if (this->ClosedLoop != val)
    {
    this->ClosedLoop = val;
    this->UpdateLines(this->GetNumberOfNodes() - 1);
    this->NeedToRender = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::ComputeCentroid(double *ioCentroid)
{
  double p[3];
  ioCentroid[0] = 0.;
  ioCentroid[1] = 0.;
  ioCentroid[2] = 0.;

  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    this->GetNthNodeWorldPosition(i, p);
    ioCentroid[0] += p[0];
    ioCentroid[1] += p[1];
    ioCentroid[2] += p[2];
  }
  double inv_N = 1. / static_cast< double >(this->GetNumberOfNodes());
  ioCentroid[0] *= inv_N;
  ioCentroid[1] *= inv_N;
  ioCentroid[2] *= inv_N;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetMarkupsNode(vtkMRMLMarkupsNode *markupNode)
{
  if (markupNode == nullptr || this->MarkupsNode == markupNode)
  {
    return;
  }

  this->MarkupsNode = markupNode;
}

//----------------------------------------------------------------------
vtkMRMLMarkupsNode *vtkSlicerAbstractRepresentation::GetMarkupsNode()
{
  return this->MarkupsNode;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetRenderer(vtkRenderer *ren)
{
  if ( ren == this->Renderer )
    {
    return;
    }

  // vtkPickingManager reduces perfomances, we don't use it
  this->UnRegisterPickers();
  this->Renderer = ren;
  // register with potentially new picker
  if (this->Renderer)
  {
    this->RegisterPickers();
  }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance <<"\n";
  os << indent << "Rebuild Locator: " <<
     (this->RebuildLocator ? "On" : "Off") << endl;

  os << indent << "Current Operation: ";
  if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Inactive)
  {
    os << "Inactive\n";
  }
  else
  {
    os << "Translate\n";
  }

  os << indent << "Line Interpolator: " << this->LineInterpolator << "\n";
  os << indent << "Point Placer: " << this->PointPlacer << "\n";

  os << indent << "Always On Top: "
     << (this->AlwaysOnTop ? "On\n" : "Off\n");
}
