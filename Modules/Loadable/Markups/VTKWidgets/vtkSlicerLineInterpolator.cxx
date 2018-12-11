/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlicerLineInterpolator.h"

#include "vtkSlicerAbstractRepresentation.h"
#include "vtkIntArray.h"

//----------------------------------------------------------------------
vtkSlicerLineInterpolator::vtkSlicerLineInterpolator() = default;

//----------------------------------------------------------------------
vtkSlicerLineInterpolator::~vtkSlicerLineInterpolator() = default;

//----------------------------------------------------------------------
void vtkSlicerLineInterpolator::GetSpan( int nodeIndex,
                                          vtkIntArray *nodeIndices,
                                          vtkSlicerAbstractRepresentation *rep)
{
  int start = nodeIndex - 1;
  int end   = nodeIndex;
  int index[2];

  // Clear the array
  nodeIndices->Reset();
  nodeIndices->Squeeze();
  nodeIndices->SetNumberOfComponents(2);

  for ( int i = 0; i < 3; i++ )
    {
    index[0] = start++;
    index[1] = end++;

    if ( rep->GetClosedLoop() )
      {
      if ( index[0] < 0 )
        {
        index[0] += rep->GetNumberOfNodes();
        }
      if ( index[1] < 0 )
        {
        index[1] += rep->GetNumberOfNodes();
        }
      if ( index[0] >= rep->GetNumberOfNodes() )
        {
        index[0] -= rep->GetNumberOfNodes();
        }
      if ( index[1] >= rep->GetNumberOfNodes() )
        {
        index[1] -= rep->GetNumberOfNodes();
        }
      }

    if ( index[0] >= 0 && index[0] < rep->GetNumberOfNodes() &&
         index[1] >= 0 && index[1] < rep->GetNumberOfNodes() )
      {
      nodeIndices->InsertNextTypedTuple( index );
      }
    }
}

//----------------------------------------------------------------------
void vtkSlicerLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
