// Copyright (c) 2010, Jerome Velut
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT OWNER ``AS IS'' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
// NO EVENT SHALL THE COPYRIGHT OWNER BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "vtkFrenetSerretFrame.h"

#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkFrenetSerretFrame);

vtkFrenetSerretFrame::vtkFrenetSerretFrame( )
{
  this->ConsistentNormals = 1;
  this->ComputeBinormal = 1;
  this->ViewUp = 0;
}

vtkFrenetSerretFrame::~vtkFrenetSerretFrame( )
{

}
//---------------------------------------------------------------------------
int vtkFrenetSerretFrame::FillInputPortInformation(int port, vtkInformation *info)
{
  if( port == 0 )
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  }
  return 1;
}

int vtkFrenetSerretFrame::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->DeepCopy( input );

  vtkDoubleArray* tangents = vtkDoubleArray::New( );
  tangents->SetNumberOfComponents( 3 );
  tangents->SetNumberOfTuples( input->GetNumberOfPoints( ) );
  tangents->SetName("FSTangents");

  vtkDoubleArray* normals = vtkDoubleArray::New( );
  normals->SetNumberOfComponents( 3 );
  normals->SetNumberOfTuples( input->GetNumberOfPoints( ) );
  normals->SetName("FSNormals");

  vtkDoubleArray* binormals = vtkDoubleArray::New( );
  binormals->SetNumberOfComponents( 3 );
  binormals->SetNumberOfTuples( input->GetNumberOfPoints( ) );
  binormals->SetName("FSBinormals");


  vtkCellArray* lines = output->GetLines( );
  lines->InitTraversal();
  vtkIdType nbPoints;
  vtkIdType* points;

  int cellIdx;
  for( cellIdx = 0; cellIdx < lines->GetNumberOfCells( ); cellIdx++ )
  {
    lines->GetNextCell( nbPoints, points);
    for( int i = 0 ; i<nbPoints; i++)
    {
      double tangent[3];
      if( i == 0 )
      {
        this->ComputeTangentVectors( points[0],points[1],tangent );
      }
      else if( i == nbPoints-1 )
      {
        this->ComputeTangentVectors( points[nbPoints-2],points[nbPoints-1],tangent );
      }
      else
      {
        this->ComputeTangentVectors( points[i-1],points[i+1],tangent );
      }
      vtkMath::Normalize( tangent );
      tangents->SetTuple(points[i],tangent);
    }


    for( int i = 0 ; i<nbPoints; i++)
    {
      double normal[3];
      if( !this->ConsistentNormals || i == 0)
      {
        double tangentLast[3], tangentNext[3];
        if( i == 0 )
        {
          tangents->GetTuple( points[i], tangentLast);
        }
        else
        {
          tangents->GetTuple( points[i-1], tangentLast);
        }
        if( i == nbPoints-1 )
        {
          tangents->GetTuple( points[i], tangentNext);
        }
        else
        {
          tangents->GetTuple( points[i+1], tangentNext);
        }
        this->ComputeNormalVectors( tangentLast, tangentNext, normal );

        if( this->ConsistentNormals )
        {
          this->RotateVector( normal, tangentLast, this->ViewUp );
        }
      }

      if( this->ConsistentNormals && i != 0)
      {

        double tangent[3], lastNormal[3];
        normals->GetTuple(points[i-1],lastNormal);
        tangents->GetTuple(points[i],tangent);

        this->ComputeConsistentNormalVectors( tangent,
                                              lastNormal,
                                              normal );

      }
      vtkMath::Normalize( normal );
      normals->SetTuple(points[i],normal);

      if( this->ComputeBinormal == 1 )
      {
        double tangent[3], binormal[3];
        tangents->GetTuple( points[i], tangent );
        vtkMath::Cross( tangent, normal, binormal );
        binormals->SetTuple( points[i], binormal );
      }
    }
  }

  output->GetPointData( )->AddArray( normals );
  output->GetPointData( )->AddArray( tangents );
  if( this->ComputeBinormal == 1 )
  {
    output->GetPointData( )->AddArray( binormals );
  }
  normals->Delete( );
  tangents->Delete( );
  binormals->Delete( );
  return 1;
}

void vtkFrenetSerretFrame::ComputeTangentVectors( vtkIdType pointIdNext, vtkIdType pointIdLast, double* tangent )
{

  vtkPolyData* input = static_cast<vtkPolyData*>(this->GetInput( 0 ));
  double ptNext[3];
  double ptLast[3];

  input->GetPoint( pointIdNext, ptNext);
  input->GetPoint( pointIdLast, ptLast);

  int comp;
  for( comp = 0; comp < 3; comp++ )
  {
    tangent[comp] = ( ptLast[comp] - ptNext[comp] )/2;
  }
}

void vtkFrenetSerretFrame::ComputeConsistentNormalVectors( double* tangent,
                                                           double* normalLast,
                                                           double* normal )
{
  double temp[3];
  vtkMath::Cross( normalLast, tangent, temp);
  vtkMath::Cross( tangent, temp, normal );
}

void vtkFrenetSerretFrame::ComputeNormalVectors( double* tgNext,
                                                 double* tgLast,
                                                 double* normal )
{
  int comp;
  for( comp = 0; comp < 3; comp++ )
  {
    normal[comp] = ( tgNext[comp] - tgLast[comp] );
  }
  if( vtkMath::Norm(normal) == 0 ) // tgNext == tgLast
  {
    double unit[3] = {1,0,0};
    vtkMath::Cross( tgLast, unit, normal );
  }

}

void vtkFrenetSerretFrame::RotateVector( double* vector, const double* axis, double angle )
{
  double UdotN = vtkMath::Dot(vector,axis);
  double NvectU[3];
  vtkMath::Cross(axis, vector, NvectU);

  for( int comp = 0; comp < 3 ; comp++)
  {
    vector[comp] =  cos( angle )*vector[comp]
      + (1-cos( angle ))*UdotN*axis[comp]
      + sin( angle )*NvectU[comp];
  }
}
