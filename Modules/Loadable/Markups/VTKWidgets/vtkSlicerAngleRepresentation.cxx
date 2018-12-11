/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerAngleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSlicerAngleRepresentation.h"
#include "vtkCleanPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkArcSource.h"
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
#include "vtkLinearSlicerLineInterpolator.h"
#include "vtkFollower.h"
#include "vtkSphereSource.h"
#include "vtkVectorText.h"
#include "vtkVolumePicker.h"
#include "vtkPickingManager.h"
#include <cmath>

vtkStandardNewMacro(vtkSlicerAngleRepresentation);

//----------------------------------------------------------------------
vtkSlicerAngleRepresentation::vtkSlicerAngleRepresentation()
{
  this->LineInterpolator = vtkLinearSlicerLineInterpolator::New();

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

  this->ArcSource = vtkArcSource::New();
  this->ArcSource->SetResolution(30);
  this->ArcMapper = vtkPolyDataMapper::New();
  this->ArcMapper->SetInputConnection(
    this->ArcSource->GetOutputPort());
  this->ArcActor = vtkActor::New();
  this->ArcActor->SetMapper(this->ArcMapper);
  this->ArcActor->SetProperty(this->LinesProperty);

  this->Angle = 0.0;
  this->TextInput = vtkVectorText::New();
  this->TextInput->SetText( "0" );
  this->TextMapper = vtkPolyDataMapper::New();
  this->TextMapper->SetInputConnection(
    this->TextInput->GetOutputPort());
  this->TextActor = vtkFollower::New();
  this->TextActor->SetMapper(this->TextMapper);
  this->TextActor->GetProperty()->SetColor( 1.0, 0.1, 0.0 );
  this->ScaleInitialized = false;

  this->LabelFormat = new char[8];
  snprintf(this->LabelFormat,8,"%s","%-#6.3g");

  this->CursorPicker->AddPickList(this->LinesActor);
  this->CursorPicker->AddPickList(this->ArcActor);
}

//----------------------------------------------------------------------
vtkSlicerAngleRepresentation::~vtkSlicerAngleRepresentation()
{
  this->Lines->Delete();
  this->LinesMapper->Delete();
  this->LinesActor->Delete();

  this->ArcSource->Delete();
  this->ArcMapper->Delete();
  this->ArcActor->Delete();

  this->TextInput->Delete();
  this->TextMapper->Delete();
  this->TextActor->Delete();

  delete [] this->LabelFormat;
  this->LabelFormat = nullptr;

  this->LinesProperty->Delete();
  this->ActiveLinesProperty->Delete();

}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation::CreateDefaultProperties()
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
void vtkSlicerAngleRepresentation::Highlight(int highlight)
{
  if ( !InternalMarkup )
  {
    return;
  }

  if ( highlight && this->InternalMarkup->ActiveControl == -1 &&
       this->InteractionState == vtkSlicerAbstractRepresentation::Nearby )
  {
    this->LinesActor->SetProperty(this->ActiveLinesProperty);
    this->ArcActor->SetProperty(this->ActiveLinesProperty);
  }
  else
  {
    this->LinesActor->SetProperty(this->LinesProperty);
    this->ArcActor->SetProperty(this->LinesProperty);
  }

  this->NeedToRenderOn();
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation::BuildLines()
{
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();

  int i, j;
  vtkIdType index = 0;

  int count = this->GetNumberOfNodes();
  for (i = 0; i < this->GetNumberOfNodes(); i++)
  {
    count += this->GetNumberOfIntermediatePoints(i);
  }

  points->SetNumberOfPoints(count);
  vtkIdType numLines = count;

  if (numLines > 0)
  {
    vtkIdType *lineIndices = new vtkIdType[numLines];

    double pos[3];
    for (i = 0; i < this->GetNumberOfNodes(); i++)
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

    lines->InsertNextCell(numLines, lineIndices);
    delete [] lineIndices;
  }

  this->Lines->SetPoints(points);
  this->Lines->SetLines(lines);

  points->Delete();
  lines->Delete();

  // Build Arc
  if ( this->GetNumberOfNodes() != 3 ||
       !this->LinesActor->GetVisibility() ||
       !this->Renderer)
  {
    this->ArcActor->SetVisibility(0);
    this->TextActor->SetVisibility(0);
    return;
  }

  double p1[3], p2[3], c[3], vector2[3], vector1[3];
  double l1 = 0.0, l2 = 0.0;
  this->GetNthNodeWorldPosition(0, p1);
  this->GetNthNodeWorldPosition(1, c);
  this->GetNthNodeWorldPosition(2, p2);

  // Compute the angle (only if necessary since we don't want
  // fluctuations in angle value as the camera moves, etc.)
  if ( fabs(p1[0]-c[0]) < 0.001 || fabs(p2[0]-c[0]) < 0.001 )
  {
    return;
  }

  vector1[0] = p1[0] - c[0];
  vector1[1] = p1[1] - c[1];
  vector1[2] = p1[2] - c[2];
  vector2[0] = p2[0] - c[0];
  vector2[1] = p2[1] - c[1];
  vector2[2] = p2[2] - c[2];
  l1 = vtkMath::Normalize( vector1 );
  l2 = vtkMath::Normalize( vector2 );
  this->Angle = acos( vtkMath::Dot( vector1, vector2 ) );

  // Place the label and place the arc
  const double length = l1 < l2 ? l1 : l2;
  const double anglePlacementRatio = 0.5;
  const double l = length * anglePlacementRatio;
  double arcp1[3] = { l * vector1[0] + c[0],
                      l * vector1[1] + c[1],
                      l * vector1[2] + c[2] };
  double arcp2[3] = { l * vector2[0] + c[0],
                      l * vector2[1] + c[1],
                      l * vector2[2] + c[2] };
  this->ArcSource->SetPoint1( arcp1 );
  this->ArcSource->SetPoint2( arcp2 );
  this->ArcSource->SetCenter( c );
  this->ArcSource->Update();

  vtkPoints *arcPoints = this->ArcSource->GetOutput()->GetPoints();
  const int npoints = arcPoints->GetNumberOfPoints();
  arcPoints->GetPoint(npoints/2, this->TextPosition );

  char string[512];
  snprintf( string, sizeof(string), this->LabelFormat, vtkMath::DegreesFromRadians( this->Angle ) );

  this->TextInput->SetText( string );
  this->TextActor->SetCamera( this->Renderer->GetActiveCamera() );
  this->TextActor->SetPosition( this->TextPosition );

  if (!this->ScaleInitialized)
  {
    // If a font size hasn't been specified by the user, scale the text
    // (font size) according to the length of the shortest arm of the
    // angle measurement.
    this->TextActor->SetScale( length/10.0, length/10.0, length/10.0 );
  }

  this->ArcActor->SetVisibility(1);
  this->TextActor->SetVisibility(1);
}

//-----------------------------------------------------------------------------
void vtkSlicerAngleRepresentation::WidgetInteraction(double eventPos[2])
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

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation::RotateWidget(double eventPos[])
{
  double ref[3] = {0.};
  double displayPos[2] = {0.};
  double worldPos[3] = {0.};
  double lastWorldPos[3] = {0.};
  double worldOrient[9] = {1.0, 0.0, 0.0,
                           0.0, 1.0, 0.0,
                           0.0, 0.0, 1.0};

  displayPos[0] = this->LastEventPosition[0];
  displayPos[1] = this->LastEventPosition[1];

  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, lastWorldPos,
                                               worldOrient))
  {
    ref[0] = lastWorldPos[0];
    ref[1] = lastWorldPos[1];
    ref[2] = lastWorldPos[2];
  }
  else
  {
    return;
  }

  displayPos[0] = eventPos[0];
  displayPos[1] = eventPos[1];

  double AnglePoint[3];
  this->GetNthNodeWorldPosition(1, AnglePoint);

  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient))
  {
    double d2 = vtkMath::Distance2BetweenPoints(worldPos, AnglePoint);
    if (d2 != 0.0)
    {
      for (int i = 0; i < 3; i++)
      {
        lastWorldPos[i] -= AnglePoint[i];
        worldPos[i] -= AnglePoint[i];
      }
      double angle = -vtkMath::DegreesFromRadians
                     ( vtkMath::AngleBetweenVectors( lastWorldPos, worldPos ) );

      for (int i = 0; i < this->GetNumberOfNodes(); i++)
      {
        if (i == 1)
        {
          continue;
        }
        this->GetNthNodeWorldPosition(i, ref);
        for (int i = 0; i < 3; i++)
        {
          ref[i] -= AnglePoint[i];
        }
        vtkNew<vtkTransform> RotateTransform;
        RotateTransform->RotateZ(angle);
        RotateTransform->TransformPoint(ref, worldPos);

        for (int i = 0; i < 3; i++)
        {
          worldPos[i] += AnglePoint[i];
        }
        this->SetNthNodeWorldPosition(i, worldPos);
      }
    }
  }
  else
  {
    return;
  }
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerAngleRepresentation::GetWidgetRepresentationAsPolyData()
{
  return this->Lines;
}


//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
  this->ActiveActor->GetActors(pc);
  this->LinesActor->GetActors(pc);
  this->ArcActor->GetActors(pc);
  this->TextActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerAngleRepresentation::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->ActiveActor->ReleaseGraphicsResources(win);
  this->LinesActor->ReleaseGraphicsResources(win);
  this->ArcActor->ReleaseGraphicsResources(win);
  this->TextActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerAngleRepresentation::RenderOverlay(vtkViewport *viewport)
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
int vtkSlicerAngleRepresentation::RenderOpaqueGeometry(
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
int vtkSlicerAngleRepresentation::RenderTranslucentPolygonalGeometry(
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
vtkTypeBool vtkSlicerAngleRepresentation::HasTranslucentPolygonalGeometry()
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

//----------------------------------------------------------------------------
double vtkSlicerAngleRepresentation::GetAngle()
{
  return this->Angle;
}

//----------------------------------------------------------------------------
void vtkSlicerAngleRepresentation::SetTextActorScale(double scale[])
{
  this->TextActor->SetScale( scale );
  this->ScaleInitialized = true;
}

//----------------------------------------------------------------------------
double *vtkSlicerAngleRepresentation::GetTextActorScale()
{
  return this->TextActor->GetScale();
}

//----------------------------------------------------------------------------
void vtkSlicerAngleRepresentation::SetLineColor(
  double r, double g, double b)
{
  if (this->GetLinesProperty())
  {
    this->GetLinesProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------
double *vtkSlicerAngleRepresentation::GetBounds()
{
  return this->Lines->GetPoints() ?
              this->Lines->GetPoints()->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
void vtkSlicerAngleRepresentation::BuildRepresentation()
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
    this->ArcMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0,-66000);
    this->ArcMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0,-66000);
    this->ArcMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->TextMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0,-66000);
    this->TextMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0,-66000);
    this->TextMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
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
    this->ArcMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
    this->ArcMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
    this->ArcMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->TextMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
    this->TextMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
    this->TextMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
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
  else
  {
    this->ActiveActor->VisibilityOff();
  }
}

//-----------------------------------------------------------------------------
void vtkSlicerAngleRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Lines Visibility: " << this->LinesActor->GetVisibility() << "\n";
  os << indent << "Arc Visibility: " << this->ArcActor->GetVisibility() << "\n";
  os << indent << "Arngle Text Visibility: " << this->TextActor->GetVisibility() << "\n";

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
