/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerPointsRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlicerPointsRepresentation3D.h"
#include "vtkAppendPolyData.h"
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

vtkStandardNewMacro(vtkSlicerPointsRepresentation3D);

//----------------------------------------------------------------------
vtkSlicerPointsRepresentation3D::vtkSlicerPointsRepresentation3D()
{
  this->appendActors = vtkAppendPolyData::New();
  this->appendActors->AddInputData(this->CursorShape);
  this->appendActors->AddInputData(this->SelectedCursorShape);
  this->appendActors->AddInputData(this->ActiveCursorShape);
}

//----------------------------------------------------------------------
vtkSlicerPointsRepresentation3D::~vtkSlicerPointsRepresentation3D()
{
  this->appendActors->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerPointsRepresentation3D::BuildLines()
{
  return;
}

//----------------------------------------------------------------------
void vtkSlicerPointsRepresentation3D::Highlight(int vtkNotUsed(highlight))
{
  return;
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerPointsRepresentation3D::GetWidgetRepresentationAsPolyData()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput();
}

//-----------------------------------------------------------------------------
double *vtkSlicerPointsRepresentation3D::GetBounds()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput()->GetPoints() ?
              this->appendActors->GetOutput()->GetBounds() : nullptr;
}
