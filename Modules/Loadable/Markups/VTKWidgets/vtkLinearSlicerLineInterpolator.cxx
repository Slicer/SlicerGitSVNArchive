/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearSlicerLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSlicerAbstractRepresentation.h"
#include "vtkLinearSlicerLineInterpolator.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkLinearSlicerLineInterpolator);

//----------------------------------------------------------------------
vtkLinearSlicerLineInterpolator::vtkLinearSlicerLineInterpolator() = default;

//----------------------------------------------------------------------
vtkLinearSlicerLineInterpolator::~vtkLinearSlicerLineInterpolator() = default;

//----------------------------------------------------------------------
int vtkLinearSlicerLineInterpolator::InterpolateLine( vtkSlicerAbstractRepresentation *rep,
                                                      int idx1, int idx2 )
{
  double p1[3] = {0}, p2[3] = {0};
  rep->GetNthNodeWorldPosition( idx1, p1 );
  rep->AddIntermediatePointWorldPosition( idx1, p1 );
  rep->GetNthNodeWorldPosition( idx2, p2 );
  rep->AddIntermediatePointWorldPosition( idx2, p2 );
  return 1;
}

//----------------------------------------------------------------------
void vtkLinearSlicerLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

