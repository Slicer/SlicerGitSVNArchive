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

#include "vtkSlicerLineRepresentation.h"
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

vtkStandardNewMacro(vtkSlicerLineRepresentation);

//----------------------------------------------------------------------
vtkSlicerLineRepresentation::vtkSlicerLineRepresentation()
{
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
}

//----------------------------------------------------------------------
vtkSlicerLineRepresentation::~vtkSlicerLineRepresentation()
{
  this->Lines->Delete();
  this->LinesMapper->Delete();
  this->LinesActor->Delete();
  this->LinesProperty->Delete();
  this->ActiveLinesProperty->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation::CreateDefaultProperties()
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
void vtkSlicerLineRepresentation::Highlight(int highlight)
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
void vtkSlicerLineRepresentation::BuildLines()
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
  vtkIdType numLines = count;
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

    lines->InsertNextCell(numLines, lineIndices);
    delete [] lineIndices;
  }

  this->Lines->SetPoints(points);
  this->Lines->SetLines(lines);

  points->Delete();
  lines->Delete();
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerLineRepresentation::GetWidgetRepresentationAsPolyData()
{
  return this->Lines;
}


//----------------------------------------------------------------------
void vtkSlicerLineRepresentation::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
  this->ActiveActor->GetActors(pc);
  this->LinesActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->ActiveActor->ReleaseGraphicsResources(win);
  this->LinesActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerLineRepresentation::RenderOverlay(vtkViewport *viewport)
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
int vtkSlicerLineRepresentation::RenderOpaqueGeometry(
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
int vtkSlicerLineRepresentation::RenderTranslucentPolygonalGeometry(
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
vtkTypeBool vtkSlicerLineRepresentation::HasTranslucentPolygonalGeometry()
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
void vtkSlicerLineRepresentation::SetLineColor(
  double r, double g, double b)
{
  if (this->GetLinesProperty())
  {
    this->GetLinesProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------
double *vtkSlicerLineRepresentation::GetBounds()
{
  return this->Lines->GetPoints() ?
              this->Lines->GetPoints()->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
void vtkSlicerLineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

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

