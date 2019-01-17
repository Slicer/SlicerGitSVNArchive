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
#include "vtkSlicerAbstractRepresentation3D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLActor.h"
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
#include "vtkBezierSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkPickingManager.h"
#include "vtkBox.h"
#include "vtkIntArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointPlacer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWindow.h"
#include "vtkProperty.h"
#include "vtkTextProperty.h"
#include "vtkCellPicker.h"
#include "vtkActor2D.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkStringArray.h"
#include "vtkLabelHierarchy.h"
#include "vtkMRMLMarkupsDisplayNode.h"

#include <set>
#include <algorithm>
#include <iterator>

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation3D::vtkSlicerAbstractRepresentation3D()
{
  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Mapper = vtkOpenGLPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());
  // This turns on resolve coincident topology for everything
  // as it is a class static on the mapper
  this->Mapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->Mapper->ScalarVisibilityOff();
  // Put this on top of other objects
  this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
  this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
  this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->SelectedMapper = vtkOpenGLPolyDataMapper::New();
  this->SelectedMapper->SetInputConnection(this->SelectedGlypher->GetOutputPort());
  // This turns on resolve coincident topology for everything
  // as it is a class static on the mapper
  this->SelectedMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->SelectedMapper->ScalarVisibilityOff();
  // Put this on top of other objects
  this->SelectedMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
  this->SelectedMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
  this->SelectedMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->ActiveMapper = vtkOpenGLPolyDataMapper::New();
  this->ActiveMapper->SetInputConnection(this->ActiveGlypher->GetOutputPort());
  // This turns on resolve coincident topology for everything
  // as it is a class static on the mapper
  this->ActiveMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->ActiveMapper->ScalarVisibilityOff();
  // Put this on top of other objects
  this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
  this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
  this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->Actor = vtkOpenGLActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  this->SelectedActor = vtkOpenGLActor::New();
  this->SelectedActor->SetMapper(this->SelectedMapper);
  this->SelectedActor->SetProperty(this->SelectedProperty);

  this->ActiveActor = vtkOpenGLActor::New();
  this->ActiveActor->SetMapper(this->ActiveMapper);
  this->ActiveActor->SetProperty(this->ActiveProperty);

  //Manage the picking stuff
  this->CursorPicker = vtkCellPicker::New();
  this->CursorPicker->PickFromListOn();
  this->CursorPicker->SetTolerance(this->Tolerance);
  this->CursorPicker->AddPickList(this->Actor);
  this->CursorPicker->AddPickList(this->SelectedActor);
  this->CursorPicker->AddPickList(this->ActiveActor);

  // Labels
  this->Labels = vtkStringArray::New();
  this->Labels->SetName( "labels" );
  this->Labels->SetNumberOfValues( 100 );
  this->Labels->SetNumberOfValues( 1 );
  this->Labels->SetValue( 0, "F" );
  this->LabelsPriority = vtkStringArray::New();
  this->LabelsPriority->SetName("priority");
  this->LabelsPriority->SetNumberOfValues( 100 );
  this->LabelsPriority->SetNumberOfValues( 1 );
  this->LabelsPriority->SetValue( 0, "1" );
  this->FocalData->GetPointData()->AddArray( this->Labels );
  this->FocalData->GetPointData()->AddArray( this->LabelsPriority );
  this->PointSetToLabelHierarchyFilter = vtkPointSetToLabelHierarchy::New();
  this->PointSetToLabelHierarchyFilter->SetTextProperty( this->TextProperty );
  this->PointSetToLabelHierarchyFilter->SetLabelArrayName( "labels" );
  this->PointSetToLabelHierarchyFilter->SetPriorityArrayName( "priority" );
  this->PointSetToLabelHierarchyFilter->SetInputData( this->FocalData );
  this->LabelsMapper = vtkLabelPlacementMapper::New();
  this->LabelsMapper->SetInputConnection( this->PointSetToLabelHierarchyFilter->GetOutputPort() );
  this->LabelsActor = vtkActor2D::New();
  this->LabelsActor->SetMapper( this->LabelsMapper );

  this->SelectedLabels = vtkStringArray::New();
  this->SelectedLabels->SetName( "labels" );
  this->SelectedLabels->SetNumberOfValues( 100 );
  this->SelectedLabels->SetNumberOfValues( 1 );
  this->SelectedLabels->SetValue( 0, "F" );
  this->SelectedLabelsPriority = vtkStringArray::New();
  this->SelectedLabelsPriority->SetName( "priority" );
  this->SelectedLabelsPriority->SetNumberOfValues( 100 );
  this->SelectedLabelsPriority->SetNumberOfValues( 1 );
  this->SelectedLabelsPriority->SetValue( 0, "1" );
  this->SelectedFocalData->GetPointData()->AddArray( this->SelectedLabels );
  this->SelectedFocalData->GetPointData()->AddArray( this->SelectedLabelsPriority );
  this->SelectedPointSetToLabelHierarchyFilter = vtkPointSetToLabelHierarchy::New();
  this->SelectedPointSetToLabelHierarchyFilter->SetTextProperty( this->SelectedTextProperty );
  this->SelectedPointSetToLabelHierarchyFilter->SetLabelArrayName( "labels" );
  this->SelectedPointSetToLabelHierarchyFilter->SetPriorityArrayName( "priority" );
  this->SelectedPointSetToLabelHierarchyFilter->SetInputData( this->SelectedFocalData );
  this->SelectedLabelsMapper = vtkLabelPlacementMapper::New();
  this->SelectedLabelsMapper->SetInputConnection( this->SelectedPointSetToLabelHierarchyFilter->GetOutputPort() );
  this->SelectedLabelsActor = vtkActor2D::New();
  this->SelectedLabelsActor->SetMapper( this->SelectedLabelsMapper );

  this->ActiveLabels = vtkStringArray::New();
  this->ActiveLabels->SetName( "labels" );
  this->ActiveLabels->SetNumberOfValues( 100 );
  this->ActiveLabels->SetNumberOfValues( 1 );
  this->ActiveLabels->SetValue( 0, "F" );
  this->ActiveLabelsPriority = vtkStringArray::New();
  this->ActiveLabelsPriority->SetName( "priority" );
  this->ActiveLabelsPriority->SetNumberOfValues( 100 );
  this->ActiveLabelsPriority->SetNumberOfValues( 1 );
  this->ActiveLabelsPriority->SetValue( 0, "1" );
  this->ActiveFocalData->GetPointData()->AddArray( this->ActiveLabels );
  this->ActiveFocalData->GetPointData()->AddArray( this->ActiveLabelsPriority );
  this->ActivePointSetToLabelHierarchyFilter = vtkPointSetToLabelHierarchy::New();
  this->ActivePointSetToLabelHierarchyFilter->SetTextProperty( this->ActiveTextProperty );
  this->ActivePointSetToLabelHierarchyFilter->SetLabelArrayName( "labels" );
  this->ActivePointSetToLabelHierarchyFilter->SetPriorityArrayName( "priority" );
  this->ActivePointSetToLabelHierarchyFilter->SetInputData( this->ActiveFocalData );
  this->ActiveLabelsMapper = vtkLabelPlacementMapper::New();
  this->ActiveLabelsMapper->SetInputConnection( this->ActivePointSetToLabelHierarchyFilter->GetOutputPort() );
  this->ActiveLabelsActor = vtkActor2D::New();
  this->ActiveLabelsActor->SetMapper( this->ActiveLabelsMapper );

  this->HandleSize = 3;
}

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation3D::~vtkSlicerAbstractRepresentation3D()
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

  this->CursorPicker->Delete();

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
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::CreateDefaultProperties()
{
  this->Glypher = vtkGlyph3D::New();
  this->Glypher->SetInputData(this->FocalData);
  this->Glypher->SetVectorModeToUseNormal();
  this->Glypher->OrientOn();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  this->SelectedGlypher = vtkGlyph3D::New();
  this->SelectedGlypher->SetInputData(this->SelectedFocalData);
  this->SelectedGlypher->SetVectorModeToUseNormal();
  this->SelectedGlypher->OrientOn();
  this->SelectedGlypher->ScalingOn();
  this->SelectedGlypher->SetScaleModeToDataScalingOff();
  this->SelectedGlypher->SetScaleFactor(1.0);

  this->ActiveGlypher = vtkGlyph3D::New();
  this->ActiveGlypher->SetInputData(this->ActiveFocalData);
  this->ActiveGlypher->SetVectorModeToUseNormal();
  this->ActiveGlypher->OrientOn();
  this->ActiveGlypher->ScalingOn();
  this->ActiveGlypher->SetScaleModeToDataScalingOff();
  this->ActiveGlypher->SetScaleFactor(1.0);

  // By default the Points are rendered as spheres
  vtkNew<vtkSphereSource> ss;
  ss->SetRadius( 0.5 );
  ss->Update();
  this->SetCursorShape( ss->GetOutput() );
  this->SetSelectedCursorShape( ss->GetOutput() );
  this->SetActiveCursorShape( ss->GetOutput() );

  this->Property = vtkProperty::New();
  this->Property->SetRepresentationToSurface();
  this->Property->SetColor( 0.4, 1.0, 1.0 );
  this->Property->SetAmbient( 0.0 );
  this->Property->SetDiffuse( 1.0 );
  this->Property->SetSpecular( 0.0 );
  this->Property->SetShading( true );
  this->Property->SetSpecularPower( 1.0 );
  this->Property->SetPointSize( 10. );
  this->Property->SetLineWidth( 2. );
  this->Property->SetOpacity( 1. );

  this->SelectedProperty = vtkProperty::New();
  this->SelectedProperty->SetRepresentationToSurface();
  this->SelectedProperty->SetColor( 1.0, 0.5, 0.5 );
  this->SelectedProperty->SetAmbient( 0.0 );
  this->SelectedProperty->SetDiffuse( 1.0 );
  this->SelectedProperty->SetSpecular( 0.0 );
  this->SelectedProperty->SetShading( true );
  this->SelectedProperty->SetSpecularPower( 1.0 );
  this->SelectedProperty->SetPointSize( 10. );
  this->SelectedProperty->SetLineWidth( 2. );
  this->SelectedProperty->SetOpacity( 1. );

  this->ActiveProperty = vtkProperty::New();
  this->ActiveProperty->SetRepresentationToSurface();
  // bright green
  this->ActiveProperty->SetColor( 0.4, 1.0, 0. );
  this->ActiveProperty->SetAmbient( 0. );
  this->ActiveProperty->SetDiffuse( 1.0 );
  this->ActiveProperty->SetSpecular( 0. );
  this->ActiveProperty->SetShading( true );
  this->ActiveProperty->SetSpecularPower( 1.0 );
  this->ActiveProperty->SetPointSize( 10. );
  this->ActiveProperty->SetLineWidth( 2. );
  this->ActiveProperty->SetOpacity( 1. );

  this->TextProperty = vtkTextProperty::New();
  this->TextProperty->SetJustificationToRight();
  this->TextProperty->SetFontSize( 15 );
  this->TextProperty->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  this->TextProperty->SetColor( 0.4, 1.0, 1.0 );
  this->TextProperty->SetOpacity( 1. );

  this->SelectedTextProperty = vtkTextProperty::New();
  this->SelectedTextProperty->SetJustificationToRight();
  this->SelectedTextProperty->SetFontSize( 15 );
  this->SelectedTextProperty->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  this->SelectedTextProperty->SetColor( 1.0, 0.5, 0.5 );
  this->SelectedTextProperty->SetOpacity( 1. );

  this->ActiveTextProperty = vtkTextProperty::New();
  this->ActiveTextProperty->SetJustificationToRight();
  this->ActiveTextProperty->SetFontSize( 15 );
  this->ActiveTextProperty->SetFontFamily( vtkTextProperty::GetFontFamilyFromString( "Arial" ) );
  // bright green
  this->ActiveTextProperty->SetColor( 0.4, 1.0, 0. );
  this->ActiveTextProperty->SetOpacity( 1. );
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::SetCursorShape(vtkPolyData *shape)
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
vtkPolyData *vtkSlicerAbstractRepresentation3D::GetCursorShape()
{
  return this->CursorShape;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::SetSelectedCursorShape(vtkPolyData *shape)
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
vtkPolyData *vtkSlicerAbstractRepresentation3D::GetSelectedCursorShape()
{
  return this->SelectedCursorShape;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::SetActiveCursorShape(
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
vtkPolyData *vtkSlicerAbstractRepresentation3D::GetActiveCursorShape()
{
  return this->ActiveCursorShape;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::SetTolerance(double tol)
{
  this->Tolerance = tol;
  this->CursorPicker->SetTolerance(this->Tolerance);
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if ( !pm )
    {
    return;
    }
  pm->AddPicker( this->CursorPicker, this );
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  this->UpdateWidget();

  if ( this->MarkupsNode == nullptr )
    {
    return;
    }

  vtkMRMLMarkupsDisplayNode* display = vtkMRMLMarkupsDisplayNode::SafeDownCast
    ( this->MarkupsNode->GetDisplayNode() );
  if ( display == nullptr )
    {
    return;
    }

  if ( display->GetTextVisibility() )
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
    this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters( 0, -66000 );
    this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters( 0, -66000 );
    this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter( -66000 );
    this->SelectedMapper->SetRelativeCoincidentTopologyLineOffsetParameters( 0, -66000 );
    this->SelectedMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters( 0, -66000 );
    this->SelectedMapper->SetRelativeCoincidentTopologyPointOffsetParameter( -66000 );
    this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters( 0, -66000 );
    this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters( 0, -66000 );
    this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter( -66000 );
    }
  else
    {
    this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters( -1, -1 );
    this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters( -1, -1 );
    this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter( -1 );
    this->SelectedMapper->SetRelativeCoincidentTopologyLineOffsetParameters( -1, -1 );
    this->SelectedMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters( -1, -1 );
    this->SelectedMapper->SetRelativeCoincidentTopologyPointOffsetParameter( -1 );
    this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters( -1, -1 );
    this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters( -1, -1 );
    this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter( -1 );
    }

  double p1[4], p2[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint( p1 );
  p1[3] = 1.0;
  this->Renderer->SetWorldPoint( p1 );
  this->Renderer->WorldToView();
  this->Renderer->GetViewPoint( p1 );

  double depth = p1[2];
  double aspect[2];
  this->Renderer->ComputeAspect();
  this->Renderer->GetAspect( aspect );

  p1[0] = -aspect[0];
  p1[1] = -aspect[1];
  this->Renderer->SetViewPoint( p1 );
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint( p1 );

  p2[0] = aspect[0];
  p2[1] = aspect[1];
  p2[2] = depth;
  p2[3] = 1.0;
  this->Renderer->SetViewPoint( p2 );
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint( p2 );

  double distance = sqrt( vtkMath::Distance2BetweenPoints( p1, p2 ) );

  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double viewport[4];
  this->Renderer->GetViewport(viewport);

  double x, y, scale;

  x = size[0] * ( viewport[2] - viewport[0] );
  y = size[1] * ( viewport[3] - viewport[1] );

  scale = sqrt( x * x + y * y );
  distance = scale / distance;

  this->Glypher->SetScaleFactor( distance * this->HandleSize );
  this->SelectedGlypher->SetScaleFactor( distance * this->HandleSize );
  this->ActiveGlypher->SetScaleFactor( distance * this->HandleSize );

  int numPoints = this->GetNumberOfNodes();
  int ii;

  this->FocalPoint->SetNumberOfPoints( 0 );
  this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples( 0 );
  this->Labels->SetNumberOfValues( 0 );
  this->LabelsPriority->SetNumberOfValues( 0 );

  for ( ii = 0; ii < numPoints; ii++ )
    {
    if ( !this->GetNthNodeVisibility( ii ) ||
         ii == this->GetActiveNode() ||
         this->GetNthNodeSelected( ii ) )
      {
      continue;
      }

    double worldPos[3], worldOrient[9] = {0}, orientation[4] = {0};
    this->GetNthNodeWorldPosition( ii, worldPos );
    bool skipPoint = false;
    for (int jj = 0; jj < this->FocalPoint->GetNumberOfPoints(); jj++ )
      {
      double* pos = this->FocalPoint->GetPoint(jj);
      double eps = 0.001;
      if ( fabs(pos[0] - worldPos[0]) < eps &&
           fabs(pos[1] - worldPos[1]) < eps &&
           fabs(pos[2] - worldPos[2]) < eps )
        {
        skipPoint = true;
        break;
        }
      }

    if ( skipPoint )
      {
      continue;
      }

    this->FocalPoint->InsertNextPoint( worldPos );
    this->GetNthNodeOrientation( ii, orientation );
    this->FromOrientationQuaternionToWorldOrient( orientation, worldOrient );
    this->FocalData->GetPointData()->GetNormals()->InsertNextTuple( worldOrient + 6 );
    this->Labels->InsertNextValue( this->GetNthNodeLabel( ii ) );
    this->LabelsPriority->InsertNextValue( std::to_string ( ii ) );
    }

  this->FocalPoint->Modified();
  this->FocalData->GetPointData()->GetNormals()->Modified();
  this->FocalData->Modified();

  this->SelectedFocalPoint->SetNumberOfPoints( 0 );
  this->SelectedFocalData->GetPointData()->GetNormals()->SetNumberOfTuples( 0 );
  this->SelectedLabels->SetNumberOfValues( 0 );
  this->SelectedLabelsPriority->SetNumberOfValues( 0 );

  for ( ii = 0; ii < numPoints; ii++ )
    {
    if ( !this->GetNthNodeVisibility( ii ) ||
         ii == this->GetActiveNode() ||
         !this->GetNthNodeSelected( ii ) )
      {
      continue;
      }

    double worldPos[3], worldOrient[9] = {0}, orientation[4] = {0};
    this->GetNthNodeWorldPosition( ii, worldPos );
    bool skipPoint = false;
    for (int jj = 0; jj < this->SelectedFocalPoint->GetNumberOfPoints(); jj++ )
      {
      double* pos = this->SelectedFocalPoint->GetPoint(jj);
      double eps = 0.001;
      if ( fabs(pos[0] - worldPos[0]) < eps &&
           fabs(pos[1] - worldPos[1]) < eps &&
           fabs(pos[2] - worldPos[2]) < eps )
        {
        skipPoint = true;
        break;
        }
      }

    if ( skipPoint )
      {
      continue;
      }

    this->SelectedFocalPoint->InsertNextPoint( worldPos );
    this->GetNthNodeOrientation( ii, orientation );
    this->FromOrientationQuaternionToWorldOrient( orientation, worldOrient );
    this->SelectedFocalData->GetPointData()->GetNormals()->InsertNextTuple( worldOrient + 6 );
    this->SelectedLabels->InsertNextValue( this->GetNthNodeLabel( ii ) );
    this->SelectedLabelsPriority->InsertNextValue( std::to_string( ii ) );
    }

  this->SelectedFocalPoint->Modified();
  this->SelectedFocalData->GetPointData()->GetNormals()->Modified();
  this->SelectedFocalData->Modified();

  if (this->GetActiveNode() >= 0 &&
      this->GetActiveNode() < this->GetNumberOfNodes() &&
      this->GetNthNodeVisibility( this->GetActiveNode() ) )
    {
    double worldPos[3], worldOrient[9] = {0}, orientation[4] = {0};
    this->GetActiveNodeWorldPosition( worldPos );
    this->ActiveFocalPoint->SetPoint( 0, worldPos );
    this->GetActiveNodeOrientation( orientation );
    this->FromOrientationQuaternionToWorldOrient( orientation, worldOrient );
    this->ActiveFocalData->GetPointData()->GetNormals()->SetTuple( 0, worldOrient + 6 );

    this->ActiveFocalPoint->Modified();
    this->ActiveFocalData->GetPointData()->GetNormals()->Modified();
    this->ActiveFocalData->Modified();

    this->ActiveLabels->SetValue( 0, this->GetActiveNodeLabel() );
    this->ActiveLabelsPriority->SetValue( 0, std::to_string(this->GetActiveNode()) );

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
int vtkSlicerAbstractRepresentation3D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
{
  if ( this->GetAssemblyPath(X, Y, 0., this->CursorPicker) )
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::Nearby;
    }
  else
    {
    this->InteractionState = vtkSlicerAbstractRepresentation::Outside;
    }
  return this->InteractionState;
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkSlicerAbstractRepresentation3D::WidgetInteraction(double eventPos[2])
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
void vtkSlicerAbstractRepresentation3D::TranslateNode(double eventPos[2])
{
  if (this->GetActiveNodeLocked())
    {
    return;
    }

  double ref[3];

  if (!this->GetActiveNodeWorldPosition(ref))
    {
    return;
    }

  double displayPos[2];
  displayPos[0] = eventPos[0] + this->InteractionOffset[0];
  displayPos[1] = eventPos[1] + this->InteractionOffset[1];

  double worldPos[3], worldOrient[9];
  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient))
    {
    if (this->RestrictFlag != vtkSlicerAbstractRepresentation::RestrictNone)
      {
      double oldWorldPos[3];
      this->GetActiveNodeWorldPosition(oldWorldPos);
      for (int i = 0; i < 3; ++i)
        { 
        worldPos[i] = (this->RestrictFlag == (i + 1)) ? worldPos[i] : oldWorldPos[i];
        }
      }
    this->SetActiveNodeToWorldPosition(worldPos);
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::ShiftWidget(double eventPos[2])
{
  double ref[3] = {0.};
  double displayPos[2] = {0.};
  double worldPos[3], worldOrient[9];

  displayPos[0] = this->LastEventPosition[0];
  displayPos[1] = this->LastEventPosition[1];

  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient))
    {
    ref[0] = worldPos[0];
    ref[1] = worldPos[1];
    ref[2] = worldPos[2];
    }
  else
    {
    return;
    }

  displayPos[0] = eventPos[0];
  displayPos[1] = eventPos[1];

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient))
    {
    return;
    }

  double vector[3];
  vector[0] = worldPos[0] - ref[0];
  vector[1] = worldPos[1] - ref[1];
  vector[2] = worldPos[2] - ref[2];

  // SetNthNodeWorldPosition calls vtkMRMLMarkupsNode::PointModifiedEvent reporting the id of the point modified.
  // However, already for > 200 points, it gets bad perfomance. Therefore, we call a simply modified call at the end.
  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (this->GetNthNodeLocked(i))
      {
      continue;
      }

    this->GetNthNodeWorldPosition(i, ref);
    for (int i = 0; i < 3; ++i)
      {
      if ( this->RestrictFlag != vtkSlicerAbstractRepresentation::RestrictNone )
        {
        worldPos[i] = (this->RestrictFlag == (i + 1)) ? ref[i] + vector[i] : ref[i];
        }
      else
        {
        worldPos[i] = ref[i] + vector[i];
        }
      }

    this->SetNthNodeWorldPosition(i, worldPos);
    }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::ScaleWidget(double eventPos[2])
{
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

  double centroid[3];
  ComputeCentroid(centroid);

  double r2 = vtkMath::Distance2BetweenPoints(ref, centroid);

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient))
    {
    return;
    }
  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centroid);
  if (d2 < 0.0000001)
    {
    return;
    }

  double ratio = sqrt(d2 / r2);

  // SetNthNodeWorldPosition calls vtkMRMLMarkupsNode::PointModifiedEvent reporting the id of the point modified.
  // However, already for > 200 points, it gets bad perfomance. Therefore, we call a simply modified call at the end.
  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (this->GetNthNodeLocked(i))
      {
      continue;
      }

    this->GetNthNodeWorldPosition(i, ref);
    for (int i = 0; i < 3; i++)
      {
      worldPos[i] = centroid[i] + ratio * (ref[i] - centroid[i]);
      }

    this->SetNthNodeWorldPosition(i, worldPos);
    }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::RotateWidget(double eventPos[2])
{
  // If any node is locked return
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (this->GetNthNodeLocked(i))
      {
      return;
      }
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

  double centroid[3];
  ComputeCentroid(centroid);

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient))
    {
    return;
    }

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
                 ( vtkMath::AngleBetweenVectors( lastWorldPos, worldPos ) );

  // SetNthNodeWorldPosition calls vtkMRMLMarkupsNode::PointModifiedEvent reporting the id of the point modified.
  // However, already for > 200 points, it gets bad perfomance. Therefore, we call a simply modified call at the end.
  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    this->GetNthNodeWorldPosition(i, ref);
    for (int i = 0; i < 3; i++)
      {
      ref[i] -= centroid[i];
      }
    vtkNew<vtkTransform> RotateTransform;
    RotateTransform->RotateY(angle);
    RotateTransform->TransformPoint(ref, worldPos);

    for (int i = 0; i < 3; i++)
      {
      worldPos[i] += centroid[i];
      }

    this->SetNthNodeWorldPosition(i, worldPos);
    }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
  this->SelectedActor->GetActors(pc);
  this->ActiveActor->GetActors(pc);
  this->LabelsActor->GetActors(pc);
  this->SelectedLabelsActor->GetActors(pc);
  this->ActiveLabelsActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::ReleaseGraphicsResources(
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
int vtkSlicerAbstractRepresentation3D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  if (this->Actor->GetVisibility())
    {
    count += this->Actor->RenderOverlay(viewport);
    }
  if (this->SelectedActor->GetVisibility())
    {
    count += this->SelectedActor->RenderOverlay(viewport);
    }
  if (this->ActiveActor->GetVisibility())
    {
    count += this->ActiveActor->RenderOverlay(viewport);
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
int vtkSlicerAbstractRepresentation3D::RenderOpaqueGeometry(
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
int vtkSlicerAbstractRepresentation3D::RenderTranslucentPolygonalGeometry(
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
vtkTypeBool vtkSlicerAbstractRepresentation3D::HasTranslucentPolygonalGeometry()
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
void vtkSlicerAbstractRepresentation3D::PrintSelf(ostream& os,
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
