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

#include "vtkSlicerCurveRepresentation.h"
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
#include "vtkBezierSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkVolumePicker.h"
#include "vtkPickingManager.h"

vtkStandardNewMacro(vtkSlicerCurveRepresentation);

//----------------------------------------------------------------------
vtkSlicerCurveRepresentation::vtkSlicerCurveRepresentation()
{
  vtkBezierSlicerLineInterpolator::SafeDownCast(this->LineInterpolator)
    ->SetMaximumCurveLineSegments(100);

  this->Lines = vtkPolyData::New();
  this->LinesMapper = vtkPolyDataMapper::New();
  this->LinesMapper->SetInputData(this->Lines);
  this->LinesMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->LinesMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
  this->LinesMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
  this->LinesMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->LinesActor = vtkActor::New();
  this->LinesActor->SetMapper(this->LinesMapper);
  this->LinesActor->SetProperty(this->LinesProperty);

  this->CursorPicker->AddPickList(this->LinesActor);

  this->centroidPoint = new vtkSlicerAbstractRepresentationCentroidPoint;
}

//----------------------------------------------------------------------
vtkSlicerCurveRepresentation::~vtkSlicerCurveRepresentation()
{
  this->Lines->Delete();
  this->LinesMapper->Delete();
  this->LinesActor->Delete();
  this->LinesProperty->Delete();
  this->ActiveLinesProperty->Delete();

  if (this->centroidPoint)
  {
    delete this->centroidPoint;
  }
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkSlicerCurveRepresentation::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Translate)
  {
    this->TranslateNode(eventPos);
  }
  else if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Shift)
  {
    this->ShiftWidget(eventPos);
  }
  else if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Scale)
  {
    this->ScaleWidget(eventPos);
  }
  else if (this->CurrentOperation == vtkSlicerAbstractRepresentation::Rotate)
  {
    this->RotateWidget(eventPos);
  }

  this->UpdateCentroidPoint();

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation::UpdateCentroidPoint()
{
  if ( !this->ClosedLoop && this->centroidPoint->Visibility )
  {
    this->centroidPoint->Visibility = false;
    this->NeedToRender = 1;
    return;
  }

  if ( !this->ClosedLoop || this->GetNumberOfNodes() < 2 )
  {
    return;
  }

  if ( !this->centroidPoint->Visibility )
  {
    this->centroidPoint->Visibility = true;
  }
  // Add a new point at centroid position
  double centroid[4];
  this->ComputeCentroid(centroid);

  this->centroidPoint->WorldPosition[0] = centroid[0];
  this->centroidPoint->WorldPosition[1] = centroid[1];
  this->centroidPoint->WorldPosition[2] = centroid[2];

  this->Renderer->SetWorldPoint(centroid[0],centroid[1],centroid[2], centroid[3]);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(centroid);

  this->centroidPoint->DisplayPosition[0] = centroid[0];
  this->centroidPoint->DisplayPosition[1] = centroid[1];

  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
int vtkSlicerCurveRepresentation::ActivateNode(double displayPos[2])
{
  if ( !InternalMarkup )
  {
    return 0;
  }

  this->BuildLocator();

  if (this->InteractionState != vtkSlicerAbstractRepresentation::Nearby)
  {
    if ( this->InternalMarkup->ActiveControl != -3)
    {
      this->InternalMarkup->ActiveControl = -3;
      this->NeedToRender = 1;
    }
    return ( this->InternalMarkup->ActiveControl >= 0 );
  }

  // Find closest node to this display pos that
  // is within PixelTolerance. PixelTolerance is calculated as
  // a percentage (this->Tolerance) of the size of the window diagonal
  double dPos[3] = {displayPos[0],displayPos[1],0};

  double *viewport = this->GetRenderer()->GetViewport();
  int winSize[2] = {1, 1};
  double x1, y1, x2, y2;

  if (this->GetRenderer()->GetRenderWindow())
  {
    int *winSizePtr = this->GetRenderer()->GetRenderWindow()->GetSize();
    if (winSizePtr)
    {
      winSize[0] = winSizePtr[0];
      winSize[1] = winSizePtr[1];
    }
  }
  x1 = winSize[0] * viewport[0];
  y1 = winSize[1] * viewport[1];

  x2 = winSize[0] * viewport[2];
  y2 = winSize[1] * viewport[3];

  this->PixelTolerance = sqrt ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))
                         * this->Tolerance * 10.;

  if ( this->centroidPoint->Visibility )
  {
    double dPosCentroid[2];

    this->Renderer->SetWorldPoint(this->centroidPoint->WorldPosition);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(this->centroidPoint->DisplayPosition);

    dPosCentroid[0] = this->centroidPoint->DisplayPosition[0];
    dPosCentroid[1] = this->centroidPoint->DisplayPosition[1];
    if ( fabs(displayPos[0] - dPosCentroid[0]) < this->PixelTolerance &&
         fabs(displayPos[1] - dPosCentroid[1]) < this->PixelTolerance)
    {
      if ( this->InternalMarkup->ActiveControl != -2)
      {
        this->InternalMarkup->ActiveControl = -2;
        this->NeedToRender = 1;
      }
      return ( this->InternalMarkup->ActiveControl >= 0 );
    }
  }

  double closestDistance2 = VTK_DOUBLE_MAX;
  int closestNode = this->Locator->FindClosestPointWithinRadius(
    this->PixelTolerance,dPos,closestDistance2);
  if ( closestNode != this->InternalMarkup->ActiveControl )
  {
    this->InternalMarkup->ActiveControl = closestNode;
    this->NeedToRender = 1;
  }
  return ( this->InternalMarkup->ActiveControl >= 0 );
}

//----------------------------------------------------------------------
int vtkSlicerCurveRepresentation::ActivateNode(int displayPos[2])
{
  double doubleDisplayPos[2];

  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->ActivateNode( doubleDisplayPos );
}

//----------------------------------------------------------------------
int vtkSlicerCurveRepresentation::ActivateNode(int X, int Y)
{
  double doubleDisplayPos[2];

  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->ActivateNode( doubleDisplayPos );
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetRepresentationToSurface();
  this->Property->SetAmbient( 0.1 );
  this->Property->SetDiffuse( 0.9 );
  this->Property->SetSpecular( 0.0 );
  this->Property->SetPointSize(10.);
  this->Property->SetLineWidth(2);
  this->Property->SetOpacity(1);

  this->ActiveProperty = vtkProperty::New();
  this->ActiveProperty->SetRepresentationToSurface();
  this->ActiveProperty->SetColor( 0.25, 0.75, 0.25 );
  this->ActiveProperty->SetAmbient( 0.1 );
  this->ActiveProperty->SetDiffuse( 0.9 );
  this->ActiveProperty->SetSpecular( 0.0 );
  this->ActiveProperty->SetOpacity(1);

  this->LinesProperty = vtkProperty::New();
  this->LinesProperty->SetRepresentationToSurface();
  this->LinesProperty->SetColor(1, 1, 1); // Set color to white
  this->LinesProperty->SetLineWidth(2.0);
  this->LinesProperty->SetAmbient( 0.1 );
  this->LinesProperty->SetDiffuse( 0.9 );
  this->LinesProperty->SetSpecular( 0.0 );
  this->LinesProperty->SetOpacity(1);

  this->ActiveLinesProperty = vtkProperty::New();
  this->ActiveLinesProperty->SetRepresentationToSurface();
  this->ActiveLinesProperty->SetLineWidth(2.0);
  this->ActiveLinesProperty->SetColor( 0.25, 0.75, 0.25 );
  this->ActiveLinesProperty->SetAmbient( 0.1 );
  this->ActiveLinesProperty->SetDiffuse( 0.9 );
  this->ActiveLinesProperty->SetSpecular( 0.0 );
  this->ActiveLinesProperty->SetOpacity(1);
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation::Highlight(int highlight)
{
  if ( !InternalMarkup )
  {
    return;
  }

  if ( highlight && this->InternalMarkup->ActiveControl == -1 &&
       this->InteractionState == vtkSlicerAbstractRepresentation::Nearby )
  {
    this->LinesActor->SetProperty(this->ActiveLinesProperty);
  }
  else
  {
    this->LinesActor->SetProperty(this->LinesProperty);
  }

  this->NeedToRenderOn();
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation::BuildLines()
{
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();

  int i, j;
  vtkIdType index = 0;

  int numberOfNodes = this->GetNumberOfNodes();
  int count = numberOfNodes;
  for (i = 0; i < numberOfNodes; i++)
  {
    count += this->GetNumberOfIntermediatePoints(i);
  }

  points->SetNumberOfPoints(count);
  vtkIdType numLines;

  if (this->ClosedLoop && count > 0)
  {
    numLines = count+1;
  }
  else
  {
    numLines = count;
  }

  if (numLines > 0)
  {
    vtkIdType *lineIndices = new vtkIdType[numLines];

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

    if (this->ClosedLoop)
    {
      lineIndices[index] = 0;
    }

    lines->InsertNextCell(numLines, lineIndices);
    delete [] lineIndices;
  }

  this->Lines->SetPoints(points);
  this->Lines->SetLines(lines);

  points->Delete();
  lines->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation::ComputeCentroid(double *ioCentroid)
{
  int NumberOfTotalPoints;
  vtkNew<vtkPoints> curvePoints;
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    double pos[3];
    this->GetNthNodeWorldPosition(i, pos);
    curvePoints->InsertNextPoint(pos);
    // take only one intermediate point every ten
    // in this way we avoid very nearby points
    for (int j = 10; j < this->GetNumberOfIntermediatePoints(i) / 10; j = j+10)
    {
      this->GetIntermediatePointWorldPosition(i, j, pos);
      curvePoints->InsertNextPoint(pos);
    }
  }

  NumberOfTotalPoints = curvePoints->GetNumberOfPoints();
  if (NumberOfTotalPoints == 0)
  {
    return;
  }

  // Samples the points along the polyline at equal distances
  double distanceFromLastSampledPoint = 0.;
  double previousCurvePoint[3];
  curvePoints->GetPoint(0, previousCurvePoint);

  double totalLength = 0.;
  for (int curvePointIndex = 1; curvePointIndex < NumberOfTotalPoints; curvePointIndex++)
  {
    double currentCurvePoint[3];
    curvePoints->GetPoint(curvePointIndex, currentCurvePoint);
    curvePoints->GetPoint(curvePointIndex - 1, previousCurvePoint);
    totalLength += vtkMath::Distance2BetweenPoints(currentCurvePoint, previousCurvePoint);
  }

  curvePoints->GetPoint(0, previousCurvePoint);
  double samplingDistance = totalLength / (NumberOfTotalPoints * 100);

  vtkNew<vtkPoints> sampledPoints;
  sampledPoints->InsertNextPoint(previousCurvePoint);

  for (int curvePointIndex = 1; curvePointIndex < NumberOfTotalPoints; curvePointIndex++)
  {
    double currentCurvePoint[3];
    curvePoints->GetPoint(curvePointIndex, currentCurvePoint);
    double segmentLength = vtkMath::Distance2BetweenPoints(currentCurvePoint, previousCurvePoint);
    if (fabs(segmentLength) < 1.e-8)
    {
      continue;
    }

    double remainingSegmentLength = distanceFromLastSampledPoint + segmentLength;
    if (remainingSegmentLength >= samplingDistance)
    {
      double segmentDirectionVector[3];
      vtkMath::Subtract(currentCurvePoint, previousCurvePoint, segmentDirectionVector);
      vtkMath::MultiplyScalar(segmentDirectionVector, 1./segmentLength);
      // distance of new sampled point from previous curve point
      double distanceFromLastInterpolatedPoint = samplingDistance - distanceFromLastSampledPoint;
      while (remainingSegmentLength >= samplingDistance)
      {
        double newSampledPoint[3], segmentShiftVector[3];
        for (int i = 0; i < 3; i++)
        {
          segmentShiftVector[i] = segmentDirectionVector[i];
        }
        vtkMath::MultiplyScalar(segmentShiftVector, distanceFromLastInterpolatedPoint);
        vtkMath::Add(previousCurvePoint, segmentShiftVector, newSampledPoint);
        sampledPoints->InsertNextPoint(newSampledPoint);
        distanceFromLastSampledPoint = 0;
        distanceFromLastInterpolatedPoint += samplingDistance;
        remainingSegmentLength -= samplingDistance;
      }
      distanceFromLastSampledPoint = remainingSegmentLength;
    }
    else
    {
      distanceFromLastSampledPoint += segmentLength;
    }
    for (int i = 0; i < 3; i++)
    {
      previousCurvePoint[i] = currentCurvePoint[i];
    }
  }

  double p[3];
  ioCentroid[0] = 0.;
  ioCentroid[1] = 0.;
  ioCentroid[2] = 0.;

  for (int i = 0; i < sampledPoints->GetNumberOfPoints(); i++)
  {
    sampledPoints->GetPoint(i, p);
    ioCentroid[0] += p[0];
    ioCentroid[1] += p[1];
    ioCentroid[2] += p[2];
  }

  double inv_N = 1. / static_cast< double >(sampledPoints->GetNumberOfPoints());
  ioCentroid[0] *= inv_N;
  ioCentroid[1] *= inv_N;
  ioCentroid[2] *= inv_N;

  curvePoints->Squeeze();
  sampledPoints->Squeeze();
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerCurveRepresentation::GetWidgetRepresentationAsPolyData()
{
  return this->Lines;
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation::BuildRepresentation()
{
  if ( !InternalMarkup )
  {
    return;
  }

  // Make sure we are up to date with any changes made in the placer
  this->UpdateWidget();

  if (this->AlwaysOnTop)
  {
    // max value 65536 so we subtract 66000 to make sure we are
    // zero or negative
    this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(0,-66000);
    this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0,-66000);
    this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0,-66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0,-66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->LinesMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0,-66000);
    this->LinesMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0,-66000);
    this->LinesMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
  }
  else
  {
    this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
    this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
    this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
    this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
    this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->LinesMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
    this->LinesMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
    this->LinesMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
  }

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

  double distance =
    sqrt(vtkMath::Distance2BetweenPoints(p1, p2));

  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double viewport[4];
  this->Renderer->GetViewport(viewport);

  double x, y, scale;

  x = size[0] * (viewport[2]-viewport[0]);
  y = size[1] * (viewport[3]-viewport[1]);

  scale = sqrt(x*x + y*y);


  distance = 1000 * distance / scale;

  this->Glypher->SetScaleFactor(distance * this->HandleSize);
  this->ActiveGlypher->SetScaleFactor(distance * this->HandleSize);
  int numPoints = this->GetNumberOfNodes();
  int i;
  if (this->InternalMarkup->ActiveControl >= 0 &&
    this->InternalMarkup->ActiveControl < this->GetNumberOfNodes())
  {
    this->FocalPoint->SetNumberOfPoints(numPoints-1);
    this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(numPoints-1);
  }
  else
  {
    this->FocalPoint->SetNumberOfPoints(numPoints);
    this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(numPoints);
  }
  int idx = 0;
  for (i = 0; i < numPoints; i++)
  {
    if (i != this->InternalMarkup->ActiveControl)
    {
      double worldPos[3];
      double worldOrient[9] = {1.0,0.0,0.0,
                               0.0,1.0,0.0,
                               0.0,0.0,1.0};
      this->GetNthNodeWorldPosition(i, worldPos);
      this->FocalPoint->SetPoint(idx, worldPos);
      this->FocalData->GetPointData()->GetNormals()->SetTuple(idx, worldOrient+6);
      idx++;
    }
  }
  if (this->InternalMarkup->ActiveControl != -2 && this->centroidPoint->Visibility)
  {
    this->FocalPoint->InsertNextPoint(this->centroidPoint->WorldPosition);
    double worldOrient[9] = {1.0,0.0,0.0,
                             0.0,1.0,0.0,
                             0.0,0.0,1.0};
    this->FocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient+6);
  }

  this->FocalPoint->Modified();
  this->FocalData->GetPointData()->GetNormals()->Modified();
  this->FocalData->Modified();

  if (this->InternalMarkup->ActiveControl >= 0 &&
       this->InternalMarkup->ActiveControl < this->GetNumberOfNodes())
  {
    double worldPos[3];
    double worldOrient[9] = {1.0,0.0,0.0,
                             0.0,1.0,0.0,
                             0.0,0.0,1.0};
    this->GetNthNodeWorldPosition(this->InternalMarkup->ActiveControl, worldPos);
    this->ActiveFocalPoint->SetPoint(0, worldPos);
    this->ActiveFocalData->GetPointData()->GetNormals()->SetTuple(0, worldOrient+6);

    this->ActiveFocalPoint->Modified();
    this->ActiveFocalData->GetPointData()->GetNormals()->Modified();
    this->ActiveFocalData->Modified();
    this->ActiveActor->VisibilityOn();
  }
  else if (this->InternalMarkup->ActiveControl == -2 && this->centroidPoint->Visibility)
  {
    this->ActiveFocalPoint->SetPoint(0, this->centroidPoint->WorldPosition);

    this->ActiveFocalPoint->Modified();
    this->ActiveFocalData->GetPointData()->GetNormals()->Modified();
    this->ActiveFocalData->Modified();
    this->ActiveActor->VisibilityOn();
  }
  else
  {
    this->ActiveActor->VisibilityOff();
  }
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
  this->ActiveActor->GetActors(pc);
  this->LinesActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->ActiveActor->ReleaseGraphicsResources(win);
  this->LinesActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerCurveRepresentation::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  if (this->Actor->GetVisibility())
  {
    count +=  this->Actor->RenderOverlay(viewport);
  }
  if (this->ActiveActor->GetVisibility())
  {
    count +=  this->ActiveActor->RenderOverlay(viewport);
  }
  if (this->LinesActor->GetVisibility())
  {
    count +=  this->LinesActor->RenderOverlay(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerCurveRepresentation::RenderOpaqueGeometry(
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
  if (this->ActiveActor->GetVisibility())
  {
    count += this->ActiveActor->RenderOpaqueGeometry(viewport);
  }
  if (this->LinesActor->GetVisibility())
  {
    count += this->LinesActor->RenderOpaqueGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerCurveRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  if (this->Actor->GetVisibility())
  {
    count += this->Actor->RenderTranslucentPolygonalGeometry(viewport);
  }
  if (this->ActiveActor->GetVisibility())
  {
    count += this->ActiveActor->RenderTranslucentPolygonalGeometry(viewport);
  }
  if (this->LinesActor->GetVisibility())
  {
    count += this->LinesActor->RenderTranslucentPolygonalGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerCurveRepresentation::HasTranslucentPolygonalGeometry()
{  
  int result=0;
  if (this->Actor->GetVisibility())
  {
    result |= this->Actor->HasTranslucentPolygonalGeometry();
  }
  if (this->ActiveActor->GetVisibility())
  {
    result |= this->ActiveActor->HasTranslucentPolygonalGeometry();
  }
  if (this->LinesActor->GetVisibility())
  {
    result |= this->LinesActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

//----------------------------------------------------------------------------
void vtkSlicerCurveRepresentation::SetLineColor(
  double r, double g, double b)
{
  if (this->GetLinesProperty())
  {
    this->GetLinesProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------
double *vtkSlicerCurveRepresentation::GetBounds()
{
  return this->Lines->GetPoints() ?
              this->Lines->GetPoints()->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
void vtkSlicerCurveRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Closed Loop: " << (this->ClosedLoop ? "On\n" : "Off\n");

  os << indent << "Line Visibility: " << this->LinesActor->GetVisibility() << "\n";

  if (this->LinesProperty)
  {
    os << indent << "Lines Property: " << this->LinesProperty << "\n";
  }
  else
  {
    os << indent << "Lines Property: (none)\n";
  }

  if (this->ActiveLinesProperty)
  {
    os << indent << "Active Lines Property: " << this->ActiveLinesProperty << "\n";
  }
  else
  {
    os << indent << "Active Lines Property: (none)\n";
  }

}

